#/*
#* Copyright 2019 Rochus Keller <mailto:me@rochus-keller.ch>
#*
#* This file is part of the EbnfStudio application.
#*
#* The following is the license that applies to this copy of the
#* application. For a license to use the application under conditions
#* other than those described here, please email to me@rochus-keller.ch.
#*
#* GNU General Public License Usage
#* This file may be used under the terms of the GNU General Public
#* License (GPL) versions 2.0 or 3.0 as published by the Free Software
#* Foundation and appearing in the file LICENSE.GPL included in
#* the packaging of this file. Please review the following information
#* to ensure GNU General Public Licensing requirements will be met:
#* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
#* http://www.gnu.org/copyleft/gpl.html.
#*/

QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = EbnfStudio
TEMPLATE = app

CONFIG(debug, debug|release) {
        DEFINES += _DEBUG
}

QMAKE_CXXFLAGS += -Wno-reorder -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable

SOURCES += main.cpp\
        MainWindow.cpp \
    EbnfEditor.cpp \
    EbnfHighlighter.cpp \
    EbnfLexer.cpp \
    EbnfToken.cpp \
    EbnfSyntax.cpp \
    EbnfParser.cpp \
    EbnfErrors.cpp \
    EbnfAnalyzer.cpp \
    SynTreeGen.cpp \
    HtmlSyntax.cpp \
    SyntaxTreeMdl.cpp \
    GenUtils.cpp \
    CocoGen.cpp \
    FirstFollowSet.cpp \
    AntlrGen.cpp \
    LlgenGen.cpp

HEADERS  += MainWindow.h \
    EbnfEditor.h \
    EbnfHighlighter.h \
    EbnfLexer.h \
    EbnfToken.h \
    EbnfSyntax.h \
    EbnfParser.h \
    EbnfErrors.h \
    EbnfAnalyzer.h \
    SynTreeGen.h \
    HtmlSyntax.h \
    SyntaxTreeMdl.h \
    GenUtils.h \
    CocoGen.h \
    FirstFollowSet.h \
    AntlrGen.h \
    LlgenGen.h


!include(../NAF/Gui2/Gui2.pri) {
         message( "Missing NAF Gui2" )
 }

RESOURCES += \
    EbnfStudio.qrc
