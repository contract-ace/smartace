# Sets default configurations specific SmartACE.

macro(configure_smartace)
    set(
        INT_MODEL
        "USE_STDINT"
        CACHE STRING
        "Select between boost::multiprecision (USE_BOOST_MP) and stdint.h (USE_STDINT)."
    )

    if(INT_MODEL STREQUAL "USE_STDINT")
        add_definitions(-DMC_USE_STDINT)
    elseif(INT_MODEL STREQUAL "USE_BOOST_MP")
        add_definitions(-DMC_USE_BOOST_MP)
    else()
        message(FATAL_ERROR "Invalid integer model: ${INT_MODEL}")
    endif()
endmacro()
