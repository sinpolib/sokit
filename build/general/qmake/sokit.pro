# ----------------------------------------------------
# sokit.pro
# ----------------------------------------------------

TEMPLATE = app
TARGET = sokit

QT += gui network
CONFIG += debug_and_release build_all thread
DEFINES += QT_NETWORK_LIB
INCLUDEPATH += . ./../../../tmp ./../../../src/sokit
DEPENDPATH += .
UI_DIR += ./../../../tmp
RCC_DIR += ./../../../tmp

win32 {
    DEFINES += QT_LARGEFILE_SUPPORT
    CONFIG += windows qt_static

    QMAKE_CFLAGS_MT =-MT
    QMAKE_CFLAGS_MT_DBG =-MTd
    QMAKE_CFLAGS_MT_DLL =-MD
    QMAKE_CFLAGS_MT_DLLDBG =-MDd
}

CONFIG(debug, debug|release) {
    DESTDIR = ../../../bin/debug
    MOC_DIR += ./../../../tmp/debug
    OBJECTS_DIR += ./../../../tmp/debug
    INCLUDEPATH += ./../../../tmp/debug

    QMAKE_CFLAGS_DEBUG = $$unique(QMAKE_CFLAGS_DEBUG)
    QMAKE_CXXFLAGS_DEBUG = $$unique(QMAKE_CFLAGS_DEBUG)

    CONFIG(qt_static) {
        QMAKE_CFLAGS_DEBUG -= $$QMAKE_CFLAGS_MT_DLLDBG
        QMAKE_CFLAGS_DEBUG += $$QMAKE_CFLAGS_MT_DBG
        QMAKE_CXXFLAGS_DEBUG -= $$QMAKE_CFLAGS_MT_DLLDBG
        QMAKE_CXXFLAGS_DEBUG += $$QMAKE_CFLAGS_MT_DBG
    } else {
        QMAKE_CFLAGS_DEBUG -= $$QMAKE_CFLAGS_MT_DBG
        QMAKE_CFLAGS_DEBUG += $$QMAKE_CFLAGS_MT_DLLDBG
        QMAKE_CXXFLAGS_DEBUG -= $$QMAKE_CFLAGS_MT_DBG
        QMAKE_CXXFLAGS_DEBUG += $$QMAKE_CFLAGS_MT_DLLDBG
    }
} else {
    DESTDIR = ../../../bin/release
    MOC_DIR += ./../../../tmp/release
    OBJECTS_DIR += ./../../../tmp/release
    INCLUDEPATH += ./../../../tmp/release

    QMAKE_CFLAGS_RELEASE = $$unique(QMAKE_CFLAGS_RELEASE)
    QMAKE_CXXFLAGS_RELEASE = $$unique(QMAKE_CXXFLAGS_RELEASE)

    CONFIG(qt_static) {
        QMAKE_CFLAGS_RELEASE -= $$QMAKE_CFLAGS_MT_DLL
        QMAKE_CFLAGS_RELEASE += $$QMAKE_CFLAGS_MT
        QMAKE_CXXFLAGS_RELEASE -= $$QMAKE_CFLAGS_MT_DLL
        QMAKE_CXXFLAGS_RELEASE += $$QMAKE_CFLAGS_MT
    } else {
        QMAKE_CFLAGS_RELEASE -= $$QMAKE_CFLAGS_MT
        QMAKE_CFLAGS_RELEASE += $$QMAKE_CFLAGS_MT_DLL
        QMAKE_CXXFLAGS_RELEASE -= $$QMAKE_CFLAGS_MT
        QMAKE_CXXFLAGS_RELEASE += $$QMAKE_CFLAGS_MT_DLL
    }
}

HEADERS += ../../../src/sokit/resource.h \
    ../../../src/sokit/setting.h \
    ../../../src/sokit/toolkit.h \
    ../../../src/sokit/baseform.h \
    ../../../src/sokit/clientform.h \
    ../../../src/sokit/clientskt.h \
    ../../../src/sokit/helpform.h \
    ../../../src/sokit/logger.h \
    ../../../src/sokit/main.h \
    ../../../src/sokit/notepadform.h \
    ../../../src/sokit/transferskt.h \
    ../../../src/sokit/transferform.h \
    ../../../src/sokit/serverskt.h \
    ../../../src/sokit/serverform.h
SOURCES += ../../../src/sokit/baseform.cpp \
    ../../../src/sokit/clientform.cpp \
    ../../../src/sokit/clientskt.cpp \
    ../../../src/sokit/helpform.cpp \
    ../../../src/sokit/logger.cpp \
    ../../../src/sokit/main.cpp \
    ../../../src/sokit/notepadform.cpp \
    ../../../src/sokit/serverform.cpp \
    ../../../src/sokit/serverskt.cpp \
    ../../../src/sokit/setting.cpp \
    ../../../src/sokit/toolkit.cpp \
    ../../../src/sokit/transferform.cpp \
    ../../../src/sokit/transferskt.cpp
FORMS += ../../../src/sokit/clientform.ui \
    ../../../src/sokit/helpform.ui \
    ../../../src/sokit/serverform.ui \
    ../../../src/sokit/transferform.ui
TRANSLATIONS += ../../../src/sokit/sokit.ts
RESOURCES += ../../../src/sokit/icons.qrc

QMAKE_PRE_LINK = lupdate ./sokit.pro
QMAKE_POST_LINK = lrelease ../../../src/sokit/sokit.ts -qm $$DESTDIR/sokit.lan

win32 {
    RC_FILE = ../../../src/sokit/sokit.rc
    LIBS += -lWs2_32 -lWinmm -lImm32
    QMAKE_LFLAGS_DEBUG += /PDB:"$$DESTDIR/sokit.pdb"
    QMAKE_CFLAGS_DEBUG += /Fd"$$OBJECTS_DIR/sokit.pdb"
    QMAKE_CXXFLAGS_DEBUG += /Fd"$$OBJECTS_DIR/sokit.pdb"

   CONFIG(qt_static) {
        exists( $(QTDIR)/lib/static ) {
	   QMAKE_LIBDIR_QT = $(QTDIR)/lib/static 
       }
    } else {
        exists( $(QTDIR)/lib/shared ) {
	   QMAKE_LIBDIR_QT = $(QTDIR)/lib/shared 
       }
    }
}

