/**
 * @date 2019
 * Data and helper functions for generating mappings. This is meant to reduce
 * code duplication.
 */

#include <libsolidity/modelcheck/model/Mapping.h>

#include <libsolidity/modelcheck/analysis/VariableScope.h>
#include <libsolidity/modelcheck/codegen/Literals.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

MapGenerator::MapGenerator(
    Mapping const& _src, size_t _ct, TypeConverter const& _converter
): M_LEN(_ct)
 , M_TYPE(_converter.get_type(_src))
 , M_TYPES(_converter)
 , M_MAP_RECORD(_converter.mapdb().resolve(_src))
 , M_VAL_T(M_TYPES.get_type(*M_MAP_RECORD.value_type))
 , M_TMP(make_shared<CVarDecl>(M_TYPE, "tmp", false))
 , M_ARR(make_shared<CVarDecl>(M_TYPE, "arr", true))
 , M_DAT(make_shared<CVarDecl>(M_VAL_T, "dat"))
{
    m_keys.reserve(M_MAP_RECORD.key_types.size());
    for (auto const* KEY : M_MAP_RECORD.key_types)
    {
        m_keys.push_back(make_shared<CVarDecl>(
            M_TYPES.get_type(*KEY), "key_" + to_string(m_keys.size()))
        );
    }
}

// -------------------------------------------------------------------------- //

CStructDef MapGenerator::declare(bool _forward_declare) const
{
    shared_ptr<CParams> t;
    if (!_forward_declare)
    {
        t = make_shared<CParams>();  

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
    const auto INIT_VAL = M_TYPES.get_init_val(*M_MAP_RECORD.value_type);
    auto fid = make_shared<CVarDecl>(M_TYPE, "Init_0_" + M_MAP_RECORD.name);

    shared_ptr<CBlock> body;
    if (!_forward_declare) body = expand_init(INIT_VAL);

    return CFuncDef(move(fid), {}, move(body));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_nd_initializer(bool _forward_declare) const
{
    const auto NAME = "ND_" + M_MAP_RECORD.name;
    const auto ND_VAL = M_TYPES.get_nd_val(*M_MAP_RECORD.value_type, NAME);
    auto fid = make_shared<CVarDecl>(M_TYPE, NAME);

    shared_ptr<CBlock> body;
    if (!_forward_declare) body = expand_init(ND_VAL);

    return CFuncDef(move(fid), {}, move(body));
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
        if (M_LEN > 0)
        {
            body = make_shared<CBlock>(CBlockList{expand_access(0, "", true)});
        }
        else
        {
            body = make_shared<CBlock>(CBlockList{});
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
        auto const ND_VAL = M_TYPES.get_nd_val(*M_MAP_RECORD.value_type, NAME);
        auto default_rv = make_shared<CReturn>(ND_VAL);

        if (M_LEN > 0)
        {
            body = make_shared<CBlock>(CBlockList{
                expand_access(0, "", false), move(default_rv)
            });
        }
        else
        {
            body = make_shared<CBlock>(CBlockList{move(default_rv)});
        }
    }

    return CFuncDef(move(fid), move(params), move(body));
}

// -------------------------------------------------------------------------- //

shared_ptr<CBlock> MapGenerator::expand_init(CExprPtr const& _init_data) const
{
    CBlockList block;
    block.push_back(M_TMP);
    
    if (M_LEN > 0)
    {
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
    }
    
    block.push_back(make_shared<CReturn>(M_TMP->id()));
    return make_shared<CBlock>(move(block));
}

// -------------------------------------------------------------------------- //

CStmtPtr MapGenerator::expand_access(
    size_t _depth, string const& _suffix, bool _is_writer
) const
{
    if (_depth == M_MAP_RECORD.key_types.size())
    {
        auto const DATA = M_ARR->access("data" + _suffix);
        if (_is_writer)
        {
            return DATA->assign(M_DAT->id())->stmt();
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
            auto const CURR_KEY = M_ARR->access("curr" + SUFFIX)->access("v");
            auto const REQ_KEY = m_keys[_depth]->access("v");

            auto const NEXT = expand_access(_depth + 1, SUFFIX, _is_writer);

            stmt = make_shared<CIf>(make_shared<CBinaryOp>(
                make_shared<CIntLiteral>(i), "==", REQ_KEY
            ), NEXT, stmt);
        }
        return make_shared<CBlock>(CBlockList{stmt});
    }
}

// -------------------------------------------------------------------------- //

string MapGenerator::name_global_key(size_t _id)
{
    return VariableScopeResolver::rewrite(
        "addr_" + to_string(_id), true, VarContext::STRUCT
    );
}

// -------------------------------------------------------------------------- //

MapGenerator::KeyIterator::KeyIterator(
    size_t _width, size_t _depth
): M_WIDTH(_width), M_DEPTH(_depth), m_indices({0})
{
}

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
