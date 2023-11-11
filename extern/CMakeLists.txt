# SPDX-License-Identifier: GPL-3.0-or-later

set(CMAKE_CXX_CLANG_TIDY "")

# Note that installing external software unasked involves a security risk and may violate the software's licenses. To protect vsgopenmw's users against OpenMW's ill-chosen defaults, fetching has been disabled here.
if(NOT OPENMW_USE_SYSTEM_BULLET)
    message(FATAL_ERROR "E_UNSUPPORTED(!OPENMW_USE_SYSTEM_BULLET)")
endif()
if(NOT OPENMW_USE_SYSTEM_RECASTNAVIGATION)
    message(FATAL_ERROR "E_UNSUPPORTED(!OPENMW_USE_SYSTEM_RECASTNAVIGATION)")
endif()
#if(NOT OPENMW_USE_SYSTEM_MYGUI)
#    message(FATAL_ERROR "E_UNSUPPORTED(!OPENMW_USE_SYSTEM_MYGUI)")
#endif()
if(NOT OPENMW_USE_SYSTEM_SQLITE3)
    message(FATAL_ERROR "E_UNSUPPORTED(!OPENMW_USE_SYSTEM_SQLITE3)")
endif()
if(NOT OPENMW_USE_SYSTEM_ICU)
    message(FATAL_ERROR "E_UNSUPPORTED(!OPENMW_USE_SYSTEM_ICU)")
endif()
if(BUILD_UNITTESTS AND NOT OPENMW_USE_SYSTEM_GOOGLETEST)
    message(FATAL_ERROR "E_UNSUPPORTED(!OPENMW_USE_SYSTEM_GOOGLETEST)")
endif()
if(NOT OPENMW_USE_SYSTEM_YAML_CPP)
    message(FATAL_ERROR "E_UNSUPPORTED(!OPENMW_USE_SYSTEM_YAML_CPP)")
endif()
if(BUILD_BENCHMARKS AND NOT OPENMW_USE_SYSTEM_BENCHMARK)
    message(FATAL_ERROR "E_UNSUPPORTED(!OPENMW_USE_SYSTEM_USE_SYSTEM_BENCHMARK)")
endif()

# Like `FetchContent_MakeAvailable` but passes EXCLUDE_FROM_ALL to `add_subdirectory`.
macro(FetchContent_MakeAvailableExcludeFromAll)
    foreach(contentName IN ITEMS ${ARGV})
        string(TOLOWER ${contentName} contentNameLower)
        FetchContent_GetProperties(${contentName})
        if(NOT ${contentNameLower}_POPULATED)
            FetchContent_Populate(${contentName})
            if(EXISTS ${${contentNameLower}_SOURCE_DIR}/CMakeLists.txt)
                add_subdirectory(${${contentNameLower}_SOURCE_DIR}
                    ${${contentNameLower}_BINARY_DIR} EXCLUDE_FROM_ALL)
            endif()
        endif()
    endforeach()
endmacro()

if (NOT OPENMW_USE_SYSTEM_VSG)
    include(FetchContent)
    FetchContent_Declare(vsg
            URL https://github.com/vsg-dev/VulkanSceneGraph/archive/refs/tags/v1.0.9.zip
            URL_HASH SHA3_256=d6a9cb1058fabf8c9174138e7c111b365e17c5a34468709144b7e765fb2c9d6e
            SOURCE_DIR fetched/vsg
    )
    FetchContent_MakeAvailableExcludeFromAll(vsg)
endif()

if (NOT OPENMW_USE_SYSTEM_VSGXCHANGE)
    include(FetchContent)
    FetchContent_Declare(vsgXchange
            URL https://github.com/vsg-dev/vsgXchange/archive/refs/tags/v1.0.5.zip
            URL_HASH SHA3_256=05fcbe8b0c4bde28d295f20b26ebcd7a6a4df540f459639e73a4c9db5b42ee20
            SOURCE_DIR fetched/vsgXchange
    )
    FetchContent_MakeAvailableExcludeFromAll(vsgXchange)
    add_dependencies(vsg vsgXchange)
