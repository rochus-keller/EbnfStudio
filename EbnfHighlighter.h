#ifndef EBNFHIGHLIGHTER_H
#define EBNFHIGHLIGHTER_H

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

#include <QSyntaxHighlighter>
#include "EbnfToken.h"

class EbnfHighlighter : public QSyntaxHighlighter
{
public:
    enum { NonTermProp = QTextFormat::UserProperty };
    typedef QSet<EbnfToken::Sym> Keywords;
    EbnfHighlighter(QTextDocument* doc);
    void setKeywords( const Keywords& kw ) { d_kw = kw; }
    const Keywords& getKeywords() const { return d_kw; }
protected:
    // Override
    void highlightBlock( const QString & text );
private:
    enum Category { C_Ebnf, C_Cmt, C_Kw, C_Lit, C_Prod, C_Nt, C_Pred, C_Gray, C_Max };
    QTextCharFormat d_format[C_Max];
    Keywords d_kw;
};

#endif // EBNFHIGHLIGHTER_H
