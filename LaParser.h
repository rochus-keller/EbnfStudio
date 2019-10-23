#ifndef LAPARSER_H
#define LAPARSER_H
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

#include <QByteArray>
#include <QSharedData>

class LaLexer
{
public:
    struct Tok
    {
        enum { Invalid, Literal, Ident, Or, And, Not, LPar, RPar, Eof };
        quint8 d_type;
        QByteArray d_val;
        Tok(quint8 t = Invalid, const QByteArray& v = QByteArray() ):d_type(t),d_val(v){}
    };
    LaLexer();
    void setStr( const QByteArray& );
    Tok nextToken();
    Tok peekToken(quint8 lookAhead = 1);
protected:
    Tok nextTokenImp();
    int skipWhiteSpace();
    Tok ident();
    Tok literal();
    Tok token( quint8 type, int len = 1, const QByteArray& v = QByteArray() );
private:
    QByteArray d_str;
    QList<Tok> d_buffer;
    int d_pos;
};

/* Syntax
 *
 * la_list ::= ( expression | empty ) { ':' ( expression | empty ) }
 * expression ::= term { '|' term }
 * term ::= factor { '&' factor }
 * factor ::= String | Literal | '!' factor | '(' expression ')'
 *
 * */

class LaParser
{
public:
    struct Ast;
    typedef QExplicitlySharedDataPointer<Ast> AstRef;
    typedef QList<AstRef> LaExpr;
    struct Ast : public QSharedData
    {
        enum { Invalid, And, Or, Not, Ident, Literal };
        quint8 d_type;
        QByteArray d_val;
        QList<AstRef> d_subs;
        Ast( quint8 t = Invalid, const QByteArray& v = QByteArray() ):d_type(t),d_val(v) {}
    };

    LaParser();
    bool parse(const QByteArray&);
    const LaExpr& getLaExpr() const { return d_lax; }
    const QByteArray& getErr() const { return d_err; }
protected:
    AstRef expression();
    AstRef term();
    AstRef factor();
private:
    LaLexer d_lex;
    QByteArray d_err;
    LaExpr d_lax;
};

#endif // LAPARSER_H
