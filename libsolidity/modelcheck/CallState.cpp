/**
 * @date 2019
 * First-pass visitor for generating the CallState of Solidity in C models,
 * which consist of the struct of CallState.
 */

#include <libsolidity/modelcheck/CallState.h>
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

CallState::CallState(
    ASTNode const& _ast,
	TypeConverter const& _converter,
    bool _forward_declare
): m_ast(_ast), m_converter(_converter), m_forward_declare(_forward_declare)
{
}

void CallState::print(ostream& _stream)
{
	ScopedSwap<ostream*> stream_swap(m_ostream, &_stream);
    m_ast.accept(*this);
}
// -------------------------------------------------------------------------- //

void CallState::endVisit(ContractDefinition const& _node)
{
  auto translation = m_converter.translate(_node);

  (*m_ostream) << "struct CallState";
  if (!m_forward_declare)
  {
      (*m_ostream) << "{";
      (*m_ostream) << "int sender;";
      (*m_ostream) << "unsigned int value;";
      (*m_ostream) << "unsigned int blocknum;";
      (*m_ostream) << "}";
  }
  (*m_ostream) << ";";
}

}
}
}
