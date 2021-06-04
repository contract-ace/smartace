#include <libsolidity/modelcheck/scheduler/CompInvarGenerator.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/TypeAnalyzer.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/Primitives.h>
#include <libsolidity/modelcheck/utils/Types.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

CompInvarGenerator::CompInvarGenerator(
    shared_ptr<AnalysisStack const> _stack,
    list<Actor> const& _actors,
    Settings _settings
): m_stack(_stack), m_settings(_settings)
{
    // Extracts mappings.
    for (auto actor : _actors)
    {
        auto const& contract = (*actor.contract);
        for (auto decl : contract.state_variables())
        {
            analyze_actor(actor.decl->id(), contract, contract.name(), decl);
        }
    }
}

// -------------------------------------------------------------------------- //

void CompInvarGenerator::print_invariants(ostream& _stream)
{
    if (m_settings.rule == InvarRule::None) return;

    for (auto map : m_maps)
    {
        // Generates identifier.
        string ty = (m_settings.inferred ? "bool" : "int");
        string infer_name = "Infer_" + to_string(map.id);
        auto inv_id = make_shared<CVarDecl>(ty, "Inv_" + to_string(map.id));
        auto infer_id = make_shared<CVarDecl>(ty, infer_name);

        // Generates body.
        auto params = make_params(map.fields);
        auto body = make_shared<CBlock>(make_body(infer_name, params));

        // Outputs definitions.
        CFuncDef inv(inv_id, params, move(body));
        if (m_settings.inferred)
        {
            CFuncDef inf(infer_id, params, nullptr, CFuncDef::Modifier::EXTERN);
            _stream << inf;
            _stream << "PARTIAL_FN " << inv;
        }
        else
        {
            _stream << inv;
        }
    }
}

// -------------------------------------------------------------------------- //

CBlockList CompInvarGenerator::apply_interference(NondetSourceRegistry &_nd_reg)
{
    // Wraps interference application in a lambda.
    CBlockList block;
    auto apply = [&self=(*this),&_nd_reg,&block]
                 (MapData const& _map, KeyIterator const& _indices)
    {
        // Determine field.
        string const FIELD = "data" + _indices.suffix();
        auto const DATA = make_shared<CMemberAccess>(_map.path, FIELD);

        // Create non-deterministic value.
        string const MSG = _map.display + "::" + _indices.suffix();
        auto const ND = _nd_reg.val(*_map.base_type, MSG);

        // Initializes.
        block.push_back(DATA->assign(ND)->stmt());

        // Applies the invariant, if applicable.
        if (self.m_settings.rule != InvarRule::None)
        {
            self.apply_invariant(block, false, DATA, _map);
        }
    };

    // Populates and returns block.
    expand_map(apply);
    return block;
}

// -------------------------------------------------------------------------- //

CBlockList CompInvarGenerator::check_interference()
{
    // Only generate assertions if the invariants are checked.
    CBlockList block;
    if (m_settings.rule != InvarRule::Checked)
    {
        return block;
    }

    // Wraps interference checking in a lambda.
    auto check = [&self=(*this),&block]
                 (MapData const& _map, KeyIterator const& _indices)
    {
        // Determine field.
        string const FIELD = "data" + _indices.suffix();
        auto const DATA = make_shared<CMemberAccess>(_map.path, FIELD);

        // Applies the invariant, if applicable.
        self.apply_invariant(block, true, DATA, _map);
    };

    // Populates and returns block.
    expand_map(check);
    return block;
}

// -------------------------------------------------------------------------- //

void CompInvarGenerator::analyze_actor(
    CExprPtr _path,
    FlatContract const& _contract,
    string _display,
    VariableDeclaration const* _decl
)
{
    // Updates path.
    auto const NAME = VariableScopeResolver::rewrite(
        _decl->name(), false, VarContext::STRUCT
    );
    _path = make_shared<CMemberAccess>(_path, NAME);
    _display += "::" + _decl->name();

    // Determines if _decl is a map/struct.
    if (auto rec = _contract.find_structure(_decl))
    {
        // If _decl is a struct, expand to all children.
        for (auto child : rec->fields())
        {
            analyze_actor(_path, _contract, _display, child.get());
        }
    }
    else if (auto entry = m_stack->types()->map_db().resolve(*_decl))
    {
        // Registers map.
        m_maps.emplace_back();
        m_maps.back().id = m_maps.size();
        m_maps.back().depth = entry->key_types.size();
        m_maps.back().path = _path;
        m_maps.back().base_type = entry->value_type;
        m_maps.back().display = _display;

        // Populates fields.
        list<string> path;
        auto & fields = m_maps.back().fields;
        extract_map_fields(fields, path, entry->value_type->annotation().type);
    }
    else if (m_settings.stateful && is_simple_type(*_decl->annotation().type))
    {
        if (_decl->annotation().type->category() != Type::Category::Address)
        {
            m_control_state.emplace_back();
            m_control_state.back().path = _path;
            m_control_state.back().type = _decl->annotation().type;
        }
    }
}

