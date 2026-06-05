QT       += core
QT       -= gui

TARGET = ebnfc
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += ..

CONFIG(debug, debug|release) {
    DEFINES += _DEBUG
}



!win32 {
    QMAKE_CXXFLAGS += -Wno-reorder -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-switch \
        -Wno-deprecated-declarations -Wno-sign-compare -Wno-parentheses -Wno-unused-parameter -Werror=return-type
}

SOURCES += \
    CppGen.cpp \
    EbnfAnalyzer.cpp \
    EbnfAnalyzer2.cpp \
    EbnfC.cpp \
    EbnfErrors.cpp \
    EbnfLexer.cpp \
    EbnfParser.cpp \
    EbnfSyntax.cpp \
    EbnfToken.cpp \
    FirstFollowSet.cpp \
    GenUtils.cpp \
    LaParser.cpp

HEADERS += \
    CppGen.h \
    EbnfAnalyzer.h \
    EbnfAnalyzer2.h \
    EbnfErrors.h \
    EbnfLexer.h \
    EbnfParser.h \
    EbnfSyntax.h \
    EbnfToken.h \
    EbnfVersion.h \
    FirstFollowSet.h \
    GenUtils.h \
    LaParser.h



