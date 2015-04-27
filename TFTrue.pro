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
        SOURCE_DIR = $(HOME)/Documents/Projects/source-sdk-2013/mp/src
        STEAMWORKS_DIR = $(HOME)/Documents/Projects/opensteamworks

        DEFINES += \
                GNUC \
                POSIX \
                _LINUX \
                LINUX \
                RAD_TELEMETRY_DISABLED \
                NO_MALLOC_OVERRIDE
        QMAKE_CXXFLAGS = \
                -march=pentium3 -mmmx -msse -m32 -Wall -Werror \
                -fvisibility=hidden -fvisibility-inlines-hidden \
                -fno-strict-aliasing -Wno-delete-non-virtual-dtor -Wno-unused -Wno-reorder \
                -Wno-overloaded-virtual -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-sign-compare \
                -std=c++11 -Dtypeof=decltype -Wno-inconsistent-missing-override
        QMAKE_CXXFLAGS_RELEASE = -O3 -flto
        QMAKE_CXXFLAGS_DEBUG = -g3
        QMAKE_CXXFLAGS_SHLIB = # remove -fPIC
        QMAKE_CXXFLAGS_WARN_ON = # remove -Wall -W
        QMAKE_LFLAGS = \
                -lrt -m32 -Wl,--no-gnu-unique -fuse-ld=gold -static-libstdc++ -static-libgcc \
                -Wl,--version-script=$$PWD/version-script
        QMAKE_LFLAGS_RELEASE = -s -flto

        LIBS += \
                $${SOURCE_DIR}/lib/public/linux32/tier1.a \
                $${SOURCE_DIR}/lib/public/linux32/mathlib.a \
                $$PWD/ModuleScanner/ModuleScanner.a \
                $$PWD/FunctionRoute/FunctionRoute.a \
                -L$${STEAMWORKS_DIR}/redistributable_bin/linux32/ -lsteam_api \
                -L$$PWD/bin/ -ltier0_srv -lvstdlib_srv
}

win32 {
        SOURCE_DIR = E:/source-sdk-2013/mp/src

        DEFINES += \
                RAD_TELEMETRY_DISABLED \
                WIN32 \
                _WIN32 \
                _CRT_SECURE_NO_DEPRECATE \
                _CRT_NONSTDC_NO_DEPRECATE \
                _MBCS \
                RAD_TELEMETRY_DISABLED \
                _ALLOW_RUNTIME_LIBRARY_MISMATCH \
                _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH

        QMAKE_CXXFLAGS_RELEASE = -MT -O2 -fp:fast -Zi -Oy-
        QMAKE_CXXFLAGS_DEBUG = -MTd -Zi
        QMAKE_CXXFLAGS_WARN_ON = -W4 # Remove -W3 and some disabled warnings
        QMAKE_LFLAGS += -MERGE:".rdata=.text"
        QMAKE_LFLAGS_DEBUG += -NODEFAULTLIB:libcmt

        QMAKE_INCDIR += $$PWD

        LIBS += \
                $${SOURCE_DIR}/lib/public/tier0.lib \
                $${SOURCE_DIR}/lib/public/tier1.lib \
                $${SOURCE_DIR}/lib/public/tier2.lib \
                $${SOURCE_DIR}/lib/public/tier3.lib \
                $${SOURCE_DIR}/lib/public/mathlib.lib \
                $${STEAMWORKS_DIR}/redistributable_bin/steam_api.lib \
                $${SOURCE_DIR}/lib/public/vstdlib.lib \
                $$PWD/ModuleScanner/ModuleScanner.lib \
                $$PWD/FunctionRoute/FunctionRoute.lib
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

QMAKE_INCDIR += $${SOURCE_DIR}/common
QMAKE_INCDIR += $${SOURCE_DIR}/public
QMAKE_INCDIR += $${SOURCE_DIR}/public/tier0
QMAKE_INCDIR += $${SOURCE_DIR}/public/tier1
QMAKE_INCDIR += $${SOURCE_DIR}/game/shared
QMAKE_INCDIR += $${SOURCE_DIR}/game/server
QMAKE_INCDIR += $${STEAMWORKS_DIR}
QMAKE_INCDIR += ./FunctionRoute
QMAKE_INCDIR += ./ModuleScanner
