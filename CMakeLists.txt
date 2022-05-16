cmake_minimum_required(VERSION 3.16)
project(TFTrue)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(TFTrue SHARED
    AutoUpdater.cpp AutoUpdater.h
    MRecipient.cpp  MRecipient.h
    SDK.h
    TFTrue.cpp      TFTrue.h
    bunnyhop.cpp    bunnyhop.h
    editablecommands.h
    fov.cpp         fov.h
    items.cpp       items.h
    lib_json/json_batchallocator.h
    lib_json/json_reader.cpp
    lib_json/json_tool.h
    lib_json/json_value.cpp
    lib_json/json_writer.cpp
    logs.cpp        logs.h
    sourcetv.cpp    sourcetv.h
    stats.cpp       stats.h
    tournament.cpp  tournament.h
    utils.cpp       utils.h
)
set_target_properties(TFTrue PROPERTIES PREFIX "")

target_include_directories(TFTrue PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/common
    ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/public
    ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/public/tier0
    ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/public/tier1
    ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/game/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/game/server

    ${CMAKE_CURRENT_SOURCE_DIR}/FunctionRoute
    ${CMAKE_CURRENT_SOURCE_DIR}/ModuleScanner
)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(TFTrue PUBLIC
        DEBUG
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
    target_compile_definitions(TFTrue PUBLIC
        NDEBUG

    )
endif()

set(CMAKE_BUILD_TYPE RelWithDebInfo)

if(UNIX)
    target_compile_definitions(TFTrue PRIVATE
        GNUC
        LINUX
        NO_MALLOC_OVERRIDE
        POSIX
        RAD_TELEMETRY_DISABLED
        VERSION_SAFE_STEAM_API_INTERFACES
        _LINUX
        typeof=decltype
    )

    target_link_libraries(TFTrue PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/FunctionRoute/FunctionRoute.a
        ${CMAKE_CURRENT_SOURCE_DIR}/ModuleScanner/ModuleScanner.a
        ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/linux/mathlib_i486.a
        ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/linux/tier1_i486.a
        "-L${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-2013/lib/public/linux32/" libsteam_api.so
        "-L${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/linux/"           libtier0_srv.so
        "-L${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/linux/"           libvstdlib_srv.so
    )

    target_compile_options(TFTrue PUBLIC
        -Wall
        -Wno-packed-not-aligned
        -Wno-class-memaccess
        -Wno-delete-non-virtual-dtor
        -Wno-invalid-offsetof
        -Wno-overloaded-virtual
        -Wno-reorder
        -Wno-sign-compare
        -Wno-unknown-pragmas
        -Wno-unused
        -fno-strict-aliasing
        -fpermissive
        -fvisibility-inlines-hidden
        -fvisibility=hidden
        -m32
        -march=pentium3
        -mmmx
        -msse
        -std=c++11
        # https://github.com/alliedmodders/sourcemod/blob/1fbe5e1daaee9ba44164078fe7f59d862786e612/public/sample_ext/AMBuildScript#L161-L164
        -Dstrcmpi=stricmp
        -Dstricmp=strcasecmp
        -D_stricmp=strcasecmp
        -D_snprintf=snprintf
        -D_vsnprintf=vsnprintf
    )

    target_link_options(TFTrue PUBLIC
        # -fno-builtin
        -zmuldefs
        # -z muldefs
        -lrt
        -m32
        # -v
        # -lm
        # -ldl
        #--no-as-needed
        #-no-undefined
        # -Wl,--no-gnu-unique
        -fuse-ld=gold
        # -static-libstdc++
        # -static-libgcc
        -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/version-script
    )

    if(CMAKE_BUILD_TYPE STREQUAL Release)
        target_compile_options(TFTrue PUBLIC
            -O3
            -flto
        )

        target_link_options(TFTrue PUBLIC
            -s
            -flto
        )
    endif()
endif()

if(WIN32)
    target_compile_definitions(TFTrue PUBLIC
        RAD_TELEMETRY_DISABLED RAD_TELEMETRY_DISABLED
        VERSION_SAFE_STEAM_API_INTERFACES
        WIN32
        _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH
        _ALLOW_RUNTIME_LIBRARY_MISMATCH
        _CRT_NONSTDC_NO_DEPRECATE
        _CRT_SECURE_NO_DEPRECATE
        _MBCS
        _WIN32
    )

    set_property(TARGET TFTrue PROPERTY
             MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    target_link_libraries(TFTrue PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/FunctionRoute/FunctionRoute.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/ModuleScanner/ModuleScanner.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/public/mathlib.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/public/steam_api.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/public/tier0.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/public/tier1.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/public/tier2.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/public/tier3.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/hl2sdk-tf2/lib/public/vstdlib.lib
        legacy_stdio_definitions.lib
    )

    target_link_options(TFTrue PUBLIC
        -OPT:REF
    )
endif()
