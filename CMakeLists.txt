cmake_minimum_required(VERSION 3.23)

project(aijsondb)

if (MSVC)
    # warning level 4
    add_compile_options(/experimental:c11atomics /std:c11 /bigobj)
endif()

set(ENV{TEST_DOMINO_DATA} ${PROJECT_SOURCE_DIR}/data)
message($ENV{TEST_DOMINO_DATA})

set (XINTL_DIR C:/Del/Excel/xlnt/build/installed/include)

include_directories(${PROJECT_SOURCE_DIR}/aijsondb  ${PROJECT_SOURCE_DIR}/third_party/quickjs-ng ${PROJECT_SOURCE_DIR}/third_party/jsoncons/include ${XINTL_DIR})

add_executable(aijsondbcli)

target_sources(aijsondbcli
  PRIVATE
    aijsondbcli/main.cpp
    aijsondb/aijsondblib.cpp
    aijsondb/aijsondbindex.cpp
    third_party/quickjs-ng/quickjs-amalgam.c
 "aijsondb/aijsondbresolver.cpp")

add_library( 
    aijsondbc   SHARED aijsondbc/aijsondbc.cpp aijsondb/aijsondblib.cpp aijsondb/aijsondbindex.cpp third_party/quickjs-ng/quickjs-amalgam.c 
 "aijsondb/aijsondbresolver.cpp")

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)



add_executable(tests)

set (PROJECT_LINK_LIBS xlntd.lib  )
set (PROJECT_LINK_DIR C:/Del/Excel/xlnt/build/installed/lib)

target_sources(tests
  PRIVATE
    tests/test.cpp
    aijsondb/aijsondblib.cpp
     aijsondb/aijsondbindex.cpp
    third_party/quickjs-ng/quickjs-amalgam.c
 "aijsondb/aijsondbresolver.cpp" "tests/test2.cpp")
target_compile_definitions(tests PUBLIC TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/")
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain ${PROJECT_LINK_DIR}/${PROJECT_LINK_LIBS})

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(tests)