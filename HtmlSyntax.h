#ifndef HTMLIMPORT_H
#define HTMLIMPORT_H

/*
* Copyright 2019 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the EbnfStudio application.
*
* The following is the license that applies to this copy of the
* application. For a license to use the application under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <QString>
#include "EbnfSyntax.h"

class QTextDocument;
class QTextStream;

class HtmlSyntax
{
public:
    HtmlSyntax();
    bool transform(const QString& inPath , const QString& outPath);
    static bool generateHtml(const QString& ebnfPath, EbnfSyntax*, bool all = true);
    QString d_error;
protected:
    bool reformat(const QTextDocument&, QTextStream& );
    typedef QList<const EbnfSyntax::Definition*> Stack;
    static void writeCompressedDefs(QTextStream& out, EbnfSyntax::Node* node,
                                    Stack& stack, int level, bool all );
};

#endif // HTMLIMPORT_H
