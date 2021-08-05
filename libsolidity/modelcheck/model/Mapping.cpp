#include <libsolidity/modelcheck/model/Mapping.h>

#include <libsolidity/modelcheck/analysis/TypeAnalyzer.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/model/NondetSourceRegistry.h>
#include <libsolidity/modelcheck/utils/KeyIterator.h>
#include <libsolidity/modelcheck/utils/Function.h>
#include <libsolidity/modelcheck/utils/LibVerify.h>
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

MapGenerator::MapGenerator(
    Mapping const& _src,
    bool _keep_sum,
    size_t _ct,
    TypeAnalyzer const& _converter
): M_LEN(_ct)
 , M_KEEP_SUM(_keep_sum && has_simple_type(*M_MAP_RECORD->value_type))
 , M_TYPE(_converter.get_type(_src))
 , M_CONVERTER(_converter)
 , M_MAP_RECORD(_converter.map_db().try_resolve(_src))
 , M_VAL_T(_converter.get_type(*M_MAP_RECORD->value_type))
 , M_TMP(make_shared<CVarDecl>(M_TYPE, "tmp", false))
 , M_ARR(make_shared<CVarDecl>(M_TYPE, "arr", true))
 , M_DAT(make_shared<CVarDecl>(M_VAL_T, "dat"))
{
    m_keys.reserve(M_MAP_RECORD->key_types.size());
    for (auto const* KEY : M_MAP_RECORD->key_types)
    {
        m_keys.push_back(make_shared<CVarDecl>(
            M_CONVERTER.get_type(*KEY), "key_" + to_string(m_keys.size()))
        );
    }

    if (M_LEN == 0)
    {
        throw runtime_error("Mapping requires at least one entry.");
    }
}

// -------------------------------------------------------------------------- //

CStructDef MapGenerator::declare(bool _forward_declare) const
{
    shared_ptr<CParams> t;
    if (!_forward_declare)
    {
        t = make_shared<CParams>();

        if (M_KEEP_SUM)
        {
            t->push_back(make_shared<CVarDecl>(M_VAL_T, "sum"));
        }

        if (M_LEN > 0)
        {
            KeyIterator indices(M_LEN, M_MAP_RECORD->key_types.size());
            do
            {
                if (indices.is_full())
                {
                    string const SUFFIX = indices.suffix();

                    t->push_back(
                        make_shared<CVarDecl>(M_VAL_T, "data" + SUFFIX)
                    );
                }
            } while (indices.next());
        }
    }
    return CStructDef(M_MAP_RECORD->name, move(t));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_zero_initializer(bool _forward_declare) const
{
    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        CBlockList block;
        block.push_back(M_TMP);

        auto init_val = M_CONVERTER.get_init_val(*M_MAP_RECORD->value_type);

        if (M_KEEP_SUM)
        {
            block.push_back(M_TMP->access("sum")->assign(init_val)->stmt());
        }
        
        KeyIterator indices(M_LEN, M_MAP_RECORD->key_types.size());
        do
        {
            if (indices.is_full())
            {
                string const SUFFIX = indices.suffix();

                block.push_back(
                    M_TMP->access("data" + SUFFIX)->assign(init_val)->stmt()
                );
            }
        } while (indices.next());
        
        block.push_back(make_shared<CReturn>(M_TMP->id()));
        body = make_shared<CBlock>(move(block));
    }

    auto id = InitFunction(*M_MAP_RECORD).default_id();
    return CFuncDef(move(id), {}, move(body));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_write(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>("void", "Write_" + M_MAP_RECORD->name);

    CParams params;
    params.push_back(M_ARR);
    params.insert(params.end(), m_keys.begin(), m_keys.end());
    params.push_back(M_DAT);

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        CBlockList block{expand_access(0, "", true, M_KEEP_SUM)};

        if (M_KEEP_SUM)
        {
            block.push_back(make_shared<CBinaryOp>(
                M_ARR->access("sum")->access("v"),
                "+=",
                M_DAT->id()->access("v")
            )->stmt());
        }

        body = make_shared<CBlock>(block);
    }

    return CFuncDef(move(fid), move(params), move(body));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_read(bool _forward_declare) const
{
    auto const NAME = "Read_" + M_MAP_RECORD->name;
    auto fid = make_shared<CVarDecl>(M_VAL_T, NAME);

    CParams params;
    params.push_back(M_ARR);
    params.insert(params.end(), m_keys.begin(), m_keys.end());

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        auto default_val = M_CONVERTER.get_init_val(*M_MAP_RECORD->value_type);

        body = make_shared<CBlock>(CBlockList{
            expand_access(0, "", false, false),
            make_shared<CReturn>(move(default_val))
        });
    }

    return CFuncDef(move(fid), move(params), move(body));
}

// -------------------------------------------------------------------------- //

CStmtPtr MapGenerator::expand_access(
    size_t _depth, string const& _suffix, bool _is_writer, bool _maintain_sum
) const
{
    if (_depth == M_MAP_RECORD->key_types.size())
    {
        auto const DATA = M_ARR->access("data" + _suffix);
        if (_is_writer)
        {
            CBlockList block;
            if (_maintain_sum)
            {
                block.push_back(make_shared<CBinaryOp>(
                    M_ARR->access("sum")->access("v"), "-=", DATA->access("v")
                )->stmt());
            }
            block.push_back(DATA->assign(M_DAT->id())->stmt());
            return make_shared<CBlock>(move(block));
        }
        else
        {
            return make_shared<CReturn>(DATA);
        }
    }
    else
    {
        shared_ptr<CIf> stmt;
        for (size_t i = 0; i < M_LEN; ++i)
        {
            auto const SUFFIX = _suffix + "_" + to_string(i);
            auto const REQ_KEY = m_keys[_depth]->access("v");

            auto key = make_shared<CIntLiteral>(i);
            auto cond = make_shared<CBinaryOp>(move(key), "==", REQ_KEY);
            auto next = expand_access(
                _depth + 1, SUFFIX, _is_writer, _maintain_sum
            );

            stmt = make_shared<CIf>(move(cond), move(next), move(stmt));
        }

        CBlockList stmts;
        if (_depth == 0)
        {
            for (size_t i = 0; i < M_MAP_RECORD->key_types.size(); ++i)
            {
                auto const REQ_KEY = m_keys[i]->access("v");
                auto key = make_shared<CIntLiteral>(M_LEN);
                auto cond = make_shared<CBinaryOp>(move(key), ">=", REQ_KEY);

                ostringstream err_msg;
                err_msg << "Model failure, mapping key out of bounds.";

                LibVerify::add_assert(stmts, cond, err_msg.str());
            }
        }
        stmts.push_back(stmt);
        return make_shared<CBlock>(move(stmts));
    }
}

// -------------------------------------------------------------------------- //

}
}
}
