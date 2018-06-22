#cmake_minimum_required(VERSION 3.8)
cmake_minimum_required(VERSION 3.5)
set(PROJECT_NAME_STR efraclib-build)
project(${PROJECT_NAME_STR})

list (INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)

find_package(Eigen3 REQUIRED)
find_package(GMP REQUIRED)
find_package(MPFR REQUIRED)
find_package(FPLLL REQUIRED)
find_package(MPREAL REQUIRED)
find_package(QSOPT REQUIRED)

# in order to switch between Debug/Release mode use the cmake option -DCFG_TYPE=<Debug/Release>
# this is normally passed on by the main configuration file
if(CFG_TYPE STREQUAL "Release")
      set(CMAKE_BUILD_TYPE Release)
      set(CMAKE_CXX_FLAGS_DEBUG "-Wall")
      message(STATUS "EFrac: build configuration set to Release")
else()
      set(CMAKE_BUILD_TYPE Debug)
      set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
      # set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -Wall")
      message(STATUS "EFrac: build configuration set to Debug")
endif()

add_definitions("-std=c++11")

file(GLOB PROJECT_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
include_directories(${PROJECT_SOURCE_DIR})

add_library(efrac SHARED ${PROJECT_SRC_FILES})
# add_library(efrac STATIC ${PROJECT_SRC_FILES})

target_link_libraries(efrac gmp mpfr qsopt_ex fplll)

install(TARGETS efrac DESTINATION lib)
install(DIRECTORY ${PROJECT_SRC_DIR} DESTINATION include)

