# Attempts to find KLEE from the user provided KLEE_PATH.
# If no path is found, cmake will search for the command `klee`.
set(
    KLEE_PATH
    ""
    CACHE STRING
    "A path to the KLEE executable, klee."
)

if(KLEE_PATH)
    find_program(
        KLEE_EXE
        NAMES klee
        PATHS ${KLEE_PATH}
        NO_DEFAULT_PATH
    )
else()
    find_program(KLEE_EXE NAMES klee)
endif()

if(KLEE_EXE)
    message(STATUS "klee found: ${KLEE_EXE}")
else()
    message(WARNING "Failed to find klee. See -DKLEE_PATH.")
endif()

# Requires the KLEE include path from the user.
set(
    KLEE_LIB
    "/home/usea/klee/include"
    CACHE STRING
    "A path to the KLEE include directory."
)

if(EXISTS "${KLEE_LIB}")
    message(STATUS "klee include found: ${KLEE_LIB}")
else()
    message(WARNING "Failed to find klee include directory. See -DKLEE_LIB.")
endif()

# Locates llvm-link, as required by KLEE.
find_program(
    LLVM_LINK_EXE
    NAMES "llvm-link-10" "llvm-link-mp-10" "llvm-link"
    DOC "The linker for llvm(-10)."
)

if(LLVM_LINK_EXE)
    message(STATUS "llvm-link found: ${LLVM_LINK_EXE}")
else()
    message(WARNING "llvm-link not found, as required by KLEE.")
endif()

# If Klee is available, generates all Klee related targets.
set(KLEE_DRIVER_BC "klee_driver.bc")
set(KLEE_DEPS "")
list(APPEND KLEE_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/libverify/verify_klee.c")
foreach(fn ${EXE_HARNESSED_C})
    list(APPEND KLEE_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/${fn}")
endforeach(fn)
set(KLEE_BCS "")
foreach(fn ${KLEE_DEPS})
    get_filename_component(raw_fn "${fn}" NAME_WE)
    list(APPEND KLEE_BCS "${CMAKE_BINARY_DIR}/${raw_fn}.bc")
endforeach()

if(KLEE_EXE)
    # Target for Klee driver.
    if(KLEE_LIB AND LLVM_LINK_EXE)
        set(KLEE_FLAGS "")
        list(APPEND KLEE_FLAGS "-DMC_USE_STDINT")
        list(APPEND KLEE_FLAGS "-emit-llvm")
        list(APPEND KLEE_FLAGS "-c")
        list(APPEND KLEE_FLAGS "-g")
        list(APPEND KLEE_FLAGS "-O0")
        list(APPEND KLEE_FLAGS "-Xclang")
        list(APPEND KLEE_FLAGS "-disable-O0-optnone")

        add_custom_target(
            klee_driver
            COMMAND ${CMAKE_C_COMPILER} -I ${KLEE_LIB} ${KLEE_FLAGS} ${KLEE_DEPS}
            COMMAND ${LLVM_LINK_EXE} -f ${KLEE_BCS} -o ${KLEE_DRIVER_BC}
            COMMAND_EXPAND_LISTS
            SOURCES ${KLEE_DEPS}
        )
    endif()

    # Target for klee
    if(KLEE_EXE)
        set(KLEE_ARGS "")
        list (APPEND KLEE_ARGS "--simplify-sym-indices")
        list (APPEND KLEE_ARGS "--write-cvcs")
        list (APPEND KLEE_ARGS "--write-cov")
        list (APPEND KLEE_ARGS "--output-module")
        list (APPEND KLEE_ARGS "--max-memory=1000")
        list (APPEND KLEE_ARGS "--disable-inlining")
        list (APPEND KLEE_ARGS "--optimize")
        list (APPEND KLEE_ARGS "--use-forked-solver")
        list (APPEND KLEE_ARGS "--use-cex-cache")
        list (APPEND KLEE_ARGS "--libc=klee")
        list (APPEND KLEE_ARGS "--external-calls=all")
        list (APPEND KLEE_ARGS "--only-output-states-covering-new")
        list (APPEND KLEE_ARGS "--max-sym-array-size=4096")
        list (APPEND KLEE_ARGS "--max-time=60min")
        list (APPEND KLEE_ARGS "--watchdog")
        list (APPEND KLEE_ARGS "--max-memory-inhibit=false")
        list (APPEND KLEE_ARGS "--max-static-fork-pct=1")
        list (APPEND KLEE_ARGS "--max-static-solve-pct=1")
        list (APPEND KLEE_ARGS "--max-static-cpfork-pct=1")
        list (APPEND KLEE_ARGS "--switch-type=internal")
        list (APPEND KLEE_ARGS "--search=random-path")
        list (APPEND KLEE_ARGS "--search=nurs:covnew")
        list (APPEND KLEE_ARGS "--use-batching-search")
        list (APPEND KLEE_ARGS "--batch-instructions=10000")
        list (APPEND KLEE_ARGS "--silent-klee-assume")
        list (APPEND KLEE_ARGS "--max-forks=512")

        add_custom_target(
            symbex
            COMMAND ${KLEE_EXE} ${KLEE_ARGS} ${KLEE_DRIVER_BC}
            COMMAND_EXPAND_LISTS
        )
        add_dependencies(symbex klee_driver)
    endif()
endif()
