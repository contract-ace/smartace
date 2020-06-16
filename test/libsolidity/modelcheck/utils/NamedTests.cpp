/**
 * Specific tests for libsolidity/modelcheck/utils/Named.
 * 
 * @date 2020
 */

#include <libsolidity/modelcheck/utils/Named.h>

#include <boost/test/unit_test.hpp>

#include <libsolidity/ast/AST.h>

using namespace std;
using namespace langutil;

namespace dev
{
namespace solidity
{
namespace modelcheck
{
namespace test
{


// -------------------------------------------------------------------------- //

BOOST_AUTO_TEST_SUITE(Utils_NamedTests)

BOOST_AUTO_TEST_CASE(named)
{
    auto name = make_shared<string>("myname");
    vector<ASTPointer<VariableDeclaration>> fields{};
    StructDefinition decl(SourceLocation(), name, fields);

    Named named(decl);
    BOOST_CHECK_EQUAL(named.name(), *name);
}

BOOST_AUTO_TEST_SUITE_END();

// -------------------------------------------------------------------------- //

}
}
}
}
