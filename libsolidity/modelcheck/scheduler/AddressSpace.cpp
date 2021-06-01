#include <libsolidity/modelcheck/scheduler/AddressSpace.h>

#include <libsolidity/modelcheck/analysis/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>
#include <libsolidity/modelcheck/utils/AbstractAddressDomain.h>
#include <libsolidity/modelcheck/utils/Contract.h>
#include <libsolidity/modelcheck/utils/LibVerify.h>

#include <list>
#include <memory>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

AddressSpace::AddressSpace(
    shared_ptr<PTGBuilder const> _address_data,
    shared_ptr<NondetSourceRegistry> _nd_reg
): MAX_ADDR(_address_data->implicit_count())
 , m_address_data(_address_data)
 , m_nd_reg(_nd_reg)
{
}

// -------------------------------------------------------------------------- //

void AddressSpace::map_constants(CBlockList & _block) const
{
    list<shared_ptr<CIdentifier>> used_so_far;
    if (m_address_data->literals().size() > 1)
    {
        LibVerify::log(_block, "[Handling constants]");
    }
    for (auto lit : m_address_data->literals())
    {
        auto const NAME = AbstractAddressDomain::literal_name(lit);
        auto decl = make_shared<CIdentifier>(NAME, false);

        if (lit == 0)
        {
            _block.push_back(decl->assign(Literals::ZERO)->stmt());
        }
        else
        {
            auto range = m_nd_reg->range(MIN_ADDR, MAX_ADDR, NAME);
            _block.push_back(decl->assign(move(range))->stmt());

            for (auto otr : used_so_far)
            {
                // TODO: bad for fuzzing, though used_so_far is often small.
                _block.push_back(make_shared<CBinaryOp>(
                    decl, "!=", otr
                )->stmt());
            }

            used_so_far.push_back(decl);
        }
    }
}

// -------------------------------------------------------------------------- //

}
}
}
