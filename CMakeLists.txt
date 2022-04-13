cmake_minimum_required(VERSION 3.1)

project(bitcoinstats)

#
# common directories
#

set(bitcoinstats_ROOT        ${PROJECT_SOURCE_DIR})
set(bitcoinstats_EXT_ROOT    ${bitcoinstats_ROOT}/ext)


if (WIN32)
set(extra_INCLUDE    ${bitcoinstats_ROOT}/ext)
else()
set(extra_INCLUDE    ${bitcoinstats_ROOT}/ext)
endif()

#
# common compiler settings
#

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

    #add_compile_options(-std=c++11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

    # enable all warnings and treat warnings as errors
    add_compile_options(-Wall -Werror -pedantic -Wno-unused-function)

    # always enable debugging symbols
    add_compile_options(-g)

elseif (MSVC)

    # generic preprocessor definitions
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DLEVELDB_COMPILE_LIBRARY)
    add_definitions(-DLEVELDB_PLATFORM_WINDOWS=1)
    add_definitions(-DLEVELDB_IS_BIG_ENDIAN=0)
    add_definitions(-DHAVE_CRC32C=0)
    add_definitions(-DHAVE_SNAPPY=1)
    add_definitions(-DHAVE_FULLFSYNC=0)
    add_definitions(-DHAVE_FDATASYNC=0)
    add_definitions(-DHAVE_CONFIG_H=1)

    # config-specific preprocessor definitions
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=2>
    )
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
        $<$<CONFIG:Release>:NDEBUG>
    )
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
        $<$<CONFIG:RelWithDebInfo>:NDEBUG>
        $<$<CONFIG:RelWithDebInfo>:DBGREL>
    )

    # enable all warnings and treat warnings as errors
    add_compile_options(/W4 /WX)

    # set static runtime libs in all configurations
    # enable multiprocessor build in release config
    add_compile_options(
        $<$<CONFIG:Release>:/MT>
        $<$<CONFIG:Release>:/MP>
    )
    add_compile_options(
        $<$<CONFIG:Debug>:/MTd>
    )
    add_compile_options(
        $<$<CONFIG:RelWithDebInfo>:/MT>
        $<$<CONFIG:RelWithDebInfo>:/MP>
        $<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>
        $<$<CONFIG:RelWithDebInfo>:/Zi>
        $<$<CONFIG:RelWithDebInfo>:/Zo>
    )
endif()

#
# sources
#

file (GLOB_RECURSE leveldb_EXTERNAL_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    include/*.h
    src/*.cpp
    include/leveldb/*.h
    include /snappy/*.h
    src/leveldb/util/*.h
    src/leveldb/util/*.cc
    src/leveldb/db/*.h
    src/leveldb/db/*.cc
    src/leveldb/table/*.h
    src/leveldb/table/*.cc
    src/leveldb/port/*.h
    src/leveldb/port/*.cc
    src/snappy/config.h
    src/snappy/snappy-c.cc
    src/snappy/snappy-internal.h
    src/snappy/snappy-sinksource.cc
    src/snappy/src/snappy/snappy-stubs-internal.cc
    src/snappy/snappy-stubs-internal.h
    src/snappy/snappy.cc
)


set(Shared_SOURCES
)


if (WIN32)
set(Platform_SOURCES
)
else()
set(Platform_SOURCES
)
endif()

set(bitcoinstats_SOURCES
	app/main.cpp
)

#message("External sources:\n${bitcoinstats_EXTERNAL_SOURCES}")

# deal with subdirectories in external sources
foreach(source IN LISTS bitcoinstats_EXTERNAL_SOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

#message("External sources:\n${leveldb_EXTERNAL_SOURCES}")

# deal with subdirectories in external sources
foreach(source IN LISTS leveldb_EXTERNAL_SOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

# deal with subdirectories in sources
foreach(source IN LISTS bitcoinstats_SOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

#
# executable target
#

add_executable(bitcoinstats
    ${bitcoinstats_EXTERNAL_SOURCES}
    ${leveldb_EXTERNAL_SOURCES}
    ${bitcoinstats_SOURCES}
    ${Platform_SOURCES}
    ${Shared_SOURCES}
)

target_include_directories(bitcoinstats PUBLIC
    ${bitcoinstats_EXT_ROOT}
    ${bitcoinstats_EXT_ROOT}/bitcoinstats
    ${bitcoinstats_ROOT}/include
    ${bitcoinstats_ROOT}/include/snappy
    ${bitcoinstats_ROOT}/src/leveldb
    ${bitcoinstats_ROOT}/src/snappy
    ${extra_INCLUDE}
)

if (WIN32)
    target_link_libraries(bitcoinstats
    )
else()
    target_link_libraries(bitcoinstats
        -ldl
        -lpthread
    )
endif()


set(bitcoinstats_BIN_DIR ${bitcoinstats_ROOT}/bin)
if (bitcoinstats_BUILD_PLATFORM)
    set(bitcoinstats_BIN_DIR ${bitcoinstats_BIN_DIR}/${bitcoinstats_BUILD_PLATFORM})
endif()
if (CMAKE_BUILD_TYPE)
    set(bitcoinstats_BIN_DIR ${bitcoinstats_BIN_DIR}/${CMAKE_BUILD_TYPE})
endif()

set_target_properties(bitcoinstats
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${bitcoinstats_BIN_DIR}
)
