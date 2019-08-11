/**
 * @date 2019
 * First-pass visitor for generating Solidity the second part of main function,
 * which consist of initializing the input parameters with nd() in main function.
 */

#include <libsolidity/modelcheck/MainFunction_2.h>
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

MainFunction_2::MainFunction_2(
    ASTNode const& _ast,
	TypeConverter const& _converter,
    bool _forward_declare
): m_ast(_ast), m_converter(_converter), m_forward_declare(_forward_declare)
{
}

void MainFunction_2::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    m_ast.accept(*this);
}
// -------------------------------------------------------------------------- //

void MainFunction_2::endVisit(ContractDefinition const& _node)
{
  auto translation = m_converter.translate(_node);

  if (!m_forward_declare)
  {
      (*m_ostream) << "switch (nd())";
      (*m_ostream) << "{";
  }
}
// -------------------------------------------------------------------------- //


/*//step-4//initialize each input parameters with nd()*/
bool MainFunction_2::visit(FunctionDefinition const& _node)
{
    auto translation = m_converter.translate(_node);
    if(!_node.isConstructor())
    {
    print_args(_node.parameters());

    i++;
  }
    return false;
}

void MainFunction_2::print_args(
    vector<ASTPointer<VariableDeclaration>> const& _args)
{
    for (unsigned int idx = 0; idx < _args.size(); ++idx)
    {
        auto const& arg = *_args[idx];
        (*m_ostream) << i << "_" << arg.name() << " = nd();";
    }

}



}
}
}
