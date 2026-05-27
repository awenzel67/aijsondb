cmake_minimum_required(VERSION 3.23)

project(aijsondb)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(IGNORE_ME ${CMAKE_C_COMPILER}) # Suppress CMake warning message

# ============================================================================
# Configure OpenXLSX
# ============================================================================
set(OPENXLSX_CREATE_DOCS           OFF)
set(OPENXLSX_BUILD_SAMPLES         OFF)
if (MSVC)
set(         BUILD_SHARED_LIBS     OFF)
else()
set(         BUILD_SHARED_LIBS     OFF)
endif()

set ( CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory( OpenXLSX )


if (MSVC)
    # warning level 4
    add_compile_options(/experimental:c11atomics /std:c11 /bigobj)
else()
   add_compile_options(-fPIC)
endif()

set(ENV{TEST_DOMINO_DATA} ${PROJECT_SOURCE_DIR}/data)
message($ENV{TEST_DOMINO_DATA})

include_directories(${PROJECT_SOURCE_DIR}/aijsondb  ${PROJECT_SOURCE_DIR}/third_party/quickjs-ng ${PROJECT_SOURCE_DIR}/third_party/jsoncons/include ${PROJECT_SOURCE_DIR}/third_party/bw-tempdir)


add_library( 
    aijsondbexcellib   
    STATIC 
   #aijsondbexcel/aijsondbimportexcel.cpp
   aijsondbexcel/aijsondbimportexcelplus.cpp 
   aijsondbexcel/excelhelper.cpp
   aijsondbexcel/chrono_helper.cpp
)
target_link_libraries(aijsondbexcellib PRIVATE OpenXLSX::OpenXLSX)
target_include_directories(aijsondbexcellib PRIVATE ${OpenXLSX_INCLUDES})
set_target_properties(aijsondbexcellib PROPERTIES
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

add_executable(aijsondbcli)

target_sources(aijsondbcli
  PRIVATE
    aijsondbcli/main.cpp
    aijsondb/aijsondblib.cpp
    aijsondb/aijsondbindex.cpp
    third_party/quickjs-ng/quickjs-amalgam.c
 "aijsondb/aijsondbresolver.cpp" "aijsondb/aijsondbloadwithimporter.cpp")

target_link_libraries(aijsondbcli PRIVATE aijsondbexcellib)
target_include_directories(aijsondbcli PRIVATE ${OpenXLSX_INCLUDES})
set_target_properties(aijsondbcli PROPERTIES
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

add_library( 
    aijsondbc   SHARED aijsondbc/aijsondbc.cpp aijsondb/aijsondblib.cpp aijsondb/aijsondbindex.cpp third_party/quickjs-ng/quickjs-amalgam.c 
 "aijsondb/aijsondbresolver.cpp" "aijsondb/aijsondbloadwithimporter.cpp")

target_link_libraries(aijsondbc PRIVATE aijsondbexcellib)


Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)



add_executable(aijsontests)

#set (PROJECT_LINK_LIBS OpenXLSXd.lib  )
#set (PROJECT_LINK_DIR C:/Del/Excel/fff/lib)

#message(${OpenXLSX_INCLUDES})

target_sources(aijsontests
  PRIVATE
    tests/test.cpp
    tests/testxlsx.cpp
    aijsondb/aijsondblib.cpp
    aijsondb/aijsondbindex.cpp
    third_party/quickjs-ng/quickjs-amalgam.c
 "aijsondb/aijsondbresolver.cpp" "aijsondb/aijsondbloadwithimporter.cpp"
  #  aijsondb/aijsondblib.cpp
  #   aijsondb/aijsondbindex.cpp
  #  third_party/quickjs-ng/quickjs-amalgam.c
 #"aijsondb/aijsondbresolver.cpp" 
 #"aijsondb/aijsondbloadwithimporter.cpp"
)
target_compile_definitions(aijsontests PUBLIC TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/")
#target_link_libraries(aijsontests PRIVATE Catch2::Catch2WithMain ${PROJECT_LINK_DIR}/${PROJECT_LINK_LIBS} ${PROJECT_LINK_DIR}/miniz.lib ${PROJECT_LINK_DIR}/nowide.lib ${PROJECT_LINK_DIR}/pugixml.lib)
target_link_libraries(aijsontests PRIVATE Catch2::Catch2WithMain aijsondbexcellib)
#target_include_directories(aijsontests PRIVATE ${OpenXLSX_INCLUDES})


list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(aijsontests)