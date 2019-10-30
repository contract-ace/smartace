/**
 * @date 2019
 * Data and helper functions for generating mappings. This is meant to reduce
 * code duplication.
 */

#include <libsolidity/modelcheck/Mapping.h>

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

shared_ptr<CIntLiteral> const MapGenerator::TRUE = make_shared<CIntLiteral>(1);
shared_ptr<CIntLiteral> const MapGenerator::FALSE = make_shared<CIntLiteral>(0);

// -------------------------------------------------------------------------- //

MapGenerator::MapGenerator(
    Mapping const& _src, int _ct, TypeConverter const& _converter
)
: M_LEN(_ct),
  M_NAME(_converter.get_name(_src)),
  M_TYPE(_converter.get_type(_src)),
  M_KEY_TYPE(_converter.get_type(_src.keyType())),
  M_VAL_TYPE(_converter.get_type(_src.valueType())),
  M_INIT_KEY(_converter.get_init_val(_src.keyType())),
  M_INIT_VAL(_converter.get_init_val(_src.valueType())),
  M_ND_KEY(_converter.get_nd_val(_src.keyType(), M_NAME + "_key")),
  M_ND_VAL(_converter.get_nd_val(_src.valueType(), M_NAME + "_val")),
  M_LEN_LITERAL(make_shared<CIntLiteral>(M_LEN))
{
}

// -------------------------------------------------------------------------- //

CStructDef MapGenerator::declare(bool _forward_declare) const
{
    shared_ptr<CParams> fields;
    if (!_forward_declare)
    {
        fields = make_shared<CParams>(CParams{
            make_shared<CVarDecl>("uint8_t", SET_FIELD, M_LEN),
            make_shared<CVarDecl>(M_KEY_TYPE, CURR_FIELD, M_LEN),
            make_shared<CVarDecl>(M_VAL_TYPE, DATA_FIELD, M_LEN),
            make_shared<CVarDecl>(M_VAL_TYPE, ND_FIELD)
        });
    }
    return CStructDef(M_NAME, move(fields));
}

// -------------------------------------------------------------------------- //

CFuncDef MapGenerator::declare_zero_initializer(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>(M_TYPE, "Init_0_" + M_NAME);

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        auto const TMP = make_tmp();
        body = make_shared<CBlock>(CBlockList{
            TMP,
            expand_loop_template(make_shared<CBlock>(CBlockList{
                read_tmp_field(SET_FIELD)->assign(FALSE)->stmt(),
                read_tmp_field(CURR_FIELD)->assign(M_INIT_KEY)->stmt(),
                read_tmp_field(DATA_FIELD)->assign(M_INIT_VAL)->stmt(),
                TMP->access(ND_FIELD)->assign(M_INIT_VAL)->stmt()
            }), true),
            make_shared<CReturn>(TMP->id())
        });
    }

    return CFuncDef(move(fid), {}, move(body));
}

CFuncDef MapGenerator::declare_nd_initializer(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>(M_TYPE, "ND_" + M_NAME);

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        auto const TMP = make_tmp();
        body = make_shared<CBlock>(CBlockList{
            TMP,
            expand_loop_template(make_shared<CBlock>(CBlockList{
                read_tmp_field(SET_FIELD)->assign(FALSE)->stmt(),
                read_tmp_field(CURR_FIELD)->assign(M_INIT_KEY)->stmt(),
                read_tmp_field(DATA_FIELD)->assign(M_INIT_VAL)->stmt(),
                TMP->access(ND_FIELD)->assign(M_ND_VAL)->stmt()
            }), true),
            make_shared<CReturn>(TMP->id())
        });
    }

    return CFuncDef(move(fid), {}, move(body));
}

CFuncDef MapGenerator::declare_write(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>("void", "Write_" + M_NAME);

    auto arr = make_arr();
    auto key = make_key();
    auto data = make_shared<CVarDecl>(M_VAL_TYPE, "dat");

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        auto idx = generate_loop_idx();

        body = make_shared<CBlock>(CBlockList{
            idx,
            expand_array_search(),
            make_shared<CIf>(
                make_shared<CBinaryOp>(idx->id(), "<", M_LEN_LITERAL),
                read_arr_field(DATA_FIELD)->assign(data->id())->stmt(),
                nullptr
            )
        });
    }

    return CFuncDef(move(fid), CParams{ arr, key, data }, move(body));
}

