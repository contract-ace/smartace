/**
 * Defines an interactive implementation of assume and require. When either
 * assertation fails, the execution is halted, the reason is logged. If the
 * assertion was a requirement, then a non-zero return value is produced.
 * @date 2019
 */

#include "verify.h"

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>

using namespace std;

// -------------------------------------------------------------------------- //

static uint64_t g_solTransactionNumber;

static const char g_solHelpCliArg[] = "help";
static const char g_solHelpCliMsg[] = "display options and settings";
static const char g_solZRetCliArg[] = "return-0";
static const char g_solZRetCliMsg[] = "when true, assertions return 0";
static const char g_solTransNCliArg[] = "count-transactions";
static const char gTransNCliMsg[] = "when true, logs total transactions";

static bool g_solZRet;
static bool g_solLogTrans;

void sol_setup(int _argc, const char **_argv)
{
    g_solTransactionNumber = 0;

    try
    {
        namespace po = boost::program_options;

        po::options_description desc("Interactive C Model Options.");
        desc.add_options()
            (g_solHelpCliArg, g_solHelpCliMsg)
            (g_solZRetCliArg, po::bool_switch(&g_solZRet), g_solZRetCliMsg)
            (g_solTransNCliArg, po::bool_switch(&g_solLogTrans), gTransNCliMsg);
    
        po::variables_map args;
        po::store(po::parse_command_line(_argc, _argv, desc), args);
        po::notify(args);

        if (args.count(g_solHelpCliArg))
        {
            cout << desc << endl;
            exit(0);
        }
    }
    catch (exception const& e)
    {
        cerr << "Interactive C Model Setup Error: " << e.what() << endl;
        exit(-1);
    }
    catch (...)
    {
        cerr << "Interactive C Model Setup Error: Unexpected error." << endl;
        exit(-1);
    }
}

// -------------------------------------------------------------------------- //

sol_raw_uint8_t sol_crypto(void)
{
    return nd_byte(0, "Select crypto value");
}

// -------------------------------------------------------------------------- //

uint8_t sol_continue(void)
{
	return nd_byte(0, "Select 0 to terminate");
}

// -------------------------------------------------------------------------- //

void sol_on_transaction(void)
{
    ++g_solTransactionNumber;
}

// -------------------------------------------------------------------------- //

void ll_assume(sol_raw_uint8_t _cond)
{
    sol_require(_cond, nullptr);
}

// -------------------------------------------------------------------------- //

void sol_assertion_impl(
    int _status, sol_raw_uint8_t _cond, char const* _check, char const* _msg
)
{
    if (!_cond)
    {
        if (g_solZRet)
        {
            _status = 0;
        }

        cerr << _check;
        if (_msg)
        {
            cerr << ": " << _msg;
        }
        cerr << endl;

        if (g_solLogTrans)
        {
            cerr << "Transaction Count: " << g_solTransactionNumber << endl;
        }

        exit(_status);
    }
}

void sol_assert(sol_raw_uint8_t _cond, const char* _msg)
{
    sol_assertion_impl(-1, _cond, "assert", _msg);
}

void sol_require(sol_raw_uint8_t _cond, const char* _msg)
{
    sol_assertion_impl(0, _cond, "require", _msg);
}

// -------------------------------------------------------------------------- //

void sol_emit(const char* _msg)
{
    cout << "Emit: " << _msg << endl;
}

// -------------------------------------------------------------------------- //

void smartace_log(const char* _msg)
{
	cout << _msg << endl;
}

// -------------------------------------------------------------------------- //

void on_entry(const char* _type, const char* _msg)
{
    cout << _msg << " [" << _type << "]: ";
}

uint8_t nd_byte(int8_t, const char* _msg)
{
    on_entry("uint8", _msg);
    uint8_t retval;
    scanf("%hhu", &retval);
    return retval;
}

uint8_t nd_range(int8_t, uint8_t _l, uint8_t _u, const char* _msg)
{
    stringstream type;
    type << "uint8 from " << unsigned(_l) << " to " << unsigned(_u - 1);
    on_entry(type.str().c_str(), _msg);

    uint8_t retval;
    scanf("%hhu", &retval);
	ll_assume(retval >= _l);
	ll_assume(retval < _u);
	return retval;
}

