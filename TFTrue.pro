TEMPLATE = lib
CONFIG += plugin
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += no_plugin_name_prefix

TARGET = TFTrue

CONFIG(debug, debug|release) {
	DEFINES = DEBUG
} else {
	DEFINES = NDEBUG
}

unix {
        DEFINES += \
                GNUC \
                POSIX \
                _LINUX \
                LINUX \
                RAD_TELEMETRY_DISABLED \
                NO_MALLOC_OVERRIDE \
                VERSION_SAFE_STEAM_API_INTERFACES
        QMAKE_CXXFLAGS = \
                -march=pentium3 -mmmx -msse -m32 -Wall \
                -fvisibility=hidden -fvisibility-inlines-hidden \
                -fno-strict-aliasing -Wno-delete-non-virtual-dtor -Wno-unused -Wno-reorder \
                -Wno-overloaded-virtual -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-sign-compare \
                -std=c++11 -Dtypeof=decltype -Wno-class-memaccess -fpermissive
        QMAKE_CXXFLAGS_RELEASE = -O3 -flto
        QMAKE_CXXFLAGS_DEBUG = -g3
        QMAKE_CXXFLAGS_SHLIB = # remove -fPIC
        QMAKE_CXXFLAGS_WARN_ON = # remove -Wall -W
        QMAKE_LFLAGS = \
                -lrt -m32 -Wl,--no-gnu-unique -fuse-ld=gold -static-libstdc++ -static-libgcc \
                -Wl,--version-script=$$PWD/version-script
        QMAKE_LFLAGS_RELEASE = -s -flto

        LIBS += \
                $$PWD/source-sdk-2013/mp/src/lib/public/linux32/tier1.a \
                $$PWD/source-sdk-2013/mp/src/lib/public/linux32/mathlib.a \
                $$PWD/ModuleScanner/ModuleScanner.a \
                $$PWD/FunctionRoute/FunctionRoute.a \
                -L$$PWD/source-sdk-2013/mp/src/lib/public/linux32/ -lsteam_api \
                -L$$PWD/bin/ -ltier0_srv -lvstdlib_srv
}

win32 {
        DEFINES += \
                RAD_TELEMETRY_DISABLED \
                WIN32 \
                _WIN32 \
                _CRT_SECURE_NO_DEPRECATE \
                _CRT_NONSTDC_NO_DEPRECATE \
                _MBCS \
                RAD_TELEMETRY_DISABLED \
                _ALLOW_RUNTIME_LIBRARY_MISMATCH \
                _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH \
                VERSION_SAFE_STEAM_API_INTERFACES

        QMAKE_CXXFLAGS_RELEASE = -MT -O2 -fp:fast -Zi -Oy-
        QMAKE_CXXFLAGS_DEBUG = -MTd -Zi
        QMAKE_CXXFLAGS_WARN_ON = -W4 # Remove -W3 and some disabled warnings
        QMAKE_LFLAGS += -MERGE:".rdata=.text"
        QMAKE_LFLAGS_DEBUG += -NODEFAULTLIB:libcmt

        QMAKE_INCDIR += $$PWD

        LIBS += \
                $$PWD/source-sdk-2013/mp/src/lib/public/tier0.lib \
                $$PWD/source-sdk-2013/mp/src/lib/public/tier1.lib \
                $$PWD/source-sdk-2013/mp/src/lib/public/tier2.lib \
                $$PWD/source-sdk-2013/mp/src/lib/public/tier3.lib \
                $$PWD/source-sdk-2013/mp/src/lib/public/mathlib.lib \
                $$PWD/source-sdk-2013/mp/src/lib/public/steam_api.lib \
                $$PWD/source-sdk-2013/mp/src/lib/public/vstdlib.lib \
                $$PWD/ModuleScanner/ModuleScanner.lib \
                $$PWD/FunctionRoute/FunctionRoute.lib \
                legacy_stdio_definitions.lib
}

SOURCES += \
    AutoUpdater.cpp \
    items.cpp \
    MRecipient.cpp \
    TFTrue.cpp \
    utils.cpp \
    lib_json/json_reader.cpp \
    lib_json/json_value.cpp \
    lib_json/json_writer.cpp \
    stats.cpp \
    logs.cpp \
    bunnyhop.cpp \
    sourcetv.cpp \
    fov.cpp \
    tournament.cpp

HEADERS += \
    AutoUpdater.h \
    items.h \
    MRecipient.h \
    SDK.h \
    TFTrue.h \
    utils.h \
    lib_json/json_batchallocator.h \
    lib_json/json_tool.h \
    stats.h \
    logs.h \
    bunnyhop.h \
    sourcetv.h \
    fov.h \
    tournament.h \
    editablecommands.h

QMAKE_INCDIR += $$PWD/source-sdk-2013/mp/src/common
QMAKE_INCDIR += $$PWD/source-sdk-2013/mp/src/public
QMAKE_INCDIR += $$PWD/source-sdk-2013/mp/src/public/tier0
QMAKE_INCDIR += $$PWD/source-sdk-2013/mp/src/public/tier1
QMAKE_INCDIR += $$PWD/source-sdk-2013/mp/src/game/shared
QMAKE_INCDIR += $$PWD/source-sdk-2013/mp/src/game/server
QMAKE_INCDIR += ./FunctionRoute
QMAKE_INCDIR += ./ModuleScanner
