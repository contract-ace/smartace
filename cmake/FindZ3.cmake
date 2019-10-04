if (USE_Z3)
  if (Z3_HOME)
    find_path(Z3_INCLUDE_DIR NAMES z3++.h PATHS "${Z3_HOME}/include")
  else()
    find_path(Z3_INCLUDE_DIR NAMES z3++.h PATH_SUFFIXES z3)
  endif()
  if (Z3_HOME)
    find_library(Z3_LIBRARY z3 PATHS "${Z3_HOME}/lib")
  else()  
    find_library(Z3_LIBRARY NAMES z3)
  endif()
  if (Z3_HOME)
    find_program(Z3_EXECUTABLE z3 PATHS "${Z3_HOME}/bin")
  else()
    find_program(Z3_EXECUTABLE z3 PATH_SUFFIXES bin)
  endif()  

    if(Z3_INCLUDE_DIR AND Z3_LIBRARY AND Z3_EXECUTABLE)
        execute_process (COMMAND ${Z3_EXECUTABLE} -version
            OUTPUT_VARIABLE libz3_version_str
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        string(REGEX REPLACE "^Z3 version ([0-9.]+).*" "\\1"
               Z3_VERSION_STRING "${libz3_version_str}")
        unset(libz3_version_str)
    endif()
    mark_as_advanced(Z3_VERSION_STRING z3_DIR)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Z3
        REQUIRED_VARS Z3_LIBRARY Z3_INCLUDE_DIR
        VERSION_VAR Z3_VERSION_STRING)

    if (NOT TARGET Z3::Z3)
        add_library(Z3::Z3 UNKNOWN IMPORTED)
        set_property(TARGET Z3::Z3 PROPERTY IMPORTED_LOCATION ${Z3_LIBRARY})
        set_property(TARGET Z3::Z3 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Z3_INCLUDE_DIR})
    endif()
else()
    set(Z3_FOUND FALSE)
endif()