// -------------------------------------------------------------------------- //

#ifdef MC_USE_STDINT
__int128_t nd_stdint_uint128_t(void)
{
    char input[40];
    scanf("%s", input);

    __int128_t retval = 0;
    for (unsigned int i = 0; input[i] != 0; ++i)
    {
        retval *= 10;
        retval += (__int128_t)(input[i] - '0');
    }

    return retval;
}
#endif

sol_raw_uint256_t ll_nd_uint256_t(void)
{
    sol_raw_uint256_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint256_t nd_increase(
    sol_raw_int256_t,
    sol_raw_uint256_t _curr,
    uint8_t _strict,
    const char* _msg
)
{
    stringstream type;
    type << "uint " << (_strict ? "larger" : "no less") << " than " << _curr;
    on_entry(type.str().c_str(), _msg);

    sol_raw_uint256_t next = ll_nd_uint256_t();
	if (_strict) ll_assume(next > _curr);
	else ll_assume(next >= _curr);
	return next;
}

// -------------------------------------------------------------------------- //

sol_raw_int8_t nd_int8_t(sol_raw_int8_t, const char* _msg)
{
    on_entry("int8", _msg);
    sol_raw_int8_t retval = 0;

    #ifdef MC_USE_STDINT
    scanf("%hhu", &retval);
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int16_t nd_int16_t(sol_raw_int16_t, const char* _msg)
{
    on_entry("int16", _msg);
    sol_raw_int16_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_int24_t nd_int24_t(sol_raw_int24_t, const char* _msg)
{
    on_entry("int24", _msg);
    sol_raw_int24_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_int32_t nd_int32_t(sol_raw_int32_t, const char* _msg)
{
    on_entry("int32", _msg);
    sol_raw_int32_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_int40_t nd_int40_t(sol_raw_int40_t, const char* _msg)
{
    on_entry("int40", _msg);
    sol_raw_int40_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_int48_t nd_int48_t(sol_raw_int48_t, const char* _msg)
{
    on_entry("int48", _msg);
    sol_raw_int48_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_int56_t nd_int56_t(sol_raw_int56_t, const char* _msg)
{
    on_entry("int56", _msg);
    sol_raw_int56_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_int64_t nd_int64_t(sol_raw_int64_t, const char* _msg)
{
    on_entry("int64", _msg);
    sol_raw_int64_t retval = 0;
    cin >> retval;
    return retval;
}

#ifdef MC_USE_STDINT
__int128_t nd_stdint_int128_t(void)
{
    char input[41];
    scanf("%s", input);

    const int IS_NEG = (input[0] == '-');
    const int FIRST_DIGIT_POS = (IS_NEG ? 1 : 0);

    __int128_t retval = 0;
    for (unsigned int i = FIRST_DIGIT_POS; input[i] != 0; ++i)
    {
        retval *= 10;
        retval += (__int128_t)(input[i] - '0');
    }

    if (IS_NEG) retval = -retval;

    return retval;
}
#endif

sol_raw_int72_t nd_int72_t(sol_raw_int72_t, const char* _msg)
{
    on_entry("int72", _msg);
    sol_raw_int72_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int80_t nd_int80_t(sol_raw_int80_t, const char* _msg)
{
    on_entry("int80", _msg);
    sol_raw_int80_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int88_t nd_int88_t(sol_raw_int88_t, const char* _msg)
{
    on_entry("int88", _msg);
    sol_raw_int88_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int96_t nd_int96_t(sol_raw_int96_t, const char* _msg)
{
    on_entry("int96", _msg);
    sol_raw_int96_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int104_t nd_int104_t(sol_raw_int104_t, const char* _msg)
{
    on_entry("int104", _msg);
    sol_raw_int104_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int112_t nd_int112_t(sol_raw_int112_t, const char* _msg)
{
    on_entry("int112", _msg);
    sol_raw_int112_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int120_t nd_int120_t(sol_raw_int120_t, const char* _msg)
{
    on_entry("int120", _msg);
    sol_raw_int120_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int128_t nd_int128_t(sol_raw_int128_t, const char* _msg)
{
    on_entry("int128", _msg);
    sol_raw_int128_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int136_t nd_int136_t(sol_raw_int136_t, const char* _msg)
{
    on_entry("int136", _msg);
    sol_raw_int136_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int144_t nd_int144_t(sol_raw_int144_t, const char* _msg)
{
    on_entry("int144", _msg);
    sol_raw_int144_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int152_t nd_int152_t(sol_raw_int152_t, const char* _msg)
{
    on_entry("int152", _msg);
    sol_raw_int152_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int160_t nd_int160_t(sol_raw_int160_t, const char* _msg)
{
    on_entry("int160", _msg);
    sol_raw_int160_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int168_t nd_int168_t(sol_raw_int168_t, const char* _msg)
{
    on_entry("int168", _msg);
    sol_raw_int168_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int176_t nd_int176_t(sol_raw_int176_t, const char* _msg)
{
    on_entry("int176", _msg);
    sol_raw_int176_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int184_t nd_int184_t(sol_raw_int184_t, const char* _msg)
{
    on_entry("int184", _msg);
    sol_raw_int184_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int192_t nd_int192_t(sol_raw_int192_t, const char* _msg)
{
    on_entry("int192", _msg);
    sol_raw_int192_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int200_t nd_int200_t(sol_raw_int200_t, const char* _msg)
{
    on_entry("int200", _msg);
    sol_raw_int200_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int208_t nd_int208_t(sol_raw_int208_t, const char* _msg)
{
    on_entry("int208", _msg);
    sol_raw_int208_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int216_t nd_int216_t(sol_raw_int216_t, const char* _msg)
{
    on_entry("int216", _msg);
    sol_raw_int216_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int224_t nd_int224_t(sol_raw_int224_t, const char* _msg)
{
    on_entry("int224", _msg);
    sol_raw_int224_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int232_t nd_int232_t(sol_raw_int232_t, const char* _msg)
{
    on_entry("int232", _msg);
    sol_raw_int232_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int240_t nd_int240_t(sol_raw_int240_t, const char* _msg)
{
    on_entry("int240", _msg);
    sol_raw_int240_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int248_t nd_int248_t(sol_raw_int248_t, const char* _msg)
{
    on_entry("int248", _msg);
    sol_raw_int248_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_int256_t nd_int256_t(sol_raw_int256_t, const char* _msg)
{
    on_entry("int256", _msg);
    sol_raw_int256_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_int128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint8_t nd_uint8_t(sol_raw_int8_t, const char* _msg)
{
    on_entry("uint8", _msg);
    sol_raw_uint8_t retval = 0;

    #ifdef MC_USE_STDINT
    scanf("%hhu", &retval);
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint16_t nd_uint16_t(sol_raw_int16_t, const char* _msg)
{
    on_entry("uint16", _msg);
    sol_raw_uint16_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_uint24_t nd_uint24_t(sol_raw_int24_t, const char* _msg)
{
    on_entry("uint24", _msg);
    sol_raw_uint24_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_uint32_t nd_uint32_t(sol_raw_int32_t, const char* _msg)
{
    on_entry("uint32", _msg);
    sol_raw_uint32_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_uint40_t nd_uint40_t(sol_raw_int40_t, const char* _msg)
{
    on_entry("uint40", _msg);
    sol_raw_uint40_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_uint48_t nd_uint48_t(sol_raw_int48_t, const char* _msg)
{
    on_entry("uint48", _msg);
    sol_raw_uint48_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_uint56_t nd_uint56_t(sol_raw_int56_t, const char* _msg)
{
    on_entry("uint56", _msg);
    sol_raw_uint56_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_uint64_t nd_uint64_t(sol_raw_int64_t, const char* _msg)
{
    on_entry("uint64", _msg);
    sol_raw_uint64_t retval = 0;
    cin >> retval;
    return retval;
}

sol_raw_uint72_t nd_uint72_t(sol_raw_int72_t, const char* _msg)
{
    on_entry("uint72", _msg);
    sol_raw_uint72_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint80_t nd_uint80_t(sol_raw_int80_t, const char* _msg)
{
    on_entry("uint80", _msg);
    sol_raw_uint80_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint88_t nd_uint88_t(sol_raw_int88_t, const char* _msg)
{
    on_entry("uint88", _msg);
    sol_raw_uint88_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint96_t nd_uint96_t(sol_raw_int96_t, const char* _msg)
{
    on_entry("uint96", _msg);
    sol_raw_uint96_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint104_t nd_uint104_t(sol_raw_int104_t, const char* _msg)
{
    on_entry("uint104", _msg);
    sol_raw_uint104_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint112_t nd_uint112_t(sol_raw_int112_t, const char* _msg)
{
    on_entry("uint112", _msg);
    sol_raw_uint112_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint120_t nd_uint120_t(sol_raw_int120_t, const char* _msg)
{
    on_entry("uint120", _msg);
    sol_raw_uint120_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint128_t nd_uint128_t(sol_raw_int128_t, const char* _msg)
{
    on_entry("uint128", _msg);
    sol_raw_uint128_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint136_t nd_uint136_t(sol_raw_int136_t, const char* _msg)
{
    on_entry("uint136", _msg);
    sol_raw_uint136_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint144_t nd_uint144_t(sol_raw_int144_t, const char* _msg)
{
    on_entry("uint144", _msg);
    sol_raw_uint144_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint152_t nd_uint152_t(sol_raw_int152_t, const char* _msg)
{
    on_entry("uint152", _msg);
    sol_raw_uint152_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint160_t nd_uint160_t(sol_raw_int160_t, const char* _msg)
{
    on_entry("uint160", _msg);
    sol_raw_uint160_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint168_t nd_uint168_t(sol_raw_int168_t, const char* _msg)
{
    on_entry("uint168", _msg);
    sol_raw_uint168_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint176_t nd_uint176_t(sol_raw_int176_t, const char* _msg)
{
    on_entry("uint176", _msg);
    sol_raw_uint176_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint184_t nd_uint184_t(sol_raw_int184_t, const char* _msg)
{
    on_entry("uint184", _msg);
    sol_raw_uint184_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint192_t nd_uint192_t(sol_raw_int192_t, const char* _msg)
{
    on_entry("uint192", _msg);
    sol_raw_uint192_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint200_t nd_uint200_t(sol_raw_int200_t, const char* _msg)
{
    on_entry("uint200", _msg);
    sol_raw_uint200_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint208_t nd_uint208_t(sol_raw_int208_t, const char* _msg)
{
    on_entry("uint208", _msg);
    sol_raw_uint208_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint216_t nd_uint216_t(sol_raw_int216_t, const char* _msg)
{
    on_entry("uint216", _msg);
    sol_raw_uint216_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint224_t nd_uint224_t(sol_raw_int224_t, const char* _msg)
{
    on_entry("uint224", _msg);
    sol_raw_uint224_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint232_t nd_uint232_t(sol_raw_int232_t, const char* _msg)
{
    on_entry("uint232", _msg);
    sol_raw_uint232_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint240_t nd_uint240_t(sol_raw_int240_t, const char* _msg)
{
    on_entry("uint240", _msg);
    sol_raw_uint240_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint248_t nd_uint248_t(sol_raw_int248_t, const char* _msg)
{
    on_entry("uint248", _msg);
    sol_raw_uint248_t retval = 0;

    #ifdef MC_USE_STDINT
    retval = nd_stdint_uint128_t();
    #elif defined MC_USE_BOOST_MP
    std::cin >> retval;
    #endif

    return retval;
}

sol_raw_uint256_t nd_uint256_t(sol_raw_int256_t, const char* _msg)
{
    on_entry("uint256", _msg);
    return ll_nd_uint256_t();
}

// -------------------------------------------------------------------------- //
