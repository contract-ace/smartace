/**
 * Defines an interactive implementation of assume for libfuzzer. When
 * assumption fails, the reason is logged, and complete the requirements
 * @date 2019
 */

#include "verify.h"

#include <cassert>
#include <csetjmp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

// -------------------------------------------------------------------------- //

// Encodes current state of the program.
// NONE: no exceptions are proprogation
// OUT_OF_DATA: this line was reached do to an out-of-data longjmp
// REQUIRED_FAIL: this line was reached do to a require failure longjmp.
enum ExceptionType { NONE, OUT_OF_DATA, REQUIRE_FAILED };

// A global variable which stores the state to restore on exception.
static jmp_buf Env;

// Global variable used to proprogation exceptions. If it is none, no exceptions
// are being proprogated. It should only be set by setjmp, which will raise the
// current exception flag.
static ExceptionType exception_type = NONE;

// Defines a global array RandData, with SizeOfRandData bytes. CounterOfRandData
// maintains an index to track which byte is currently being read.
static uint8_t * RandData;
static size_t SizeOfRandData = 0;
static size_t CounterOfRandData = 0;

// Sets up the exploration with Env environment, and returns the result of setjmp.
int SetupExploration(void);

// Terminates the current exploration. The program returns to Env, while
// proprogating the current exception type. 
void TerminateExploration(ExceptionType e);

// Moves data to the global libfuzzer array.
void ran(uint8_t const* Data, size_t Size);

// Produces the next random byte, if available. If data is exhausted, an
// exception will raise.
uint8_t tryGetNextRandByte();

// Inputs the data.
extern "C" int LLVMFuzzerTestOneInput(uint8_t const* Data, size_t Size);

// -------------------------------------------------------------------------- //

void sol_setup(int _argc, const char **_argv) {}

// -------------------------------------------------------------------------- //

uint8_t sol_continue(void)
{
	return 1;
}

// -------------------------------------------------------------------------- //

void sol_on_transaction(void) {}

// -------------------------------------------------------------------------- //

void sol_assert(sol_raw_uint8_t _cond, const char* _msg)
{
    if (!_cond)
	{
		cerr << "assert";
		if(_msg)
		{
			cerr << ": " << _msg;
		}
		cerr << endl; 
    }
	assert(_cond);
}

void sol_require(sol_raw_uint8_t _cond, const char* _msg)
{
	if (!_cond)
	{
		/*
		cout << "require";
		if(_msg)
		{
			cout << ": " << _msg;
		}
		cout << endl; 
		*/
    }
	ll_assume(_cond);	
}

// -------------------------------------------------------------------------- //

void sol_emit(const char*) {}

// -------------------------------------------------------------------------- //

void ll_assume(sol_raw_uint8_t _cond)
{
    if (!_cond) TerminateExploration(REQUIRE_FAILED);
}

// -------------------------------------------------------------------------- //

void on_entry(const char* _type, const char* _msg)
{
	// cout << _msg << " [" << _type << "]:";
}

uint8_t nd_byte(const char* _msg)
{
	on_entry("uint8", _msg);
	return tryGetNextRandByte();
}

uint8_t nd_range(uint8_t l, uint8_t u, const char* _msg)
{
	uint8_t v = nd_byte(_msg);
	return (v % (u - l)) + l;
}

// -------------------------------------------------------------------------- //

sol_raw_uint256_t nd_increase(
	sol_raw_uint256_t _curr, uint8_t _strict, const char* _msg
)
{
	if (_strict)
	{
		ll_assume(_curr < SOL_UINT256_MAX);
		_curr += 1;
	}
	sol_raw_uint256_t max_increase = SOL_UINT256_MAX - _curr;
	sol_raw_uint256_t delta = nd_uint256_t(_msg) % max_increase;
	return _curr + delta;
}

// -------------------------------------------------------------------------- //

void smartace_log(const char*) {}

// -------------------------------------------------------------------------- //

int SetupExploration(void)
{
	return setjmp(Env);
}

void TerminateExploration (ExceptionType e)
{
	exception_type = e;
	longjmp(Env, e);
}

