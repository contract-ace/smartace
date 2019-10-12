/**
 * Defines an interactive implementation of assume for libfuzzer. When
 * assumption fails, the reason is logged, and complete the requirements
 * @date 2019
 */

#include "verify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <assert.h>

/**
 * Defines an interface to libverify, with reconfigurable data types. These
 * types are controlled at build time using preprocessor definitions.
 */


#ifdef __cplusplus
extern "C" {
#endif

/*global variable Env record the environment*/
static jmp_buf Env;

/*global variable exception_type 
 * record the return value of setjmp function,
 * which can label different exception types*/
enum ExceptionType {NONE, OUT_OF_DATA, REQUIRE_FAILED};
static ExceptionType exception_type = NONE;

/*set up the exploration with Env environment,
 * return the result of setjmp function*/
int SetupExploration (void);

/*terminate the exploration with Env environment 
 * and different exception types, 
 * which can jump to setjmp with different results*/
void TerminateExploration (ExceptionType e);

/*
 * assign the global array with Data array*/
void ran(const uint8_t *Data, size_t Size);

/* whether the global array run out*/
void whether_RunOut(void);

/*get the next RandByte*/
uint8_t tryGetNextRandByte();

/*
 * input the Data */
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size);

#ifdef __cplusplus
}
#endif
void sol_assert(sol_raw_uint8_t _cond, const char* _msg)
{
    if( !_cond){
		fprintf(stderr,"assert");
		if(_msg) fprintf(stderr,":%s", _msg);
		fprintf(stderr,"\n");    
    }
        assert(_cond);
}

void sol_require(sol_raw_uint8_t _cond, const char* _msg)
{
	if( !_cond){
		fprintf(stderr,"require");
		if(_msg) fprintf(stderr,":%s", _msg);
		fprintf(stderr,"\n");
    }
		ll_assume(_cond);	
}

void ll_assume(sol_raw_uint8_t _cond){
    if(!_cond){

	/*exception_type REQUIRE_FAILED: 
	 * the ll_assume is failure*/
	TerminateExploration(REQUIRE_FAILED);	
    }
}

/*
 * declare the global array RandData 
 * with SizeOfRandData size, 
 * global Counter CounterOfRandData */
uint8_t *RandData;
size_t SizeOfRandData = 0;
size_t CounterOfRandData = 0;


/*set up the exploration with Env environment,
 * return the result of setjmp function*/
int SetupExploration (void){
	int r;
	r = setjmp(Env);
	return r;
}

/*terminate the exploration with Env environment 
 * and different exception types, 
 * which can jump to setjmp with different results*/
void TerminateExploration (ExceptionType e){
	exception_type = e;
	longjmp(Env, e);
}

/*
 * whether the global array run out */
void whether_RunOut(void)
{
	if (CounterOfRandData >= SizeOfRandData)
	{
		/*exception_type OUT_OF_DATA: 
		 * run out of RandData*/
		TerminateExploration(OUT_OF_DATA);
	}
}

/*get the next RandByte*/
uint8_t tryGetNextRandByte(){
	whether_RunOut();
	uint8_t ret = RandData[CounterOfRandData];
	CounterOfRandData++;
	return ret;
}

/*
 * assign the global array with Data array*/
void ran(const uint8_t *Data, size_t Size)
{
	RandData = (uint8_t *) malloc(Size*sizeof(uint8_t));

    //the size of the RandData should never be zero
	ll_assume(RandData!=((uint8_t *)0));

	SizeOfRandData = Size;
	CounterOfRandData = 0;
	size_t i = 0;
	while(i < Size)
	{
		RandData[i] = Data[i];
		i++;
	}
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
	exception_type = NONE;
	switch(SetupExploration())
	{
		case 1:
			/*exception_type OUT_OF_DATA: 
			 * run out of RandData*/
			break;
		case 2:
			/*exception_type REQUIRE_FAILED: 
			 * the require statement is failure*/
			break;
		case 0:
			/*normal operations*/
			//printf("setup successed. \n");
			printf("\n");
			ran(Data,Size);
			run_model();
	}
	if (RandData)
	{
		free(RandData);
	}
	return 0;
}


