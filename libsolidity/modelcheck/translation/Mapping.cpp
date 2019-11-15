/**
 * @date 2019
 * Data and helper functions for generating mappings. This is meant to reduce
 * code duplication.
 */

#include <libsolidity/modelcheck/translation/Mapping.h>

#include <libsolidity/modelcheck/codegen/Literals.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace modelcheck
{

// -------------------------------------------------------------------------- //

const std::string MapGenerator::SET_FIELD = "set";
const std::string MapGenerator::CURR_FIELD = "curr";
const std::string MapGenerator::DATA_FIELD = "data";
const std::string MapGenerator::ND_FIELD = "nd_data";

// -------------------------------------------------------------------------- //

MapGenerator::MapGenerator(
    Mapping const& _src, size_t _ct, TypeConverter const& _converter
)
: M_LEN(_ct),
  M_NAME(_converter.get_name(_src)),
  M_TYPE(_converter.get_type(_src)),
  M_KEY_T(_converter.get_type(_src.keyType())),
  M_VAL_T(_converter.get_type(_src.valueType())),
  M_INIT_KEY(_converter.get_init_val(_src.keyType())),
  M_INIT_VAL(_converter.get_init_val(_src.valueType())),
  M_ND_VAL(_converter.get_nd_val(_src.valueType(), M_NAME + "_val"))
{
}

// -------------------------------------------------------------------------- //

CStructDef MapGenerator::declare(bool _forward_declare) const
{
    shared_ptr<CParams> t;
    if (!_forward_declare)
    {
        t = make_shared<CParams>();
        t->reserve(1 + 3 * M_LEN);
        t->push_back(make_shared<CVarDecl>(M_VAL_T, ND_FIELD));
        for (unsigned int i = 0; i < M_LEN; ++i)
        {
            t->push_back(make_shared<CVarDecl>("uint8_t", field(SET_FIELD, i)));
            t->push_back(make_shared<CVarDecl>(M_KEY_T, field(CURR_FIELD, i)));
            t->push_back(make_shared<CVarDecl>(M_VAL_T, field(DATA_FIELD, i)));
        }
    }
    return CStructDef(M_NAME, move(t));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_zero_initializer(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>(M_TYPE, "Init_0_" + M_NAME);

    shared_ptr<CBlock> body;
    if (!_forward_declare) body = expand_init(M_INIT_VAL);

    return CFuncDef(move(fid), {}, move(body));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_nd_initializer(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>(M_TYPE, "ND_" + M_NAME);

    shared_ptr<CBlock> body;
    if (!_forward_declare) body = expand_init(M_ND_VAL);

    return CFuncDef(move(fid), {}, move(body));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_write(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>("void", "Write_" + M_NAME);

    auto arr = make_shared<CVarDecl>(M_TYPE, "arr", true);
    auto key = make_shared<CVarDecl>(M_KEY_T, "key");
    auto data = make_shared<CVarDecl>(M_VAL_T, "dat");

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        CStmtPtr chain = nullptr;
        for (size_t i = 0; i < M_LEN; ++i)
        {
            auto store = write_field_stmt(*arr, DATA_FIELD, i, data->id());
            chain = expand_iteration(i, *arr, *key, move(store), chain);
        }
        body = make_shared<CBlock>(CBlockList{ move(chain) });
    }

    return CFuncDef(move(fid), CParams{ arr, key, data }, move(body));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_read(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>(M_VAL_T, "Read_" + M_NAME);

    auto arr = make_shared<CVarDecl>(M_TYPE, "arr", true);
    auto key = make_shared<CVarDecl>(M_KEY_T, "key");

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        CStmtPtr chain = nullptr;
        for (size_t i = 0; i < M_LEN; ++i)
        {
            auto rv = make_shared<CReturn>(arr->access(field(DATA_FIELD, i)));
            chain = expand_iteration(i, *arr, *key, move(rv), chain);
        }
        body = make_shared<CBlock>(CBlockList{
            move(chain), make_shared<CReturn>(M_ND_VAL)
        });
    }

    return CFuncDef(move(fid), CParams{ arr, key }, move(body));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_ref(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>(M_VAL_T, "Ref_" + M_NAME, true);

    auto arr = make_shared<CVarDecl>(M_TYPE, "arr", true);
    auto key = make_shared<CVarDecl>(M_KEY_T, "key");

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        CStmtPtr chain = nullptr;
        for (size_t i = 0; i < M_LEN; ++i)
        {
            auto v = make_shared<CReference>(arr->access(field(DATA_FIELD, i)));
            auto rv = make_shared<CReturn>(move(v));
            chain = expand_iteration(i, *arr, *key, move(rv), chain);
        }
        body = make_shared<CBlock>(CBlockList{
            move(chain),
            arr->access(ND_FIELD)->assign(M_ND_VAL)->stmt(),
            make_shared<CReturn>(make_shared<CReference>(arr->access(ND_FIELD)))
        });
    }

    return CFuncDef(move(fid), CParams{ arr, key }, move(body));
}

// -------------------------------------------------------------------------- //

shared_ptr<CBlock> MapGenerator::expand_init(CExprPtr const& _init_data) const
{
    auto const TMP = make_shared<CVarDecl>(M_TYPE, "tmp", false, nullptr);
    CBlockList block;
    block.reserve(2 + 3 * M_LEN + 1);
    block.push_back(TMP);
    block.push_back(TMP->access(ND_FIELD)->assign(M_INIT_VAL)->stmt());
    for (size_t i = 0; i < M_LEN; ++i)
    {
        block.push_back(write_field_stmt(*TMP, SET_FIELD, i, Literals::ZERO));
        block.push_back(write_field_stmt(*TMP, CURR_FIELD, i, M_INIT_KEY));
        block.push_back(write_field_stmt(*TMP, DATA_FIELD, i, _init_data));
    }
    block.push_back(make_shared<CReturn>(TMP->id()));
    return make_shared<CBlock>(move(block));
}

// -------------------------------------------------------------------------- //

CStmtPtr MapGenerator::expand_iteration(
    size_t _i,
    CVarDecl const& _arr,
    CVarDecl const& _key,
    CStmtPtr _exec,
    CStmtPtr _chain
)
{
    auto is_match = make_shared<CBinaryOp>(
        _arr.access(field(CURR_FIELD, _i))->access("v"), "==", _key.access("v")
    );

    _chain = make_shared<CIf>(move(is_match), _exec, move(_chain));
    return make_shared<CIf>(
        make_shared<CBinaryOp>(
            _arr.access(field(SET_FIELD, _i)), "==", Literals::ZERO
        ),
        make_shared<CBlock>(CBlockList{
            write_field_stmt(_arr, SET_FIELD, _i, Literals::ONE),
            write_field_stmt(_arr, CURR_FIELD, _i, _key.id()),
            move(_exec)
        }),
        move(_chain)
    );
}

// -------------------------------------------------------------------------- //

string MapGenerator::field(string const& _base, size_t _i)
{
    return _base + to_string(_i);
}

// -------------------------------------------------------------------------- //

CStmtPtr MapGenerator::write_field_stmt(
    CData const& _base, string const& _field, size_t _i, CExprPtr _data
)
{
    return _base.access(field(_field, _i))->assign(move(_data))->stmt();
}

// -------------------------------------------------------------------------- //

}
}
}