uint8_t tryGetNextRandByte()
{
	if (CounterOfRandData >= SizeOfRandData)
	{
		TerminateExploration(OUT_OF_DATA);
	}
	uint8_t ret = RandData[CounterOfRandData];
	CounterOfRandData++;
	return ret;
}

void ran(uint8_t const* Data, size_t Size)
{
	// Allocates RandData, and ensures it is not null.
	RandData = (uint8_t *) malloc(Size * sizeof(uint8_t));
	ll_assume(RandData != nullptr);

	SizeOfRandData = Size;
	CounterOfRandData = 0;
	for (size_t i = 0; i < Size; ++i)
	{
		RandData[i] = Data[i];
	}
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* Data, size_t Size)
{
	exception_type = NONE;

	switch(SetupExploration())
	{
		case OUT_OF_DATA: break;
		case REQUIRE_FAILED: break;
		case NONE:
			// cout << endl;
			ran(Data, Size);
			run_model();
			break;
	}

	if (RandData) free(RandData);

	return 0;
}

// -------------------------------------------------------------------------- //

sol_raw_int8_t nd_int8_t(const char* _msg)
{
	on_entry("int8", _msg);
	sol_raw_int8_t retval = 0;
	for (int i = 1; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int8_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint8_t nd_uint8_t(const char* _msg)
{
	on_entry("uint8", _msg);
	sol_raw_uint8_t retval = 0;
	for (int i = 1; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint8_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int16_t nd_int16_t(const char* _msg)
{
	on_entry("int16", _msg);
	sol_raw_int16_t retval = 0;
	for (int i = 2; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int16_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint16_t nd_uint16_t(const char* _msg)
{
	on_entry("uint16", _msg);
	sol_raw_uint16_t retval = 0;
	for (int i = 2; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint16_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int24_t nd_int24_t(const char* _msg)
{
	on_entry("int24", _msg);
	sol_raw_int24_t retval = 0;
	for (int i = 3; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int24_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint24_t nd_uint24_t(const char* _msg)
{
	on_entry("uint24", _msg);
	sol_raw_uint24_t retval = 0;
	for (int i = 3; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint24_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int32_t nd_int32_t(const char* _msg)
{
	on_entry("int32", _msg);
	sol_raw_int32_t retval = 0;
	for (int i = 4; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int32_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint32_t nd_uint32_t(const char* _msg)
{
	on_entry("uint32", _msg);
	sol_raw_uint32_t retval = 0;
	for (int i = 4; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint32_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int40_t nd_int40_t(const char* _msg)
{
	on_entry("int40", _msg);
	sol_raw_int40_t retval = 0;
	for (int i = 5; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int40_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint40_t nd_uint40_t(const char* _msg)
{
	on_entry("uint40", _msg);
	sol_raw_uint40_t retval = 0;
	for (int i = 5; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint40_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int48_t nd_int48_t(const char* _msg)
{
	on_entry("int48", _msg);
	sol_raw_int48_t retval = 0;
	for (int i = 6; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int48_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint48_t nd_uint48_t(const char* _msg)
{
	on_entry("uint48", _msg);
	sol_raw_uint48_t retval = 0;
	for (int i = 6; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint48_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int56_t nd_int56_t(const char* _msg)
{
	on_entry("int56", _msg);
	sol_raw_int56_t retval = 0;
	for (int i = 7; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int56_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint56_t nd_uint56_t(const char* _msg)
{
	on_entry("uint56", _msg);
	sol_raw_uint56_t retval = 0;
	for (int i = 7; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint56_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int64_t nd_int64_t(const char* _msg)
{
	on_entry("int64", _msg);
	sol_raw_int64_t retval = 0;
	for (int i = 8; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int64_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint64_t nd_uint64_t(const char* _msg)
{
	on_entry("uint64", _msg);
	sol_raw_uint64_t retval = 0;
	for (int i = 8; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint64_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int72_t nd_int72_t(const char* _msg)
{
	on_entry("int72", _msg);
	sol_raw_int72_t retval = 0;
	for (int i = 9; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int72_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint72_t nd_uint72_t(const char* _msg)
{
	on_entry("uint72", _msg);
	sol_raw_uint72_t retval = 0;
	for (int i = 9; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint72_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int80_t nd_int80_t(const char* _msg)
{
	on_entry("int80", _msg);
	sol_raw_int80_t retval = 0;
	for (int i = 10; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int80_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint80_t nd_uint80_t(const char* _msg)
{
	on_entry("uint80", _msg);
	sol_raw_uint80_t retval = 0;
	for (int i = 10; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint80_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int88_t nd_int88_t(const char* _msg)
{
	on_entry("int88", _msg);
	sol_raw_int88_t retval = 0;
	for (int i = 11; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int88_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint88_t nd_uint88_t(const char* _msg)
{
	on_entry("uint88", _msg);
	sol_raw_uint88_t retval = 0;
	for (int i = 11; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint88_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int96_t nd_int96_t(const char* _msg)
{
	on_entry("int96", _msg);
	sol_raw_int96_t retval = 0;
	for (int i = 12; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int96_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint96_t nd_uint96_t(const char* _msg)
{
	on_entry("uint96", _msg);
	sol_raw_uint96_t retval = 0;
	for (int i = 12; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint96_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int104_t nd_int104_t(const char* _msg)
{
	on_entry("int104", _msg);
	sol_raw_int104_t retval = 0;
	for (int i = 13; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int104_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint104_t nd_uint104_t(const char* _msg)
{
	on_entry("uint104", _msg);
	sol_raw_uint104_t retval = 0;
	for (int i = 13; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint104_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_int112_t nd_int112_t(const char* _msg)
{
	on_entry("int112", _msg);
	sol_raw_int112_t retval = 0;
	for (int i = 14; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int112_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint112_t nd_uint112_t(const char* _msg)
{
	on_entry("uint112", _msg);
	sol_raw_uint112_t retval = 0;
	for (int i = 14; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint112_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int120_t nd_int120_t(const char* _msg)
{
	on_entry("int120", _msg);
	sol_raw_int120_t retval = 0;
	for (int i = 15; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int120_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint120_t nd_uint120_t(const char* _msg)
{
	on_entry("uint120", _msg);
	sol_raw_uint120_t retval = 0;
	for (int i = 15; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint120_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_int128_t nd_int128_t(const char* _msg)
{
	on_entry("int128", _msg);
	sol_raw_int128_t retval = 0;
	for (int i = 16; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int128_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint128_t nd_uint128_t(const char* _msg)
{
	on_entry("uint128", _msg);
	sol_raw_uint128_t retval = 0;
	for (int i = 16; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint128_t)tryGetNextRandByte();
	}
	return retval;			
}

sol_raw_int136_t nd_int136_t(const char* _msg)
{
	on_entry("int136", _msg);
	sol_raw_int136_t retval = 0;
	for (int i = 17; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int136_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint136_t nd_uint136_t(const char* _msg)
{
	on_entry("uint136", _msg);
	sol_raw_uint136_t retval = 0;
	for (int i = 17; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint136_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_int144_t nd_int144_t(const char* _msg)
{
	on_entry("int144", _msg);
	sol_raw_int144_t retval = 0;
	for (int i = 18; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int144_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint144_t nd_uint144_t(const char* _msg)
{
	on_entry("uint144", _msg);
	sol_raw_uint144_t retval = 0;
	for (int i = 18; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint144_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int152_t nd_int152_t(const char* _msg)
{
	on_entry("int152", _msg);
	sol_raw_int152_t retval = 0;
	for (int i = 19; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int152_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint152_t nd_uint152_t(const char* _msg)
{
	on_entry("uint152", _msg);
	sol_raw_uint152_t retval = 0;
	for (int i = 19; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint152_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int160_t nd_int160_t(const char* _msg)
{
	on_entry("int160", _msg);
	sol_raw_int160_t retval = 0;
	for (int i = 20; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int160_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint160_t nd_uint160_t(const char* _msg)
{
	on_entry("uint160", _msg);
	sol_raw_uint160_t retval = 0;
	for (int i = 20; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint160_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int168_t nd_int168_t(const char* _msg)
{
	on_entry("int168", _msg);
	sol_raw_int168_t retval = 0;
	for (int i = 21; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int168_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint168_t nd_uint168_t(const char* _msg)
{
	on_entry("uint168", _msg);
	sol_raw_uint168_t retval = 0;
	for (int i = 21; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint168_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int176_t nd_int176_t(const char* _msg)
{
	on_entry("int176", _msg);
	sol_raw_int176_t retval = 0;
	for (int i = 22; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int176_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint176_t nd_uint176_t(const char* _msg)
{
	on_entry("uint176", _msg);
	sol_raw_uint176_t retval = 0;
	for (int i = 22; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint176_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_int184_t nd_int184_t(const char* _msg)
{
	on_entry("int184", _msg);
	sol_raw_int184_t retval = 0;
	for (int i = 23; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int184_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint184_t nd_uint184_t(const char* _msg)
{
	on_entry("uint184", _msg);
	sol_raw_uint184_t retval = 0;
	for (int i = 23; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint184_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int192_t nd_int192_t(const char* _msg)
{
	on_entry("int192", _msg);
	sol_raw_int192_t retval = 0;
	for (int i = 24; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int192_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint192_t nd_uint192_t(const char* _msg)
{
	on_entry("uint192", _msg);
	sol_raw_uint192_t retval = 0;
	for (int i = 24; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint192_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int200_t nd_int200_t(const char* _msg)
{
	on_entry("int200", _msg);
	sol_raw_int200_t retval = 0;
	for (int i = 25; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int200_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint200_t nd_uint200_t(const char* _msg)
{
	on_entry("uint200", _msg);
	sol_raw_uint200_t retval = 0;
	for (int i = 25; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint200_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_int208_t nd_int208_t(const char* _msg)
{
	on_entry("int208", _msg);
	sol_raw_int208_t retval = 0;
	for (int i = 26; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int208_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint208_t nd_uint208_t(const char* _msg)
{
	on_entry("uint208", _msg);
	sol_raw_uint208_t retval = 0;
	for (int i = 26; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint208_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int216_t nd_int216_t(const char* _msg)
{
	on_entry("int216", _msg);
	sol_raw_int216_t retval = 0;
	for (int i = 27; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int216_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint216_t nd_uint216_t(const char* _msg)
{
	on_entry("uint216", _msg);
	sol_raw_uint216_t retval = 0;
	for (int i = 27; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint216_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int224_t nd_int224_t(const char* _msg)
{
	on_entry("int224", _msg);
	sol_raw_int224_t retval = 0;
	for (int i = 28; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int224_t)tryGetNextRandByte();
	}
	return retval;			
}

sol_raw_uint224_t nd_uint224_t(const char* _msg)
{
	on_entry("uint224", _msg);
	sol_raw_uint224_t retval = 0;
	for (int i = 28; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint224_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int232_t nd_int232_t(const char* _msg)
{
	on_entry("int232", _msg);
	sol_raw_int232_t retval = 0;
	for (int i = 29; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int232_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint232_t nd_uint232_t(const char* _msg)
{
	on_entry("uint232", _msg);
	sol_raw_uint232_t retval = 0;
	for (int i = 29; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint232_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int240_t nd_int240_t(const char* _msg)
{
	on_entry("int240", _msg);
	sol_raw_int240_t retval = 0;
	for (int i = 30; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int240_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint240_t nd_uint240_t(const char* _msg)
{
	on_entry("uint240", _msg);
	sol_raw_uint240_t retval = 0;
	for (int i = 30; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint240_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int248_t nd_int248_t(const char* _msg)
{
	on_entry("int248", _msg);
	sol_raw_int248_t retval = 0;
	for (int i = 31; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int248_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint248_t nd_uint248_t(const char* _msg)
{
	on_entry("uint248", _msg);
	sol_raw_uint248_t retval = 0;
	for (int i = 31; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint248_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int256_t nd_int256_t(const char* _msg)
{
	on_entry("int256", _msg);
	sol_raw_int256_t retval = 0;
	for (int i = 32; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_int256_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint256_t nd_uint256_t(const char* _msg)
{
	on_entry("uint256", _msg);
	sol_raw_uint256_t retval = 0;
	for (int i = 32; i > 0; i--)
	{
		retval = retval << 8;
		retval = retval + (sol_raw_uint256_t)tryGetNextRandByte();
	}
	return retval;			
}

// -------------------------------------------------------------------------- //
