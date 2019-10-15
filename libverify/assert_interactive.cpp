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

static const char g_solHelpCliArg[] = "help";
static const char g_solHelpCliMsg[] = "display options and settings";
static const char g_solZRetCliArg[] = "return-0";
static const char g_solZRetCliMsg[] = "when true, assertions return 0";

static bool g_solZRet;

void sol_setup(int _argc, const char **_argv)
{
    try
    {
        namespace po = boost::program_options;

        po::options_description desc("Interactive C Model Options.");
        desc.add_options()
            (g_solHelpCliArg, g_solHelpCliMsg)
            (g_solZRetCliArg, po::bool_switch(&g_solZRet), g_solZRetCliMsg);
    
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