/*
 * generate the random number for functions invoked by main function */
void on_entry(const char* _type, const char* _msg){
	printf("%s [%s]:", _msg, _type);
}

uint8_t rt_nd_byte(const char* _msg){
	on_entry("uint8", _msg);
	uint8_t retval = 0;
	retval = tryGetNextRandByte();
	return retval;
}

sol_raw_int8_t nd_int8_t(const char* _msg){
	on_entry("int8", _msg);
	sol_raw_int8_t retval = 0;
	for (int i = 1 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int8_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint8_t nd_uint8_t(const char* _msg){
	on_entry("uint8", _msg);
	sol_raw_uint8_t retval = 0;
	for (int i = 1 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint8_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int16_t nd_int16_t(const char* _msg){
	on_entry("int16", _msg);
	sol_raw_int16_t retval = 0;
	for (int i = 2 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int16_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint16_t nd_uint16_t(const char* _msg){
	on_entry("uint16", _msg);
	sol_raw_uint16_t retval = 0;
	for (int i = 2 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint16_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int24_t nd_int24_t(const char* _msg){
	on_entry("int24", _msg);
	sol_raw_int24_t retval = 0;
	for (int i = 3 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int24_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint24_t nd_uint24_t(const char* _msg){
	on_entry("uint24", _msg);
	sol_raw_uint24_t retval = 0;
	for (int i = 3 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint24_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int32_t nd_int32_t(const char* _msg){
	on_entry("int32", _msg);
	sol_raw_int32_t retval = 0;
	for (int i = 4 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int32_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint32_t nd_uint32_t(const char* _msg){
	on_entry("uint32", _msg);
	sol_raw_uint32_t retval = 0;
	for (int i = 4 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint32_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int40_t nd_int40_t(const char* _msg){
	on_entry("int40", _msg);
	sol_raw_int40_t retval = 0;
	for (int i = 5 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int40_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint40_t nd_uint40_t(const char* _msg){
	on_entry("uint40", _msg);
	sol_raw_uint40_t retval = 0;
	for (int i = 5 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint40_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int48_t nd_int48_t(const char* _msg){
	on_entry("int48", _msg);
	sol_raw_int48_t retval = 0;
	for (int i = 6 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int48_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint48_t nd_uint48_t(const char* _msg){
	on_entry("uint48", _msg);
	sol_raw_uint48_t retval = 0;
	for (int i = 6 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint48_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int56_t nd_int56_t(const char* _msg){
	on_entry("int56", _msg);
	sol_raw_int56_t retval = 0;
	for (int i = 7 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int56_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint56_t nd_uint56_t(const char* _msg){
	on_entry("uint56", _msg);
	sol_raw_uint56_t retval = 0;
	for (int i = 7 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint56_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int64_t nd_int64_t(const char* _msg){
	on_entry("int64", _msg);
	sol_raw_int64_t retval = 0;
	for (int i = 8 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int64_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint64_t nd_uint64_t(const char* _msg){
	on_entry("uint64", _msg);
	sol_raw_uint64_t retval = 0;
	for (int i = 8 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint64_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int72_t nd_int72_t(const char* _msg){
	on_entry("int72", _msg);
	sol_raw_int72_t retval = 0;
	for (int i = 9 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int72_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint72_t nd_uint72_t(const char* _msg){
	on_entry("uint72", _msg);
	sol_raw_uint72_t retval = 0;
	for (int i = 9 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint72_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int80_t nd_int80_t(const char* _msg){
	on_entry("int80", _msg);
	sol_raw_int80_t retval = 0;
	for (int i = 10 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int80_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_uint80_t nd_uint80_t(const char* _msg){
	on_entry("uint80", _msg);
	sol_raw_uint80_t retval = 0;
	for (int i = 10 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint80_t)tryGetNextRandByte();
	}
	return retval;
}

sol_raw_int88_t nd_int88_t(const char* _msg){
	on_entry("int88", _msg);
	sol_raw_int88_t retval = 0;
	for (int i = 11 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int88_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint88_t nd_uint88_t(const char* _msg){
	on_entry("uint88", _msg);
	sol_raw_uint88_t retval = 0;
	for (int i = 11 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint88_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int96_t nd_int96_t(const char* _msg){
	on_entry("int96", _msg);
	sol_raw_int96_t retval = 0;
	for (int i = 12 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int96_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint96_t nd_uint96_t(const char* _msg){
	on_entry("uint96", _msg);
	sol_raw_uint96_t retval = 0;
	for (int i = 12 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint96_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int104_t nd_int104_t(const char* _msg){
	on_entry("int104", _msg);
	sol_raw_int104_t retval = 0;
	for (int i = 13 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int104_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint104_t nd_uint104_t(const char* _msg){
	on_entry("uint104", _msg);
	sol_raw_uint104_t retval = 0;
	for (int i = 13 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint104_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_int112_t nd_int112_t(const char* _msg){
	on_entry("int112", _msg);
	sol_raw_int112_t retval = 0;
	for (int i = 14 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int112_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint112_t nd_uint112_t(const char* _msg){
	on_entry("uint112", _msg);
	sol_raw_uint112_t retval = 0;
	for (int i = 14 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint112_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int120_t nd_int120_t(const char* _msg){
	on_entry("int120", _msg);
	sol_raw_int120_t retval = 0;
	for (int i = 15 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int120_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint120_t nd_uint120_t(const char* _msg){
	on_entry("uint120", _msg);
	sol_raw_uint120_t retval = 0;
	for (int i = 15 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint120_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_int128_t nd_int128_t(const char* _msg){
	on_entry("int128", _msg);
	sol_raw_int128_t retval = 0;
	for (int i = 16 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int128_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint128_t nd_uint128_t(const char* _msg){
	on_entry("uint128", _msg);
	sol_raw_uint128_t retval = 0;
	for (int i = 16 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint128_t)tryGetNextRandByte();
	}
	return retval;			
}

sol_raw_int136_t nd_int136_t(const char* _msg){
	on_entry("int136", _msg);
	sol_raw_int136_t retval = 0;
	for (int i = 17 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int136_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint136_t nd_uint136_t(const char* _msg){
	on_entry("uint136", _msg);
	sol_raw_uint136_t retval = 0;
	for (int i = 17 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint136_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_int144_t nd_int144_t(const char* _msg){
	on_entry("int144", _msg);
	sol_raw_int144_t retval = 0;
	for (int i = 18 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int144_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint144_t nd_uint144_t(const char* _msg){
	on_entry("uint144", _msg);
	sol_raw_uint144_t retval = 0;
	for (int i = 18 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint144_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int152_t nd_int152_t(const char* _msg){
	on_entry("int152", _msg);
	sol_raw_int152_t retval = 0;
	for (int i = 19 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int152_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint152_t nd_uint152_t(const char* _msg){
	on_entry("uint152", _msg);
	sol_raw_uint152_t retval = 0;
	for (int i = 19 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint152_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int160_t nd_int160_t(const char* _msg){
	on_entry("int160", _msg);
	sol_raw_int160_t retval = 0;
	for (int i = 20 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int160_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint160_t nd_uint160_t(const char* _msg){
	on_entry("uint160", _msg);
	sol_raw_uint160_t retval = 0;
	for (int i = 20 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint160_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int168_t nd_int168_t(const char* _msg){
	on_entry("int168", _msg);
	sol_raw_int168_t retval = 0;
	for (int i = 21 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int168_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint168_t nd_uint168_t(const char* _msg){
	on_entry("uint168", _msg);
	sol_raw_uint168_t retval = 0;
	for (int i = 21 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint168_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int176_t nd_int176_t(const char* _msg){
	on_entry("int176", _msg);
	sol_raw_int176_t retval = 0;
	for (int i = 22 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int176_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint176_t nd_uint176_t(const char* _msg){
	on_entry("uint176", _msg);
	sol_raw_uint176_t retval = 0;
	for (int i = 22 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint176_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_int184_t nd_int184_t(const char* _msg){
	on_entry("int184", _msg);
	sol_raw_int184_t retval = 0;
	for (int i = 23 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int184_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint184_t nd_uint184_t(const char* _msg){
	on_entry("uint184", _msg);
	sol_raw_uint184_t retval = 0;
	for (int i = 23 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint184_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int192_t nd_int192_t(const char* _msg){
	on_entry("int192", _msg);
	sol_raw_int192_t retval = 0;
	for (int i = 24 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int192_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint192_t nd_uint192_t(const char* _msg){
	on_entry("uint192", _msg);
	sol_raw_uint192_t retval = 0;
	for (int i = 24 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint192_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int200_t nd_int200_t(const char* _msg){
	on_entry("int200", _msg);
	sol_raw_int200_t retval = 0;
	for (int i = 25 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int200_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint200_t nd_uint200_t(const char* _msg){
	on_entry("uint200", _msg);
	sol_raw_uint200_t retval = 0;
	for (int i = 25 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint200_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_int208_t nd_int208_t(const char* _msg){
	on_entry("int208", _msg);
	sol_raw_int208_t retval = 0;
	for (int i = 26 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int208_t)tryGetNextRandByte();
	}
	return retval;	
}

sol_raw_uint208_t nd_uint208_t(const char* _msg){
	on_entry("uint208", _msg);
	sol_raw_uint208_t retval = 0;
	for (int i = 26 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint208_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int216_t nd_int216_t(const char* _msg){
	on_entry("int216", _msg);
	sol_raw_int216_t retval = 0;
	for (int i = 27 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int216_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint216_t nd_uint216_t(const char* _msg){
	on_entry("uint216", _msg);
	sol_raw_uint216_t retval = 0;
	for (int i = 27 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint216_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int224_t nd_int224_t(const char* _msg){
	on_entry("int224", _msg);
	sol_raw_int224_t retval = 0;
	for (int i = 28 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int224_t)tryGetNextRandByte();
	}
	return retval;			
}

sol_raw_uint224_t nd_uint224_t(const char* _msg){
	on_entry("uint224", _msg);
	sol_raw_uint224_t retval = 0;
	for (int i = 28 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint224_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int232_t nd_int232_t(const char* _msg){
	on_entry("int232", _msg);
	sol_raw_int232_t retval = 0;
	for (int i = 29 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int232_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint232_t nd_uint232_t(const char* _msg){
	on_entry("uint232", _msg);
	sol_raw_uint232_t retval = 0;
	for (int i = 29 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint232_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int240_t nd_int240_t(const char* _msg){
	on_entry("int240", _msg);
	sol_raw_int240_t retval = 0;
	for (int i = 30 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int240_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint240_t nd_uint240_t(const char* _msg){
	on_entry("uint240", _msg);
	sol_raw_uint240_t retval = 0;
	for (int i = 30 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint240_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int248_t nd_int248_t(const char* _msg){
	on_entry("int248", _msg);
	sol_raw_int248_t retval = 0;
	for (int i = 31 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int248_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint248_t nd_uint248_t(const char* _msg){
	on_entry("uint248", _msg);
	sol_raw_uint248_t retval = 0;
	for (int i = 31 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint248_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_int256_t nd_int256_t(const char* _msg){
	on_entry("int256", _msg);
	sol_raw_int256_t retval = 0;
	for (int i = 32 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_int256_t)tryGetNextRandByte();
	}
	return retval;		
}

sol_raw_uint256_t nd_uint256_t(const char* _msg){
	on_entry("uint256", _msg);
	sol_raw_uint256_t retval = 0;
	for (int i = 32 ; i > 0; i--){
		retval = retval << 8;
		retval = retval + (sol_raw_uint256_t)tryGetNextRandByte();
	}
	return retval;			
}