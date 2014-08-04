# Determine platform- and compiler-specific settings

# compiler specific settings
if (CMAKE_COMPILER_IS_GNUCC)
   # add some more general preprocessor defines (only for gcc)
   message(STATUS "Using gcc-specific CXX flags")
   SET(GCC_COMPILE_FLAGS "-Wall -Wextra -g -Wno-deprecated -pedantic -Wno-long-long")
   SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}" )
   SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-inline -fdiagnostics-show-option -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef" CACHE STRING "Debug options." FORCE )
   SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -Wall"  CACHE STRING "Relwithdebinfo options." FORCE )
elseif( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
   message(STATUS "Using Clang-specific CXX flags")
   SET(GCC_COMPILE_FLAGS "-Wall -Wextra -g -Wno-deprecated -pedantic -Wno-long-long")
   SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}" )
   SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-inline -fdiagnostics-show-option -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-promo -Wswitch-default -Wundef" CACHE STRING "Debug options." FORCE )
   SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -Wall"  CACHE STRING "Relwithdebinfo options." FORCE )
elseif(MSVC)
   message(STATUS "Using MSVC-specific preprocessor identifiers and options")
   add_definitions(-DNOMINMAX) # disables the min/max macros
   add_definitions("/wd4275") # disables warning concerning dll-interface (comes up for std classes too often)
   add_definitions("/wd4251") # disables warning concerning dll-interface (comes up for std classes too often)
   add_definitions("/wd4800") # disables warning concerning casting done inside ROOT classes.
   add_definitions("/wd4996") # this compiler warnung is about that functions like fopen are unsafe.
else()
endif()
