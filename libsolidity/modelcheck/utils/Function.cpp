#include <libsolidity/modelcheck/utils/Function.h>

#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/utils/Types.h>

#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

bool collid(FunctionDefinition const& _f1, FunctionDefinition const& _f2)
{
    // The names must match.
    if (_f1.name() != _f2.name()) return false;

    // The number of arguments must match.
    auto const& params_1 = _f1.parameters();
    auto const& params_2 = _f2.parameters();
    if (params_1.size() != params_2.size()) return false;

    // The parameter types must match.
    for (size_t i = 0; i < params_1.size(); ++i)
    {
        auto const& id_1 = params_1[i]->type()->identifier();
        auto const& id_2 = params_2[i]->type()->identifier();
        if (id_1 != id_2) return false;
    }
    return true;
}

// -------------------------------------------------------------------------- //

string const InitFunction::INIT_VAR = "dest";
string const InitFunction::PREFIX = "Init_";
string const InitFunction::DEFAULT_PREFIX = "ZeroInit_";
string const InitFunction::ND_PREFIX = "ND_";

string InitFunction::specialize_name(
    TypeAnalyzer const& _converter,
    ContractDefinition const& _base,
    ContractDefinition const& _derived
)
{
    string name = _converter.get_name(_base);
    if (&_base != &_derived)
    {
        name += "_For_" + _converter.get_name(_derived);
    }
    return name;
}

InitFunction::InitFunction(
    TypeAnalyzer const& _converter,
    ContractDefinition const& _base,
    ContractDefinition const& _derived
): InitFunction(specialize_name(_converter, _base, _derived), "void")
{
}

InitFunction::InitFunction(
    TypeAnalyzer const& _converter, ASTNode const& _node
): InitFunction(_converter.get_name(_node), _converter.get_type(_node))
{
}

InitFunction::InitFunction(MapDeflate::FlatMap const& _mapping)
 : InitFunction(_mapping.name, "struct " + _mapping.name)
{
}

InitFunction::InitFunction(string _type)
 : InitFunction(_type, _type)
{
}

string InitFunction::name() const
{
    return M_NAME;
}

CFuncCallBuilder InitFunction::call_builder() const
{
    return CFuncCallBuilder(call_name());
}

shared_ptr<CVarDecl> InitFunction::call_id() const
{
    return make_id(call_name());
}

string InitFunction::call_name() const
{
    return PREFIX + M_NAME;
}

CExprPtr InitFunction::defaulted() const
{
    return make_shared<CFuncCall>(default_name(), CArgList{});
}

shared_ptr<CVarDecl> InitFunction::default_id() const
{
    return make_id(default_name());
}

string InitFunction::default_name() const
{
    return DEFAULT_PREFIX + M_NAME;
}

CExprPtr InitFunction::nd() const
{
    return make_shared<CFuncCall>(nd_name(), CArgList{});
}

shared_ptr<CVarDecl> InitFunction::nd_id() const
{
    return make_id(nd_name());
}

string InitFunction::nd_name() const
{
    return ND_PREFIX + M_NAME;
}

CExprPtr InitFunction::wrap(Type const& _type, CExprPtr _expr)
{
    if (is_wrapped_type(_type))
    {
        string const WRAP = PREFIX + TypeAnalyzer::get_simple_ctype(_type);
        return make_shared<CFuncCall>( WRAP, CArgList{ move(_expr) } );
    }
    return _expr;
}

InitFunction::InitFunction(string _name, string _type)
 : M_NAME(move(_name)), M_TYPE(move(_type))
{
}

shared_ptr<CVarDecl> InitFunction::make_id(string _name) const
{
    return make_shared<CVarDecl>(M_TYPE, move(_name));
}

// -------------------------------------------------------------------------- //

FunctionSpecialization::FunctionSpecialization(
    FunctionDefinition const& _def
): FunctionSpecialization(_def, get_scope(_def))
{
}

FunctionSpecialization::FunctionSpecialization(
    FunctionDefinition const& _def, ContractDefinition const& _for
): M_CALL(&_def), M_SRC(&get_scope(_def)), M_USER(&_for)
{
}

unique_ptr<FunctionSpecialization> FunctionSpecialization::super() const
{
    if (auto superfunc = func().annotation().superFunction)
    {
        if (superfunc->isImplemented())
        {
            return make_unique<FunctionSpecialization>(*superfunc, use_by());
        }
    }
    return nullptr;
}

string FunctionSpecialization::name(size_t _depth) const
{
    ostringstream oss;

    // Prefixed by name of source contract.
    oss << escape_decl_name(*M_SRC) << "_";

    // Determines the type of method.
    bool is_method = false;
    if (M_CALL->isConstructor())
    {
        oss << "Constructor";
    }
    else if (M_CALL->isFallback())
    {
        oss << "Fallback";
    }
    else
    {
        oss << "Method";
        is_method = true;
    }
    
    // Appends the modifier index if this is not the entry method.
    if (_depth > 0) oss << "_" << _depth;

    // Adds an override specifier, for when M_CALL is specialized for M_USER.
    if (M_SRC != M_USER)
    {
        oss << "_For_" << escape_decl_name(*M_USER);
    }

    // Adds method name if  this is a named method.
    if (is_method)
    {
        oss << "_" << escape_decl_name(*M_CALL);
    }

    return oss.str();
}

ContractDefinition const& FunctionSpecialization::source() const
{
    return (*M_SRC);
}

ContractDefinition const& FunctionSpecialization::use_by() const
{
    return (*M_USER);
}

FunctionDefinition const& FunctionSpecialization::func() const
{
    return (*M_CALL);
}

ContractDefinition const&
    FunctionSpecialization::get_scope(FunctionDefinition const& _func)
{
    auto scope = dynamic_cast<ContractDefinition const*>(_func.scope());
    if (!scope)
    {
        throw runtime_error("Detected FunctionDefinition without scope.");
    }
    return *scope;
}

// -------------------------------------------------------------------------- //

}
}
}
