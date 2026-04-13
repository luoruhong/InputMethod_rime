
FORMS += \
        $$PWD/keyboard.ui

RESOURCES += \
    $$PWD/dict.qrc

HEADERS += \
    $$PWD/keyboard.h \
    $$PWD/keybutton.h \
    $$PWD/rimeutils.h \

SOURCES += \
    $$PWD/keyboard.cpp \
    $$PWD/keybutton.cpp \
    $$PWD/rimeutils.cpp \

win32 {
    LIBS += -L$$PWD/librime/win64/lib/ -lrime
    INCLUDEPATH += $$PWD/librime/win64/include
    DEPENDPATH += $$PWD/librime/win64/include
}

#apt install librime-dev
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += rime
}