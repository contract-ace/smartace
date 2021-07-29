#include <libsolidity/modelcheck/scheduler/CompInvarGenerator.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/analysis/Inheritance.h>
#include <libsolidity/modelcheck/analysis/TypeAnalyzer.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>
#include <libsolidity/modelcheck/scheduler/ActorModel.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/Primitives.h>
#include <libsolidity/modelcheck/utils/Types.h>

#include <set>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

namespace
{
shared_ptr<CVarDecl> make_param(size_t i, Type const* type)
{
    // Determines type.
    Type const& raw_type = unwrap(*type);
    string raw;
    if (raw_type.category() == Type::Category::Bool)
    {
        raw = PrimitiveToRaw::boolean();
    }
    else if (auto itype = dynamic_cast<IntegerType const*>(&raw_type))
    {
        raw = PrimitiveToRaw::integer(itype->numBits(), itype->isSigned());
    }
    else if (auto array_ptr = dynamic_cast<ArrayType const*>(&raw_type))
    {
        if (array_ptr->isString())
        {
            raw = PrimitiveToRaw::integer(256, false);
        }
    }

    // Checks type was found
    if (raw.empty())
    {
        throw runtime_error("Unknown type: " + type->canonicalName());
    }

    // Adds param to list.
    string name = "v" + to_string(i);
    return make_shared<CVarDecl>(raw, name, false);
}
}

// -------------------------------------------------------------------------- //

CompInvarGenerator::CompInvarGenerator(
    shared_ptr<AnalysisStack const> _stack,
    ActorModel const& _actors,
    Settings _settings
): m_stack(_stack), m_settings(_settings), m_roles(_actors.vars())
{
    // Extracts mappings.
    for (auto actor : _actors.inspect())
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
        CStmtPtr initializer = DATA->assign(ND)->stmt();
        initializer = self.guard(initializer, _indices.view());
        block.push_back(initializer);
        
        // Collects values.
        // * Factor into call
        vector<CExprPtr> values;
        for (auto field : _map.fields)
        {
            // Extracts data.
            auto data = DATA;
            for (auto id : field.path)
            {
                data = make_shared<CMemberAccess>(data, id);
            }
            data = make_shared<CMemberAccess>(data, "v");
            values.push_back(data);
        }
        self.apply_invariant(block, false, _map, values, _indices.view());
    };

    // Populates and returns block.
    expand_maps(apply);
    return block;
}

// -------------------------------------------------------------------------- //

CBlockList CompInvarGenerator::check_interference(NondetSourceRegistry &_nd_reg)
{
    // Only generate assertions if the invariants are checked.
    CBlockList block;
    if (m_settings.rule != InvarRule::Checked)
    {
        return block;
    }

    // [...]
    CBlockList default_case;
    string default_err("Model failure, entry_id out of bounds.");
    LibVerify::add_require(default_case, Literals::ZERO, default_err);

    // Selects mapping and field.
    auto map_id = make_shared<CVarDecl>("uint64_t", "map_id");
    auto mcases = make_shared<CSwitch>(map_id->id(), CBlockList{});
    for (auto map : m_maps)
    {
        CBlockList map_block;

        // [...]
        std::vector<shared_ptr<CVarDecl>> vars;
        std::vector<CExprPtr> values;
        for (auto field : map.fields)
        {
            vars.push_back(make_param(vars.size(), field.type));
            map_block.push_back(vars.back());
            values.push_back(vars.back()->id());
        }

        // [...]
        auto entry_id = make_shared<CVarDecl>("uint64_t", "entry_id");
        auto ecases = make_shared<CSwitch>(entry_id->id(), default_case);

        // [...]
        auto check = [&self=(*this),&ecases,&vars]
                     (MapData const& _map, KeyIterator const& _indices)
        {
            CBlockList entry_block;

            // [...]
            auto gv = make_shared<CVarDecl>("uint8_t", "guard");
            entry_block.push_back(gv);
            entry_block.push_back(gv->assign(Literals::ZERO)->stmt());
            entry_block.push_back(self.guard(
                gv->assign(Literals::ONE)->stmt(), _indices.view()
            ));
            LibVerify::add_require(entry_block, gv->id(), "Guard");

            // [...]
            // * Factor into call.
            string const FIELD = "data" + _indices.suffix();
            auto const DATA = make_shared<CMemberAccess>(_map.path, FIELD);
            for (size_t i = 0; i < _map.fields.size(); ++i)
            {
                auto const& field = _map.fields[i];
                auto const& var = vars[i];

                // Extracts data.
                auto data = DATA;
                for (auto id : field.path)
                {
                    data = make_shared<CMemberAccess>(data, id);
                }
                data = make_shared<CMemberAccess>(data, "v");
                entry_block.push_back(var->assign(data)->stmt());
            }

            // [...]
            entry_block.push_back(make_shared<CBreak>());
            ecases->add_case(ecases->size(), move(entry_block));
        };
        expand_map(map, check);

        // [...]
        vector<size_t> tmp;
        map_block.push_back(entry_id);
        map_block.push_back(entry_id->assign(
            _nd_reg.range(0, ecases->size(), "entry")
        )->stmt());
        map_block.push_back(ecases);
        apply_invariant(map_block, true, map, values, tmp);
        map_block.push_back(make_shared<CBreak>());
        mcases->add_case(mcases->size(), move(map_block));
    }

    // [...]
    block.push_back(map_id);
    block.push_back(map_id->assign(
        _nd_reg.range(0, mcases->size() + 1, "map")
    )->stmt());
    block.push_back(mcases);

    // Populates and returns block.
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
        vector<string> path;
        auto & fields = m_maps.back().fields;
        extract_map_fields(fields, path, entry->value_type->annotation().type);
    }
    else if (m_settings.stateful && is_simple_type(*_decl->annotation().type))
    {
        if (_decl->annotation().type->category() != Type::Category::Address)
        {
            if (!_decl->isConstant())
            {
                m_control_state.emplace_back();
                m_control_state.back().path = _path;
                m_control_state.back().type = _decl->annotation().type;
            }
        }
    }
}