endif()

if(NOT OPENMW_USE_SYSTEM_BULLET)
    cmake_minimum_required(VERSION 3.11) # for FetchContent

    set(BUILD_BULLET3 OFF CACHE BOOL "")
    set(BUILD_EXTRAS OFF CACHE BOOL "")
    set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "")
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "")
    set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "")
    set(BUILD_CLSOCKET OFF CACHE BOOL "")
    set(BUILD_ENET OFF CACHE BOOL "")
    set(BUILD_CPU_DEMOS OFF CACHE BOOL "")
    set(BUILD_EGL OFF CACHE BOOL "")

    set(USE_DOUBLE_PRECISION ON CACHE BOOL "")
    set(BULLET2_MULTITHREADING ON CACHE BOOL "")

    if(BULLET_STATIC)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    else()
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    endif()

    if(MSVC)
        # this setting is badly named - having it off forces the static runtime library,
        # but having it on does nothing, letting the defaults get used.
        # OpenMW uses the defaults, and you can't mix and match.
        set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
    endif()

    # May 7, 2021
    include(FetchContent)
    FetchContent_Declare(bullet
        URL https://github.com/bulletphysics/bullet3/archive/refs/tags/3.17.tar.gz
        URL_HASH SHA512=a5105bf5f1dd365a64a350755c7d2c97942f74897a18dcdb3651e6732fd55cc1030a096f5808cf50575281f05e3ac09aa50a48d271a47b94cd61f5167a72b7cc
        SOURCE_DIR fetched/bullet
    )
    FetchContent_MakeAvailableExcludeFromAll(bullet)

    set(BULLET_INCLUDE_DIRS ${bullet_SOURCE_DIR}/src PARENT_SCOPE)

    # The order here is important to work around a bug in Bullet:
    # https://github.com/bulletphysics/bullet3/issues/3233
    set(BULLET_LIBRARIES BulletCollision LinearMath PARENT_SCOPE)
endif()

if(NOT OPENMW_USE_SYSTEM_MYGUI)
    cmake_minimum_required(VERSION 3.11) # for FetchContent

    set(MYGUI_RENDERSYSTEM 4 CACHE STRING "")
    set(MYGUI_DISABLE_PLUGINS TRUE CACHE BOOL "")
    set(MYGUI_BUILD_DEMOS OFF CACHE BOOL "")
    set(MYGUI_BUILD_PLUGINS OFF CACHE BOOL "")
    set(MYGUI_BUILD_TOOLS OFF CACHE BOOL "")

    # We appear to be using some obsolete properties in the XML.
    # See https://gitlab.com/OpenMW/openmw/-/issues/5896
    set(MYGUI_DONT_USE_OBSOLETE OFF CACHE BOOL "")

    if(MYGUI_STATIC)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    else()
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    endif()

    # master on 13 Mar 2021
    include(FetchContent)
    FetchContent_Declare(mygui
        URL https://github.com/MyGUI/mygui/archive/refs/tags/MyGUI3.4.2.zip
        URL_HASH SHA256=9941c14479c162f467041303ae163a5f7c8d0c7761a261f972a89df3ca115350
        SOURCE_DIR fetched/mygui
    )
    FetchContent_MakeAvailableExcludeFromAll(mygui)

    set(MyGUI_INCLUDE_DIRS ${mygui_SOURCE_DIR}/MyGUIEngine/include PARENT_SCOPE)
    set(MyGUI_LIBRARIES MyGUIEngine PARENT_SCOPE)
endif()

