#include <libsolidity/modelcheck/model/Mapping.h>

#include <libsolidity/modelcheck/analysis/TypeNames.h>
#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>
#include <libsolidity/modelcheck/utils/Function.h>

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
 , M_KEEP_SUM(_keep_sum)
 , M_TYPE(_converter.get_type(_src))
 , M_CONVERTER(_converter)
 , M_MAP_RECORD(_converter.map_db().resolve(_src))
 , M_VAL_T(_converter.get_type(*M_MAP_RECORD.value_type))
 , M_TMP(make_shared<CVarDecl>(M_TYPE, "tmp", false))
 , M_ARR(make_shared<CVarDecl>(M_TYPE, "arr", true))
 , M_DAT(make_shared<CVarDecl>(M_VAL_T, "dat"))
{
    m_keys.reserve(M_MAP_RECORD.key_types.size());
    for (auto const* KEY : M_MAP_RECORD.key_types)
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
            KeyIterator indices(M_LEN, M_MAP_RECORD.key_types.size());
            do
            {
                string suffix = indices.suffix();

                if (indices.is_full())
                {
                    t->push_back(make_shared<CVarDecl>(
                        M_VAL_T, "data" + suffix)
                    );
                }
            } while (indices.next());
        }
    }
    return CStructDef(M_MAP_RECORD.name, move(t));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_zero_initializer(bool _forward_declare) const
{
    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        auto init_val = M_CONVERTER.get_init_val(*M_MAP_RECORD.value_type);
        body = expand_init(move(init_val));
    }

    auto id = InitFunction(M_MAP_RECORD).default_id();
    return CFuncDef(move(id), {}, move(body));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_write(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>("void", "Write_" + M_MAP_RECORD.name);

    CParams params;
    params.push_back(M_ARR);
    params.insert(params.end(), m_keys.begin(), m_keys.end());
    params.push_back(M_DAT);

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        body = expand_update(M_KEEP_SUM);
    }

    return CFuncDef(move(fid), move(params), move(body));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_set(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>("void", "Set_" + M_MAP_RECORD.name);

    CParams params;
    params.push_back(M_ARR);
    params.insert(params.end(), m_keys.begin(), m_keys.end());
    params.push_back(M_DAT);

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        if (M_KEEP_SUM)
        {
            body = expand_update(false);
        }
        else
        {
            CFuncCallBuilder write_call("Write_" + M_MAP_RECORD.name);
            write_call.push(M_ARR->id());
            for (auto key : m_keys) write_call.push(key->id());
            write_call.push(M_DAT->id());

            body = make_shared<CBlock>(CBlockList{
                write_call.merge_and_pop_stmt()
            });
        }
    }

    return CFuncDef(move(fid), move(params), move(body));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_read(bool _forward_declare) const
{
    auto const NAME = "Read_" + M_MAP_RECORD.name;
    auto fid = make_shared<CVarDecl>(M_VAL_T, NAME);

    CParams params;
    params.push_back(M_ARR);
    params.insert(params.end(), m_keys.begin(), m_keys.end());

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        auto default_val = M_CONVERTER.get_init_val(*M_MAP_RECORD.value_type);

        CFuncCallBuilder on_fail("sol_assert");
        on_fail.push(Literals::ZERO);
        on_fail.push(Literals::ZERO);

        body = make_shared<CBlock>(CBlockList{
            expand_access(0, "", false, false),
            on_fail.merge_and_pop_stmt(),
            make_shared<CReturn>(move(default_val))
        });
    }

    return CFuncDef(move(fid), move(params), move(body));
}

// -------------------------------------------------------------------------- //

shared_ptr<CBlock> MapGenerator::expand_init(CExprPtr _init_data) const
{
    CBlockList block;
    block.push_back(M_TMP);

    if (M_KEEP_SUM)
    {
        block.push_back(M_TMP->access("sum")->assign(_init_data)->stmt());
    }
    
    KeyIterator indices(M_LEN, M_MAP_RECORD.key_types.size());
    do
    {
        string suffix = indices.suffix();

        if (indices.is_full())
        {
            block.push_back(
                M_TMP->access("data" + suffix)->assign(_init_data)->stmt()
            );
        }
    } while (indices.next());
    
    block.push_back(make_shared<CReturn>(M_TMP->id()));
    return make_shared<CBlock>(move(block));
}

// -------------------------------------------------------------------------- //

shared_ptr<CBlock> MapGenerator::expand_update(bool _maintain_sum) const
{
    CBlockList block{expand_access(0, "", true, _maintain_sum)};

    if (_maintain_sum)
    {
        block.push_back(make_shared<CBinaryOp>(
            M_ARR->access("sum")->access("v"),
            "+=",
            M_DAT->id()->access("v")
        )->stmt());
    }

    return make_shared<CBlock>(block);
}

// -------------------------------------------------------------------------- //

CStmtPtr MapGenerator::expand_access(
    size_t _depth, string const& _suffix, bool _is_writer, bool _maintain_sum
) const
{
    if (_depth == M_MAP_RECORD.key_types.size())
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
        return make_shared<CBlock>(CBlockList{stmt});
    }
}

// -------------------------------------------------------------------------- //

MapGenerator::KeyIterator::KeyIterator(
    size_t _width, size_t _depth
): M_WIDTH(_width), M_DEPTH(_depth), m_indices({0}) { }

// -------------------------------------------------------------------------- //

string MapGenerator::KeyIterator::suffix() const
{
    string suffix;
    for (auto idx : m_indices) suffix += "_" + to_string(idx);
    return suffix;
}

// -------------------------------------------------------------------------- //

bool MapGenerator::KeyIterator::is_full() const
{
    return (m_indices.size() == M_DEPTH);
}

// -------------------------------------------------------------------------- //

size_t MapGenerator::KeyIterator::top() const
{
    return m_indices.back();
}

// -------------------------------------------------------------------------- //

size_t MapGenerator::KeyIterator::size() const
{
    return m_indices.size();
}

// -------------------------------------------------------------------------- //

bool MapGenerator::KeyIterator::next()
{
    if (!is_full())
    {
        m_indices.push_back(0);
    }
    else
    {
        ++m_indices.back();
        while (m_indices.back() == M_WIDTH)
        {
            m_indices.pop_back();
            if (m_indices.empty()) break;
            ++m_indices.back();
        }
    }

    return !m_indices.empty();
}

// -------------------------------------------------------------------------- //

}
}
}
