#include <libsolidity/modelcheck/scheduler/CompInvarGenerator.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/TypeAnalyzer.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/Primitives.h>

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
    InvarRule _rule,
    InvarType _type,
    bool _infer
): m_stack(_stack), m_rule(_rule), m_type(_type), m_infer(_infer)
{
    for (auto actor : _actors)
    {
        auto const& contract = (*actor.contract);
        for (auto decl : contract.state_variables())
        {
            identify_maps(actor.decl->id(), contract, contract.name(), decl);
        }
    }
}

// -------------------------------------------------------------------------- //

void CompInvarGenerator::print_invariants(ostream& _stream)
{
    if (m_rule == InvarRule::None) return;

    for (auto map : m_maps)
    {
        // Generates identifier.
        string ty = (m_infer ? "bool" : "int");
        string infer_name = "Infer_" + to_string(map.id);
        auto inv_id = make_shared<CVarDecl>(ty, "Inv_" + to_string(map.id));
        auto infer_id = make_shared<CVarDecl>(ty, infer_name);

        // Generates body.
        auto params = generate_params(map.fields);
        auto body = make_shared<CBlock>(generate_body(infer_name, params));

        // Outputs definitions.
        CFuncDef inv(inv_id, params, move(body));
        if (m_infer)
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

CBlockList CompInvarGenerator::apply_interference(NondetSourceRegistry &nd_reg)
{
    CBlockList block;

    for (auto map : m_maps)
    {
        // Determines initial index.
        size_t offset = 0;
        if (m_type != InvarType::Universal)
        {
            offset = m_stack->addresses()->implicit_count();
        }

        // Non-deterministically initializes each field.
        auto const WIDTH = m_stack->addresses()->count();
        auto const DEPTH = map.depth;
        KeyIterator indices(WIDTH, DEPTH, offset);
        do
        {
            if (indices.is_full())
            {
                // Determine field.
                string const FIELD = "data" + indices.suffix();
                auto const DATA = make_shared<CMemberAccess>(map.path, FIELD);

                // Create non-deterministic value.
                string const MSG = map.display + "::" + indices.suffix();
                auto const ND = nd_reg.val(*map.base_type, MSG);

                // Initializes.
                block.push_back(DATA->assign(ND)->stmt());

                // Applies the invariant, if applicable.
                if (m_rule != InvarRule::None)
                {
                    apply_invariant(block, false, DATA, map);
                }
            }
        } while (indices.next());
    }

    return block;
}

// -------------------------------------------------------------------------- //

CBlockList CompInvarGenerator::check_interference()
{

    // Only generate assertions if the invariants are checked.
    CBlockList block;
    if (m_rule != InvarRule::Checked)
    {
        return block;
    }

    // Generates each assertion.
    for (auto map : m_maps)
    {
        // Determines initial index.
        // TODO: Repeated.
        size_t offset = 0;
        if (m_type != InvarType::Universal)
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
                // Determine field.
                string const FIELD = "data" + indices.suffix();
                auto const DATA = make_shared<CMemberAccess>(map.path, FIELD);

                // Applies the invariant, if applicable.
                apply_invariant(block, true, DATA, map);
            }
        } while (indices.next());
    }

    // Returns all assertions.
    return block;
}

// -------------------------------------------------------------------------- //

void CompInvarGenerator::identify_maps(
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
            identify_maps(_path, _contract, _display, child.get());
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
}

// -------------------------------------------------------------------------- //

void CompInvarGenerator::apply_invariant(
    CBlockList &_block, bool _assert, CExprPtr _data, MapData const& _map
)
{
    // Generates invariant call.
    CFuncCallBuilder inv_builder("Inv_" + to_string(_map.id));
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
    if (m_infer)
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

CParams CompInvarGenerator::generate_params(MapFieldList const& _fields)
{
    CParams params;
    for (auto field : _fields)
    {
        // Determines type.
        string raw;
        auto type = field.type;
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

        // TODO: extract bool, width, ect. (reuse primitive analysis).
        string name = "v" + to_string(params.size());
        params.push_back(make_shared<CVarDecl>(raw, name, false));
    }
    return params;
}

// -------------------------------------------------------------------------- //

CBlockList CompInvarGenerator::generate_body(string _infer, CParams &_params)
{
    CBlockList stmts;

    auto default_ret = make_shared<CReturn>(Literals::ONE);
    if (m_infer)
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
    if (auto struct_type = dynamic_cast<StructType const*>(_ty))
    {
        for (auto decl : struct_type->structDefinition().members())
        {
            auto name = VariableScopeResolver::rewrite(
                decl->name(), false, VarContext::STRUCT
            );

            _path.push_back(name);
            extract_map_fields(_fields, _path, decl->annotation().type);
            _path.pop_back();
        }
    }
    else
    {
        _fields.push_back(MapField{_path, _ty});
    }
}

// -------------------------------------------------------------------------- //

}
}
}