#[[
if(NOT OPENMW_USE_SYSTEM_RECASTNAVIGATION)
    if(RECASTNAVIGATION_STATIC)
        set(BUILD_SHARED_LIBS OFF)
    else()
        set(BUILD_SHARED_LIBS ON)
    endif()

    set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "")
    set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "")
    set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "")

    # master on 12 Oct 2022
    include(FetchContent)
    FetchContent_Declare(recastnavigation
        URL https://github.com/recastnavigation/recastnavigation/archive/405cc095ab3a2df976a298421974a2af83843baf.zip
        URL_HASH SHA512=39580ca258783ab3bda237843facc918697266e729c85065cdae1f0ed3d0ed1429a7ed08b18b926ba64402d9875a18f52904a87f43fe4fe30252f23edcfa6c70
        SOURCE_DIR fetched/recastnavigation
    )
    FetchContent_MakeAvailableExcludeFromAll(recastnavigation)
endif()

if (NOT OPENMW_USE_SYSTEM_SQLITE3)
    include(FetchContent)
    FetchContent_Declare(sqlite3
        URL https://www.sqlite.org/2023/sqlite-amalgamation-3410100.zip
        URL_HASH SHA512=7de291709e88fffc693cf24ac675950cfc35c1bf7631cfea95167105720a05cf37fb943c57c5c985db2eeaa57b31894b3c0df98a7bd2939b5746fc5a24b5ae87
        SOURCE_DIR fetched/sqlite3
    )
    FetchContent_MakeAvailableExcludeFromAll(sqlite3)

    add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
    target_include_directories(sqlite3 INTERFACE ${sqlite3_SOURCE_DIR}/)
    if (UNIX)
        target_link_libraries(sqlite3 ${CMAKE_DL_LIBS})
    endif()
    add_library(SQLite::SQLite3 ALIAS sqlite3)

    set(SQLite3_INCLUDE_DIR ${sqlite3_SOURCE_DIR}/ PARENT_SCOPE)
    set(SQLite3_LIBRARY sqlite3 PARENT_SCOPE)
endif()

add_subdirectory(smhasher)

if (BUILD_BENCHMARKS AND NOT OPENMW_USE_SYSTEM_BENCHMARK)
    cmake_minimum_required(VERSION 3.11)

    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)

    include(FetchContent)
    FetchContent_Declare(benchmark
        URL https://github.com/google/benchmark/archive/refs/tags/v1.7.1.zip
        URL_HASH SHA512=bec4016263587a57648e02b094c69e838c0a21e16c3dcfc6f03100397ab8f95d5fab1f5fd0d7e0e8adbb8212fff1eb574581158fdda1fa7fd6ff12762154b0cc
        SOURCE_DIR fetched/benchmark
    )
    FetchContent_MakeAvailableExcludeFromAll(benchmark)
endif()

if (NOT OPENMW_USE_SYSTEM_YAML_CPP)
    if(YAML_CPP_STATIC)
        set(YAML_BUILD_SHARED_LIBS OFF)
    else()
        set(YAML_BUILD_SHARED_LIBS ON)
    endif()

    include(FetchContent)
    FetchContent_Declare(yaml-cpp
        URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.7.0.zip
        URL_HASH MD5=1e8ca0d6ccf99f3ed9506c1f6937d0ec
        SOURCE_DIR fetched/yaml-cpp
    )
    FetchContent_MakeAvailableExcludeFromAll(yaml-cpp)

    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16 AND MSVC)
        target_precompile_headers(yaml-cpp PRIVATE <algorithm>)
    endif()
endif()

