
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

win32:{
    DIR_PLATFORM = win64
}
unix:{
    DIR_PLATFORM = aarch64
}

LIBS += -L$$PWD/librime/$${DIR_PLATFORM}/lib/ -lrime

INCLUDEPATH += $$PWD/librime/$${DIR_PLATFORM}/include
DEPENDPATH += $$PWD/librime/$${DIR_PLATFORM}/include
