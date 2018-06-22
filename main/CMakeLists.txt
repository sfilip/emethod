#cmake_minimum_required(VERSION 3.8)
cmake_minimum_required(VERSION 3.5)
set(PROJECT_NAME_STR arith25)
project(${PROJECT_NAME_STR})

# in order to switch between Debug/Release mode use the cmake option -DCFG_TYPE=<Debug/Release>
SET(CFG_TYPE "Release" CACHE STRING "Parameter to configure the build type (release, debug etc.)")
if(CFG_TYPE STREQUAL "Release")
      set(CMAKE_BUILD_TYPE Release)
      set(CMAKE_CXX_FLAGS_DEBUG "-Wall")
      message(STATUS "Main: build configuration set to Release")
else()
      set(CMAKE_BUILD_TYPE Debug)
      set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
      # set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -Wall")
      message(STATUS "Main: build configuration set to Debug")
endif()

# in order to switch between Debug/Release mode use the cmake option -DCFG_TYPE=<Debug/Release>
SET(CFG_FLOPOCO "Lib" CACHE STRING "Parameter to configure the build type for FloPoCo (Lib=only librariies are generated, LibExec=libraries and executables are generated, for generating the comparisons)")
if(CFG_FLOPOCO STREQUAL "Lib")
      message(STATUS "Main: build configuration for FloPoCo set to libraries only")
else()
      message(STATUS "Main: build configuration for FloPoCo set to libraries and executable")
endif()

add_definitions("-std=c++11")

# find Boost libraries
include(${PROJECT_SOURCE_DIR}/cmake/FindBoost.cmake)

# add the FloPoCo source/include directories
include_directories(${PROJECT_SOURCE_DIR}/../efrac 
	${PROJECT_SOURCE_DIR}/../flopoco/src)

# add the corresponding link directories for calling
link_directories(${PROJECT_SOURCE_DIR}/../efrac/build
	${PROJECT_SOURCE_DIR}/../flopoco )

add_subdirectory(${PROJECT_SOURCE_DIR}/../efrac ${PROJECT_SOURCE_DIR}/../efrac/build)
add_subdirectory(${PROJECT_SOURCE_DIR}/../flopoco ${PROJECT_SOURCE_DIR}/../flopoco/src)

get_directory_property(CFG_TYPE DIRECTORY ${PROJECT_SOURCE_DIR}/../efrac DEFINITION CFG_TYPE)
get_directory_property(CFG_TYPE DIRECTORY ${PROJECT_SOURCE_DIR}/../flopoco DEFINITION CFG_TYPE)

get_directory_property(CFG_TYPE DIRECTORY ${PROJECT_SOURCE_DIR}/../flopoco DEFINITION CFG_FLOPOCO)

file(GLOB emethodHW_CPP
    "*.cpp"
)

add_executable(emethodHW main.cpp ${emethodHW_CPP})

# add the corresponding link libraries
# target_link_libraries(emethodHW efrac gmp mpfr mpfi qsopt_ex ${FPLLL_LIBRARY})
#target_link_libraries(emethodHW efrac FloPoCoLib gmp gmpxx mpfr mpfi sollya qsopt_ex fplll)
target_link_libraries(emethodHW efrac FloPoCo gmp gmpxx mpfr mpfi ${SOLLYA_LIB} qsopt_ex ${FPLLL_LIBRARY} ${Boost_LIBRARIES})



# clean-up for the CMake generated files
add_custom_target(clean-cmake-files
   COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake/clean-cmake-files.cmake
)

add_custom_target(clean-run-files
   COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake/clean-run-files.cmake
)

add_custom_target(clean-all
   COMMAND ${CMAKE_BUILD_TOOL} clean
   COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake/clean-cmake-files.cmake
   COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake/clean-run-files.cmake
)