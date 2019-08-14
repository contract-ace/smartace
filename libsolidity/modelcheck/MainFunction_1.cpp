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
	TypeConverter const& _converter,
    bool _forward_declare
): m_ast(_ast), m_converter(_converter), m_forward_declare(_forward_declare)
{
}

void MainFunction_1::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    m_ast.accept(*this);
}
// -------------------------------------------------------------------------- //

void MainFunction_1::endVisit(ContractDefinition const& _node)
{
    auto translation = m_converter.translate(_node);
    if (!m_forward_declare)
    {
      /*//step-2//struct Type contract*/
      (*m_ostream) << translation.type << " contract;";
      (*m_ostream) << "struct CallState globalstate;";
      /*//step-3//Init_Type(&contract,&globalstate);*/
      (*m_ostream) << "contract = Init_" << translation.name << "(&contract,&globalstate);";
      (*m_ostream) << "struct CallState nextGS;";
      (*m_ostream) << "while (nd())";
      (*m_ostream) << "{";
  }
}
// -------------------------------------------------------------------------- //


/*//step-1//declare the input parameters of each functions*/
bool MainFunction_1::visit(FunctionDefinition const& _node)
{
    auto translation = m_converter.translate(_node);
    if (!m_forward_declare && !_node.isConstructor())
    {
      print_args(_node.parameters());
      i++;
    }
    return false;
}

void MainFunction_1::print_args(
    vector<ASTPointer<VariableDeclaration>> const& _args)
{
    for (unsigned int idx = 0; idx < _args.size(); ++idx)
    {
        auto const& arg = *_args[idx];
        (*m_ostream) << m_converter.translate(arg).type << " " << i << "_" << arg.name() << ";";
    }

}


bool MainFunction_1::visit(ContractDefinition const& _node)
{
  if (access == false)
  {
    access = true;
    auto translation = m_converter.translate(_node);

    if (!m_forward_declare)
    {
      (*m_ostream) << "int main(void)";
        (*m_ostream) << "{";
      }
    return true;
  }
  else
  {
    throw runtime_error("Multiple contracts not yet supported."); 
    return false;
  }
}



}
}
}
