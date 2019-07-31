/**
 * @date 2019
 * First-pass visitor for converting Solidity methods into functions in C.
 */

#include <libsolidity/modelcheck/MainFunction_3.h>
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

MainFunction_3::MainFunction_3(
    ASTNode const& _ast,
	TypeConverter const& _converter,
    bool _forward_declare
): m_ast(_ast), m_converter(_converter), m_forward_declare(_forward_declare)
{
}

void MainFunction_3::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    m_ast.accept(*this);
}
// -------------------------------------------------------------------------- //


void MainFunction_3::endVisit(ContractDefinition const& _node)
{
  auto translation = m_converter.translate(_node);
  if (!m_forward_declare)
  {
      (*m_ostream) << "default: assume(false);";
      (*m_ostream) << "}";
      (*m_ostream) << "nextGS.sender = nd();";
      (*m_ostream) << "nextGS.value = nd();";
      (*m_ostream) << "nextGS.blocknum = nd();";
      (*m_ostream) << "assume(nextGS.blocknum >= globalstate.blocknum);";
      (*m_ostream) << "globalstate = nextGS;";
      (*m_ostream) << "}";
      (*m_ostream) << "}";
  }
}
// -------------------------------------------------------------------------- //


/*//step-5//each case;declare each functions;break;*/
bool MainFunction_3::visit(FunctionDefinition const& _node)
{
    auto translation = m_converter.translate(_node);


      if(!_node.isConstructor()&&(_node.isPublic()))
      {
        (*m_ostream) << "case " << i << ": "<< translation.name;
        bool is_mutable = _node.stateMutability() != StateMutability::Pure;
      print_args(_node.parameters(), is_mutable ? _node.scope() : nullptr);

      i++;
    }

    return false;
}

void MainFunction_3::print_args(
    vector<ASTPointer<VariableDeclaration>> const& _args, ASTNode const* _scope)
{
  (*m_ostream) << "(";

  auto contract_scope = dynamic_cast<ContractDefinition const*>(_scope);
  if (contract_scope)
  {
      auto type = m_converter.translate(*contract_scope).type;
      (*m_ostream) << "&contract, &globalstate";
  }

  for (unsigned int idx = 0; idx < _args.size(); ++idx)
  {
      if (contract_scope || idx > 0)
      {
          (*m_ostream) << ",";
      }

      auto const& arg = *_args[idx];
      (*m_ostream) << i << "_" << arg.name();
  }

  (*m_ostream) << "); break;";

}



}
}
}
