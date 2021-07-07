/**
 * Used to generate compositional invariants. All invariant options are
 * implemented at this level.
 * 
 * @date 2021
 */

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/modelcheck/codegen/Details.h>
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

class ActorModel;
class FlatContract;
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

    // Setting object for invariant generation.
    struct Settings
    {
        // Specifies how invariant should be placed.
        InvarRule rule = InvarRule::None;

        // Specifies the specificity of the invariants.
        InvarType type = InvarType::Universal;

        // If true, then control state variables are passed to the invariant.
        bool stateful = false;

        // If true, then an invariant synthesis problem is generated.
        bool inferred = false;
    };

    // Generates invariants for all contracts in _actors, that conform to the
    // choice of _settings.
    CompInvarGenerator(
        std::shared_ptr<AnalysisStack const> _stack,
        ActorModel const& _actors,
        Settings _settings
    );

    // Declares are invariants used by the bundle.
    void print_invariants(std::ostream& _stream);

    // Expands and applies interference to all mappings.
    CBlockList apply_interference(NondetSourceRegistry &_nd_reg);

    // Expands and checks that interference is closed for all mappings.
    CBlockList check_interference(NondetSourceRegistry &_nd_reg);

private:
    // Represents a field in a mapping.
    struct MapField
    {
        std::list<std::string> path;
        Type const* type;
    };
    using MapFieldList = std::vector<MapField>;

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

    // Invariant settings.
    Settings m_settings;

    // Role list.
    std::list<std::shared_ptr<CMemberAccess>> m_roles;

    // Records all mappings within _maps. The list is computed recursively,
    // interating over each declaration within _contract. This assumes that
    // _decl is a substructure in _contract with path given by _path.
    void analyze_actor(
        CExprPtr _path,
        FlatContract const& _contract,
        std::string _display,
        VariableDeclaration const* _decl
    );

    // Helper method to iterate over all indices of all mappings. For each
    // entry, the provided function is applied to the mapping entry and the
    // index summary. Note that literal users are filtered out.
    using MapVisitor = std::function<void(MapData const&, KeyIterator const&)>;
    void expand_maps(MapVisitor _f);
    void expand_map(MapData const&_map, MapVisitor _f);

    // Helper method to guard a statement with a role check. The statement is
    // applied if at least one index does not belong to an implict user, and
    // does not equate with any role.
    CStmtPtr guard(CStmtPtr _inst, std::vector<size_t> const& _indices) const;

    // Applies the invariants of _map to _data. The application is appended to
    // _block. If _assert is set, then the invariant is asserted, otherwise it
    // is assumed.
    void apply_invariant(
        CBlockList &_block,
        bool _assert,
        MapData const& _map,
        std::vector<CExprPtr> const& _values,
        std::vector<size_t> const& _indices
    ) const;

    // Generates the invariant parameters for a mapping with value type _vtype.
    CParams make_params(MapFieldList const& _fields) const;

    // Generates the invariant body for a mapping with parameters _params and
    // synthesis placeholder method _infer.
    CBlockList make_body(
        std::string const &_infer, CParams const& _params
    ) const;

    // Collects the path to each primitive value stored by _ty within _fields.
    // It is assumed that _path is the path from the base type of a mapping, to
    // a substructure of type _ty.
    void extract_map_fields(
        MapFieldList &_fields, std::list<std::string> &_path, Type const *_ty
    );
};

// -------------------------------------------------------------------------- //

}
}
}