// -------------------------------------------------------------------------- //

void CompInvarGenerator::expand_maps(MapVisitor _f)
{
    for (auto map : m_maps)
    {
        expand_map(map, _f);
    }
}

void CompInvarGenerator::expand_map(MapData const& _map, MapVisitor _f)
{
    // Determines initial index.
    size_t offset = 0;
    if (m_settings.type != InvarType::Universal)
    {
        offset = m_stack->addresses()->implicit_count();
    }

    // Checks each field.
    auto const WIDTH = m_stack->addresses()->count();
    auto const DEPTH = _map.depth;
    KeyIterator indices(WIDTH, DEPTH, offset);
    do
    {
        if (indices.is_full())
        {
            _f(_map, indices);
        }
    } while (indices.next());
}

// -------------------------------------------------------------------------- //

CStmtPtr CompInvarGenerator::guard(
    CStmtPtr _inst, std::vector<size_t> const& _indices
) const
{
    // If role guards are disabled, bypass.
    if (m_settings.type != InvarType::RoleBased)
    {
        return _inst;
    }

    // Computes role guard.
    CExprPtr guards = Literals::ZERO;
    for (auto i : _indices)
    {
        size_t offset = m_stack->addresses()->implicit_count();
        if (i >= offset)
        {
            // The mapping entry is abstract if at least one index is abstract.
            auto key = make_shared<CIntLiteral>(i);
            CExprPtr clause = Literals::ONE;
            for (auto role : m_roles)
            {
                auto term = make_shared<CBinaryOp>(role, "!=", key);
                clause = make_shared<CBinaryOp>(clause, "&&", term);
            }
            guards = make_shared<CBinaryOp>(guards, "||", clause);
        }
    }

    // Applies role guard to instruction.
    return make_shared<CIf>(guards, _inst);
}

// -------------------------------------------------------------------------- //

void CompInvarGenerator::apply_invariant(
    CBlockList &_block,
    bool _assert,
    MapData const& _map,
    vector<CExprPtr> const& _values,
    vector<size_t> const& _indices
) const
{
    // Ensures invariants are enabled.
    if (m_settings.rule == InvarRule::None)
    {
        return;
    }

    // Generates invariant call.
    CFuncCallBuilder inv_builder("Inv_" + to_string(_map.id));
    for (auto state : m_control_state)
    {
        inv_builder.push(make_shared<CMemberAccess>(state.path, "v"));
    }
    for (auto v : _values)
    {
        inv_builder.push(v);
    }
    auto inv_call = inv_builder.merge_and_pop();

    // Generates invariant application.
    CStmtPtr inv_chk;
    if (m_settings.inferred)
    {
        CFuncCallBuilder chk_builder(_assert ? "sassert" : "assume");
        chk_builder.push(inv_call);
        inv_chk = chk_builder.merge_and_pop_stmt();
    }
    else
    {
        if (_assert)
        {
            inv_chk = make_shared<CExprStmt>(LibVerify::make_assert(inv_call));
        }
        else
        {
            inv_chk = make_shared<CExprStmt>(LibVerify::make_require(inv_call));
        }
    }

    // If there are no indices, the guard is already checked.
    if (!_indices.empty())
    {
        inv_chk = guard(inv_chk, _indices);
    }

    // Applies invariant.
    _block.push_back(inv_chk);
}

// -------------------------------------------------------------------------- //

CParams CompInvarGenerator::make_params(MapFieldList const& _fields) const
{
    CParams params;

    // Adds control state to invariant signature.
    for (auto state : m_control_state)
    {
        params.push_back(make_param(params.size(), state.type));
    }

    // Adds mapping data to invariant signature.
    for (auto field : _fields)
    {
        params.push_back(make_param(params.size(), field.type));
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
    MapFieldList &_fields, vector<string> &_path, Type const *_ty
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
