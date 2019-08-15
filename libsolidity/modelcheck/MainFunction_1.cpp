/**
 * @date 2019
 * First-pass visitor for generating Solidity the first part of main function,
 * which consist of the decalaration of contract, globalstate, nextGS
 * and every input parameters of functions in main function.
 */

#include <libsolidity/modelcheck/MainFunction_1.h>
#include <libsolidity/modelcheck/Utility.h>
#include <sstream>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

MainFunction_1::MainFunction_1(
    ASTNode const& _ast,
    TypeConverter const& _converter
): m_ast(_ast), m_converter(_converter)
{
}

// -------------------------------------------------------------------------- //

void MainFunction_1::print(ostream& _stream)
{
    ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    m_i = 0;
    m_access = false;
    m_ast.accept(*this);
}
// -------------------------------------------------------------------------- //

void MainFunction_1::endVisit(ContractDefinition const& _node)
{
    /*//step-2//struct Type contract*/
    (*m_ostream) << m_converter.get_type(_node) << " contract;";
    (*m_ostream) << "struct CallState globalstate;";
    /*//step-3//Init_Type(&contract,&globalstate);*/
    (*m_ostream) << "contract = Init_"
                 << m_converter.get_name(_node)
                 << "(&contract,&globalstate);";
    (*m_ostream) << "struct CallState nextGS;";
    (*m_ostream) << "while (nd())";
    (*m_ostream) << "{";
}

/*//step-1//declare the input parameters of each functions*/
bool MainFunction_1::visit(FunctionDefinition const& _node)
{
    (void) _node;
    print_args(_node.parameters());
    m_i++;
    return false;
}

bool MainFunction_1::visit(ContractDefinition const& _node)
{
    (void) _node;
    if (m_access == false)
    {
        m_access = true;
        (*m_ostream) << "int main(void)";
        (*m_ostream) << "{";
        return true;
    }
    else
    {
        throw runtime_error("Multiple contracts not yet supported."); 
    }
}

// -------------------------------------------------------------------------- //

void MainFunction_1::print_args(
    vector<ASTPointer<VariableDeclaration>> const& _args
)
{
    for (unsigned int idx = 0; idx < _args.size(); ++idx)
    {
        auto const& arg = *_args[idx];
        (*m_ostream) << m_converter.get_type(arg)
                     << " " << m_i << "_" << arg.name() << ";";
    }

}

// -------------------------------------------------------------------------- //

}
}
}