if (NOT OPENMW_USE_SYSTEM_ICU)
    set(ICU_ENV "ICU_DATA_FILTER_FILE=${CMAKE_CURRENT_SOURCE_DIR}/icufilters.json")
    if (ANDROID)
        # Note: Must be a build directory, not an install root, since the configure script
        # looks for a configuration file which does not get installed.
        set(OPENMW_ICU_HOST_BUILD_DIR "" CACHE STRING "A pre-built ICU build directory for the host system if cross-compiling")
        if (OPENMW_ICU_HOST_BUILD_DIR STREQUAL "")
            message(FATAL_ERROR "If cross-compiling on android you must set the \
            OPENMW_ICU_HOST_BUILD_DIR to the path of a pre-compiled build of \
            ICU 70.1 for the system doing the build, as ICU needs to be able \
            to run its own executables as part of the build process.")
        endif()
        # We need a host version of ICU so that the tools can be run when building the data library.
        set(NDK_STANDARD_ROOT ${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64)
        string(REPLACE "android-" "" ANDROIDVER ${ANDROID_PLATFORM})
        # Wants a triple such as aarch64-linux-android, excluding a trailing
        # -clang etc.
        string(REGEX MATCH "^[^-]\+-[^-]+-[^-]+" ICU_TOOLCHAIN_NAME ${ANDROID_TOOLCHAIN_NAME})
        set(ICU_ENV
            ${ICU_ENV}
            "CC=${CMAKE_C_COMPILER_LAUNCHER} ${NDK_STANDARD_ROOT}/bin/${ICU_TOOLCHAIN_NAME}${ANDROIDVER}-clang"
            "CXX=${CMAKE_CXX_COMPILER_LAUNCHER} ${NDK_STANDARD_ROOT}/bin/${ICU_TOOLCHAIN_NAME}${ANDROIDVER}-clang"
            "RANLIB=${NDK_STANDARD_ROOT}/bin/${ICU_TOOLCHAIN_NAME}-ranlib"
            "AR=${NDK_STANDARD_ROOT}/bin/${ICU_TOOLCHAIN_NAME}-ar"
            "CPPFLAGS=${ANDROID_COMPILER_FLAGS}"
            "LDFLAGS=${ANDROID_LINKER_FLAGS} -lc -lstdc++"
            )
        set(ICU_ADDITIONAL_OPTS --disable-tools --host=${ICU_TOOLCHAIN_NAME}${ANDROIDVER} --with-cross-build=${OPENMW_ICU_HOST_BUILD_DIR})
    endif()
    include(ExternalProject)
    ExternalProject_Add(icu
        URL https://github.com/unicode-org/icu/archive/refs/tags/release-70-1.zip
        URL_HASH MD5=49d5e2e5bab93ae1a4b56e916150544c
        SOURCE_DIR fetched/icu
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env ${ICU_ENV}
            <SOURCE_DIR>/icu4c/source/configure --enable-static --disable-shared
            --disable-tests --disable-samples --disable-icuio --disable-extras ${ICU_ADDITIONAL_OPTS}
        BUILD_COMMAND make
        INSTALL_COMMAND ""
    )
    ExternalProject_Get_Property(icu SOURCE_DIR BINARY_DIR)
    set(ICU_INCLUDE_DIRS
        ${SOURCE_DIR}/icu4c/source/common
        ${SOURCE_DIR}/icu4c/source/i18n
        PARENT_SCOPE
    )
    foreach(ICULIB data uc i18n)
        add_library(ICU::${ICULIB} STATIC IMPORTED GLOBAL)
        set_target_properties(ICU::${ICULIB} PROPERTIES IMPORTED_LOCATION
            ${BINARY_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}icu${ICULIB}${CMAKE_STATIC_LIBRARY_SUFFIX})
        add_dependencies(ICU::${ICULIB} icu)
    endforeach()
    set(ICU_LIBRARIES ICU::i18n ICU::uc ICU::data PARENT_SCOPE)
endif()

if ((BUILD_UNITTESTS OR BUILD_OPENCS_TESTS) AND NOT OPENMW_USE_SYSTEM_GOOGLETEST)
    cmake_minimum_required(VERSION 3.11)

    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
        URL_HASH SHA512=1479ea2f3172c622c0ca305f5b2bc45a42941221ec0ac7865e6d6d020ec4d008d952fc64e01a4c5138d7bed4148cf75596f25bb9e9044a98bbbf5662053ea11c
        SOURCE_DIR fetched/googletest
    )
    if (MSVC)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()
    FetchContent_MakeAvailableExcludeFromAll(googletest)

    add_library(GTest::GTest ALIAS gtest)
    add_library(GMock::GMock ALIAS gmock)
endif()
]]
