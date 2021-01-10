# Attempts to find Seahorn from the user provided SEA_PATH.
# If no path is found, cmake will search for the command `sea`.
set(SEA_PATH "" CACHE STRING "A path to the Seahorn executable, sea.")

if(SEA_PATH)
    find_program(
	    SEA_EXE
	    NAMES sea
        PATHS ${SEA_PATH}
        NO_DEFAULT_PATH
    )
else()
    find_program(SEA_EXE NAMES sea)
endif()

if(SEA_EXE)
    message(STATUS "sea found: ${SEA_EXE}")
else()
    message(WARNING "Failed to find sea. See -DSEA_PATH.")
endif()

# Attempts to find llvm-dis to aid humans in interpreting invariants.
find_program(
    LLVM_DIS_EXE
    NAMES "llvm-dis-10" "llvm-dis-mp-10" "llvm-dis"
    DOC "The disassembler for LLVM(-10)"
)

if(LLVM_DIS_EXE)
    message(STATUS "llvm-dis found: ${LLVM_DIS_EXE}")
else()
    message(WARNING "Failed to find llvm-dis. See -DLLVM_DIS_PATH.")
endif()

# Locates the model files requried by Seahorn.
set(SEAHORN_DEPS "")
list(APPEND SEAHORN_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/libverify/verify_seahorn.c")
foreach(fn ${EXE_HARNESSED_C})
    list(APPEND SEAHORN_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/${fn}")
endforeach(fn)

# Locates all libverify(Seahorn) dependancies, and forwards all compiler flags.
set(CMODEL_COMPILE_DEFS "")
get_directory_property(CMODEL_COMPILE_DEFS_RAW DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
foreach(d ${CMODEL_COMPILE_DEFS_RAW})
    list(APPEND CMODEL_COMPILE_DEFS "-D${d}")
endforeach(d)
list(APPEND CMODEL_COMPILE_DEFS "-DMC_USE_EXTERNAL_NONDET")

# Sets solver mode.
set(HORN_SOLVER_LIA "lia")
set(HORN_SOLVER_NLIA "nlia")
set(HORN_SOLVER_BV "bv")
set(HORN_SOLVER_MODES ${HORN_SOLVER_LIA} ${HORN_SOLVER_NLIA} ${HORN_SOLVER_BV})
set(HORN_SOLVER_MSG "Theories available to spacer (lia/nlia/bv).")
set(SEA_HORN_SOLVER ${HORN_SOLVER_LIA} CACHE STRING ${HORN_SOLVER_MSG})
set_property(CACHE SEA_HORN_SOLVER PROPERTY STRINGS ${HORN_SOLVER_MODES})

# Configures YAML files.
set(SEA_YAML "${CMAKE_CURRENT_SOURCE_DIR}/yaml/sea.common.yaml")
set(CEX_YAML "${CMAKE_CURRENT_SOURCE_DIR}/yaml/sea.cex.yaml")
set(NLIA_YAML "${CMAKE_CURRENT_SOURCE_DIR}/yaml/sea.nlia.yaml")
set(BV_YAML "${CMAKE_CURRENT_SOURCE_DIR}/yaml/sea.bv.yaml")

set(SEA_COMMON_YAMA "")
list(APPEND SEA_COMMON_YAMA "-y" "${SEA_YAML}")
if(SEA_HORN_SOLVER STREQUAL HORN_SOLVER_NLIA)
    list(APPEND SEA_COMMON_YAMA "-y" "${NLIA_YAML}")
elseif(SEA_HORN_SOLVER STREQUAL HORN_SOLVER_BV)
    list(APPEND SEA_COMMON_YAMA "-y" "${BV_YAML}")
endif()

set(SEA_CEX_YAMA "${SEA_COMMON_YAMA}")
list(APPEND SEA_CEX_YAMA "-y" "${CEX_YAML}")

# Handles additional arguments, if provided.
set(SEA_ARGS "" CACHE STRING "Additional arguments to pass to Seahorn.")

# If all dependancies were located, adds all Seahorn targets.
if(SEA_EXE)
    # Merges arguments to sea.
    set(SEA_FULL_ARGS "")
    list(APPEND SEA_FULL_ARGS ${CMODEL_COMPILE_DEFS})
    list(APPEND SEA_FULL_ARGS ${SEA_ARGS})

    # Adds pipeline to produce optimized LLVM bytecode and dot diagram.
    if(LLVM_DIS_EXE)
        set(SEA_INSPECT_TEMP_REL "sea_temps")
        set(SEA_INSPECT_TEMP_ABS "${CMAKE_BINARY_DIR}/${SEA_INSPECT_TEMP_REL}")
        set(SEA_FINAL_BC "merged.pp.ms.o.bc")
        set(SEA_FINAL_LL "merged.pp.ms.o.ll")
        add_custom_command(
            OUTPUT "${SEA_INSPECT_TEMP_REL}/${SEA_FINAL_BC}"
            COMMAND ${SEA_EXE} yama ${SEA_COMMON_YAMA} pf ${SEAHORN_DEPS} ${SEA_FULL_ARGS} --save-temps --temp-dir ${SEA_INSPECT_TEMP_ABS}
            DEPENDS ${SEAHORN_DEPS}
            COMMAND_EXPAND_LISTS
        )
        add_custom_command(
            OUTPUT ${SEA_FINAL_LL}
            COMMAND ${LLVM_DIS_EXE} "${SEA_INSPECT_TEMP_ABS}/${SEA_FINAL_BC}" -o "${CMAKE_BINARY_DIR}/${SEA_FINAL_LL}"
            DEPENDS "${SEA_INSPECT_TEMP_ABS}/${SEA_FINAL_BC}"
        )
        add_custom_target(
            sea_inspect
            COMMAND ${SEA_EXE} inspect --cfg-dot "${CMAKE_BINARY_DIR}/${SEA_FINAL_LL}"
            SOURCES "${CMAKE_BINARY_DIR}/${SEA_FINAL_LL}"
        )
    endif()

    add_custom_target(
        verify
        COMMAND ${SEA_EXE} yama ${SEA_COMMON_YAMA} pf ${SEAHORN_DEPS} ${SEA_FULL_ARGS} --show-invars
        SOURCES ${SEAHORN_DEPS}
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(
        cex
        COMMAND ${SEA_EXE} yama ${SEA_CEX_YAMA} pf ${SEAHORN_DEPS} ${SEA_FULL_ARGS} --cex=cex.ll
        SOURCES ${SEAHORN_DEPS}
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(
        witness
        COMMAND ${SEA_EXE} yama ${SEA_CEX_YAMA} exe-cex ${SEAHORN_DEPS} ${SEA_FULL_ARGS} -DMC_LOG_ALL -o witness
        COMMAND_EXPAND_LISTS
    )
endif()