CFuncDef MapGenerator::declare_read(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>(M_VAL_TYPE, "Read_" + M_NAME);

    auto arr = make_arr();
    auto key = make_key();

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        auto idx = generate_loop_idx();

        body = make_shared<CBlock>(CBlockList{
            idx,
            expand_array_search(),
            make_shared<CIf>(
                make_shared<CBinaryOp>(idx->id(), "==", M_LEN_LITERAL),
                make_shared<CReturn>(M_ND_VAL),
                nullptr
            ),
            make_shared<CReturn>(read_arr_field(DATA_FIELD))
        });
    }

    return CFuncDef(move(fid), CParams{ arr, key }, move(body));
}

CFuncDef MapGenerator::declare_ref(bool _forward_declare) const
{
    auto fid = make_shared<CVarDecl>(M_VAL_TYPE, "Ref_" + M_NAME, true);

    auto arr = make_arr();
    auto key = make_key();

    shared_ptr<CBlock> body;
    if (!_forward_declare)
    {
        auto idx = generate_loop_idx();

        body = make_shared<CBlock>(CBlockList{
            idx,
            expand_array_search(),
            make_shared<CIf>(
                make_shared<CBinaryOp>(idx->id(), "==", M_LEN_LITERAL),
                make_shared<CBlock>(CBlockList{
                    arr->access(ND_FIELD)->assign(M_ND_VAL)->stmt(),
                    make_shared<CReturn>(
                        make_shared<CReference>(arr->access(ND_FIELD))
                    )
                }),
                nullptr
            ),
            make_shared<CReturn>(
                make_shared<CReference>(read_arr_field(DATA_FIELD))
            )
        });
    }

    return CFuncDef(move(fid), CParams{ arr, key }, move(body));
}

// -------------------------------------------------------------------------- //

shared_ptr<CVarDecl> MapGenerator::generate_loop_idx() const
{
    return make_shared<CVarDecl>("int", "i", false, FALSE);
}

shared_ptr<CForLoop> MapGenerator::expand_loop_template(
    shared_ptr<CBlock> _body, bool _new_idx
) const
{
    auto idx = generate_loop_idx();
    auto loop_bnds = make_shared<CBinaryOp>(idx->id(), "<", M_LEN_LITERAL);
    auto loop_incr = make_shared<CUnaryOp>("++", idx->id(), true)->stmt();
    shared_ptr<CVarDecl> init = (_new_idx) ? idx : nullptr;

    return make_shared<CForLoop>(init, loop_bnds, loop_incr, _body);
}

shared_ptr<CForLoop> MapGenerator::expand_array_search() const
{
    return expand_loop_template(make_shared<CBlock>(CBlockList{
        make_shared<CIf>(
            make_shared<CBinaryOp>(read_arr_field(SET_FIELD), "!=", TRUE),
            make_shared<CBlock>(CBlockList{
                read_arr_field(CURR_FIELD)->assign(make_key()->id())->stmt(),
                read_arr_field(SET_FIELD)->assign(TRUE)->stmt(),
                make_shared<CBreak>()
            }),
            make_shared<CIf>(
                make_shared<CBinaryOp>(
                    read_arr_field(CURR_FIELD)->access("v"),
                    "==",
                    make_key()->access("v")
                ),
                make_shared<CBreak>(),
                nullptr
            )
        )
    }), false);
}

// -------------------------------------------------------------------------- //

shared_ptr<CVarDecl> MapGenerator::make_tmp() const
{
    return make_shared<CVarDecl>(M_TYPE, "tmp", false, nullptr);
}

shared_ptr<CVarDecl> MapGenerator::make_arr() const
{
    return make_shared<CVarDecl>(M_TYPE, "arr", true);
}

shared_ptr<CVarDecl> MapGenerator::make_key() const
{
    return make_shared<CVarDecl>(M_KEY_TYPE, "key");
}

shared_ptr<CIndexAccess> MapGenerator::read_tmp_field(string _field) const
{
    auto idx = generate_loop_idx()->id();
    return make_tmp()->id()->access(_field)->offset(idx);
}

shared_ptr<CIndexAccess> MapGenerator::read_arr_field(string _field) const
{
    auto idx = generate_loop_idx()->id();
    return make_arr()->id()->access(_field)->offset(idx);
}

// -------------------------------------------------------------------------- //

}
}
}
