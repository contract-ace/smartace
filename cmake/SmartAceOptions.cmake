# Sets default configurations specific SmartACE.

macro(configure_smartace)
    set(INT_MODEL_STDINT "USE_STDINT")
    set(INT_MODEL_BOOST_MP "USE_BOOST_MP")
    set(INT_MODEL_OPTS ${INT_MODEL_STDINT} ${INT_MODEL_BOOST_MP})
    set(INT_MODEL_MSG "Select between boost::multiprecision (USE_BOOST_MP) and stdint.h (USE_STDINT).")
    set(INT_MODEL ${INT_MODEL_STDINT} CACHE STRING ${INT_MODEL_MSG})
    set_property(CACHE INT_MODEL PROPERTY STRINGS ${INT_MODEL_OPTS})

    if(INT_MODEL STREQUAL "${INT_MODEL_STDINT}")
        add_definitions(-DMC_USE_STDINT)
    elseif(INT_MODEL STREQUAL "${INT_MODEL_BOOST_MP}")
        add_definitions(-DMC_USE_BOOST_MP)
    else()
        message(FATAL_ERROR "Invalid integer model: ${INT_MODEL}")
    endif()
endmacro()