// -------------------------------------------------------------------------- //

void CompInvarGenerator::expand_map(MapVisitor _f)
{
    for (auto map : m_maps)
    {
        // Determines initial index.
        size_t offset = 0;
        if (m_settings.type != InvarType::Universal)
        {
            offset = m_stack->addresses()->implicit_count();
        }

        // Checks each field.
        auto const WIDTH = m_stack->addresses()->count();
        auto const DEPTH = map.depth;
        KeyIterator indices(WIDTH, DEPTH, offset);
        do
        {
            if (indices.is_full())
            {
                _f(map, indices);
            }
        } while (indices.next());
    }
}

// -------------------------------------------------------------------------- //

void CompInvarGenerator::apply_invariant(
    CBlockList &_block, bool _assert, CExprPtr _data, MapData const& _map
) const
{
    // Generates invariant call.
    CFuncCallBuilder inv_builder("Inv_" + to_string(_map.id));
    for (auto state : m_control_state)
    {
        inv_builder.push(make_shared<CMemberAccess>(state.path, "v"));
    }
    for (auto field : _map.fields)
    {
        // Extracts data.
        auto data = _data;
        for (auto id : field.path)
        {
            data = make_shared<CMemberAccess>(data, id);
        }
        data = make_shared<CMemberAccess>(data, "v");
        inv_builder.push(data);
    }
    auto inv_call = inv_builder.merge_and_pop();

    // Applies invariant.
    if (m_settings.inferred)
    {
        CFuncCallBuilder chk_builder(_assert ? "__VERIFIER_assert" : "assume");
        chk_builder.push(inv_call);
        _block.push_back(chk_builder.merge_and_pop_stmt());
    }
    else
    {
        if (_assert)
        {
            LibVerify::add_assert(_block, inv_call);
        }
        else
        {
            LibVerify::add_require(_block, inv_call);
        }
    }
}

// -------------------------------------------------------------------------- //

CParams CompInvarGenerator::make_params(MapFieldList const& _fields) const
{
    // Helper function to add params.
    CParams params;
    auto add_param = [&params](Type const* type)
    {
        // Determines type.
        string raw;
        if (type->category() == Type::Category::Bool)
        {
            raw = PrimitiveToRaw::boolean();
        }
        else if (auto itype = dynamic_cast<IntegerType const*>(type))
        {
            raw = PrimitiveToRaw::integer(itype->numBits(), itype->isSigned());
        }
        else
        {
            throw runtime_error("Unknown type: " + type->canonicalName());
        }

        // Adds param to list.
        string name = "v" + to_string(params.size());
        params.push_back(make_shared<CVarDecl>(raw, name, false));
    };

    // Adds control state to invariant signature.
    for (auto state : m_control_state)
    {
        add_param(state.type);
    }

    // Adds mapping data to invariant signature.
    for (auto field : _fields)
    {
        add_param(field.type);
    }

    // Returns parameter list.
    return params;
}

// -------------------------------------------------------------------------- //

CBlockList CompInvarGenerator::make_body(
    string const& _infer, CParams const& _params
) const
{
    CBlockList stmts;

    auto default_ret = make_shared<CReturn>(Literals::ONE);
    if (m_settings.inferred)
    {
        CFuncCallBuilder infer_builder(_infer);

        // Computes base case. In the base case, every field is zero.
        CExprPtr base_case = Literals::ONE;
        for (auto param : _params)
        {
            auto arg = param->id();

            // Updates base case.
            auto check = make_shared<CBinaryOp>(arg, "==", Literals::ZERO);
            base_case = make_shared<CBinaryOp>(check, "&&", base_case);

            // Passes along to infer.
            infer_builder.push(arg);
        }
        stmts.push_back(make_shared<CIf>(base_case, default_ret));

        // Fallback to synthesis.
        stmts.push_back(make_shared<CReturn>(infer_builder.merge_and_pop()));
    }
    else
    {
        stmts.push_back(default_ret);
    }

    return stmts;
}

// -------------------------------------------------------------------------- //

void CompInvarGenerator::extract_map_fields(
    MapFieldList &_fields, list<string> &_path, Type const *_ty
)
{
    // If this is a structure, every sub-field must be analyzed.
    if (auto struct_type = dynamic_cast<StructType const*>(_ty))
    {
        for (auto decl : struct_type->structDefinition().members())
        {
            // Determines field name.
            auto name = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );

            // Recurses on field.
            _path.push_back(name);
            extract_map_fields(_fields, _path, decl->annotation().type);
            _path.pop_back();
        }
    }
    else if (is_simple_type(*_ty))
    {
        _fields.push_back(MapField{_path, _ty});
    }
}

// -------------------------------------------------------------------------- //

}
}
}
