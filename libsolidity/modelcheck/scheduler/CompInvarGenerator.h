/**
 * Used to generate compositional invariants. All invariant options are
 * implemented at this level.
 * 
 * @date 2021
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/modelcheck/codegen/Details.h>
#include <libsolidity/modelcheck/scheduler/ActorModel.h>
#include <libsolidity/modelcheck/utils/KeyIterator.h>
#include <libsolidity/modelcheck/utils/LibVerify.h>

#include <functional>
#include <list>
#include <memory>
#include <ostream>
#include <vector>

namespace dev
{
namespace solidity
{
namespace modelcheck
{

class NondetSourceRegistry;

// -------------------------------------------------------------------------- //

/**
 * Generates invariants for mappings.
 */
class CompInvarGenerator
{
public:
    // Specifies how invariants should be instrumented:
    // - None: Invariants are unused. All mapping entries are non-deterministic.
    // - Unchecked: Invariants are assumed but never assert.
    // - Checked: Invariants assumed and asserted. Requires an extra 'client'.
    enum class InvarRule { None, Unchecked, Checked };

    // Specifies the specificity of invariants, if instrumented at all:
    // - Universal: All users have a single invariant, including implicit users.
    // - Singleton: All users, except implicit users, share a single invariant.
    // - RoleBased: An invariant exists for each rule.
    enum class InvarType { Universal, Singleton, RoleBased };

    // Generates invariants for all contracts in _actors, that conform to the
    // choice of _rule and _type. If _infer is true, then the invariants will
    // conform to Seahorn's synthesis format.
    CompInvarGenerator(
        std::shared_ptr<AnalysisStack const> _stack,
        std::list<Actor> const& _actors,
        InvarRule _rule,
        InvarType _type,
        bool _stateful,
        bool _infer
    );

    // Declares are invariants used by the bundle.
    void print_invariants(std::ostream& _stream);

    // Expands and applies interference to all mappings.
    CBlockList apply_interference(NondetSourceRegistry &nd_reg);

    // Expands and checks that interference is closed for all mappings.
    CBlockList check_interference();

private:
    // Represents a field in a mapping.
    struct MapField
    {
        std::list<std::string> path;
        Type const* type;
    };
    using MapFieldList = std::list<MapField>;

    // Records mapping data for invariant instrumentation.
    struct MapData
    {
        size_t id;
        size_t depth;
        CExprPtr path;
        TypeName const* base_type;
        MapFieldList fields;
        std::string display;
    };
    std::vector<MapData> m_maps;

    // Records contract state that must be passed to an invariant.
    struct SharedData
    {
        CExprPtr path;
        Type const* type;
    };
    std::vector<SharedData> m_control_state;

    // Analysis results.
    std::shared_ptr<AnalysisStack const> m_stack;

    // The invariant placement used by the harness.
    InvarRule m_rule;

    // The invariant type used by the harness.
    InvarType m_type;

    // If true, then checked invariants are also inferred.
    bool m_infer;

    // Records all mappings within _maps. The list is computed recursively,
    // interating over each declaration within _contract. This assumes that
    // _decl is a substructure in _contract with path given by _path.
    void analyze_actor(
        bool _stateful,
        CExprPtr _path,
        FlatContract const& _contract,
        std::string _display,
        VariableDeclaration const* _decl
    );

    // Applies the invariants of _map to _data. The application is appended to
    // _block. If _assert is set, then the invariant is asserted, otherwise it
    // is assumed.
    void apply_invariant(
        CBlockList &_block, bool _assert, CExprPtr _data, MapData const& _map
    );

    // Generates the invariant parameters for a mapping with value type _vtype.
    CParams generate_params(MapFieldList const& _fields);

    // Generates the invariant body for a mapping with parameters _params and
    // synthesis placeholder method _infer.
    CBlockList generate_body(std::string _infer, CParams &_params);

    //
    void extract_map_fields(
        MapFieldList &_fields, std::list<std::string> &_path, Type const *_ty
    );
};

// -------------------------------------------------------------------------- //

}
}
}
