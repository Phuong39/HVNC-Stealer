QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 5): QT += widgets

CONFIG += c++11
QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++ -lstdc++ -lpthread
LIBS += -lws2_32

SOURCES += \
    libIP2Location/IP2Loc_DBInterface.c \
    libIP2Location/IP2Location.c \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    libIP2Location/IP2Loc_DBInterface.h \
    libIP2Location/IP2Location.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
