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

#include "LaParser.h"
#include <QString>
#include <QtDebug>

const char* LaLexer::Tok::s_typeNames[] =
{
    "???", "Literal", "Ident", "Or", "And", "Not", "LPar", "RPar", "Colon", "Index", "Eof"
};
const char* LaParser::Ast::s_typeNames[] =
{
    "???",
    "And", "Or", "Not", "Ident", "Literal", "La"
};

LaParser::LaParser()
{

}

bool LaParser::parse(const QByteArray& str)
{
    d_err.clear();
    d_res.reset();
    d_lex.setStr( str );
#if 0
    LaLexer::Tok t = d_lex.nextToken();
    while( t.isValid())
    {
        qDebug() << LaLexer::Tok::s_typeNames[t.d_type] << t.d_val;
        t = d_lex.nextToken();
    }
    d_lex.setStr( str );
#endif
    d_res = laExpr();
    if( d_res.data() == 0 )
        return false;
    if( d_lex.nextToken().d_type != LaLexer::Tok::Eof )
    {
        d_err = "invalid LaExpr";
        return false;
    }
    return true;
}

LaParser::AstRef LaParser::expression()
{
    QList<AstRef> terms;
    AstRef r = term();
    if( r.data() == 0 )
        return r;
    terms << r;
    while( d_lex.peekToken().d_type == LaLexer::Tok::Or )
    {
        d_lex.nextToken();
        r = term();
        if( r.data() == 0 )
            return r;
        terms << r;
    }
    if( terms.size() == 1 )
        return terms.first();
    r = new Ast(Ast::Or);
    r->d_subs = terms;
    return r;
}

LaParser::AstRef LaParser::laExpr()
{
    QList<AstRef> terms;
    AstRef r = laTerm();
    if( r.data() == 0 )
        return r;
    terms << r;
    while( d_lex.peekToken().d_type == LaLexer::Tok::Or )
    {
        d_lex.nextToken();
        r = laTerm();
        if( r.data() == 0 )
            return r;
        terms << r;
    }
    if( terms.size() == 1 )
        return terms.first();
    r = new Ast(Ast::Or);
    r->d_subs = terms;
    return r;
}

LaParser::AstRef LaParser::term()
{
    QList<AstRef> factors;
    AstRef r = factor();
    if( r.data() == 0 )
        return r;
    factors << r;
    while( d_lex.peekToken().d_type == LaLexer::Tok::And )
    {
        d_lex.nextToken();
        r = factor();
        if( r.data() == 0 )
            return r;
        factors << r;
    }
    if( factors.size() == 1 )
        return factors.first();
    r = new Ast(Ast::And);
    r->d_subs = factors;
    return r;
}

LaParser::AstRef LaParser::laTerm()
{
    QList<AstRef> factors;
    AstRef r = laFactor();
    if( r.data() == 0 )
        return r;
    factors << r;
    while( d_lex.peekToken().d_type == LaLexer::Tok::And )
    {
        d_lex.nextToken();
        r = laFactor();
        if( r.data() == 0 )
            return r;
        factors << r;
    }
    if( factors.size() == 1 )
        return factors.first();
    r = new Ast(Ast::And);
    r->d_subs = factors;
    return r;
}

LaParser::AstRef LaParser::factor()
{
    LaLexer::Tok t = d_lex.nextToken();
    if( t.d_type == LaLexer::Tok::Literal )
        return AstRef(new Ast( Ast::Literal, t.d_val ));
    if( t.d_type == LaLexer::Tok::Ident )
        return AstRef(new Ast( Ast::Ident, t.d_val ));
    if( t.d_type == LaLexer::Tok::Not )
    {
        AstRef r = factor();
        if( r.data() == 0 )
            return r;
        AstRef n(new Ast(Ast::Not));
        n->d_subs << r;
        return n;
    }
    if( t.d_type == LaLexer::Tok::LPar )
    {
        AstRef r = expression();
        if( r.data() == 0 )
            return r;
        t = d_lex.nextToken();
        if( t.d_type != LaLexer::Tok::RPar )
        {
            d_err = "expecting ')'";
            return AstRef();
        }
        return r;
    }
    d_err = "invalid factor";
    return AstRef();
}

LaParser::AstRef LaParser::laFactor()
{
    LaLexer::Tok t = d_lex.nextToken();
    if( t.d_type == LaLexer::Tok::Index )
    {
        if( t.d_val.toUInt() == 0 )
        {
            d_err = "expecting Index > 0";
            return AstRef();
        }
        LaLexer::Tok colon = d_lex.nextToken();
        if( colon.d_type != LaLexer::Tok::Colon )
        {
            d_err = "expecting ':'";
            return AstRef();
        }
        AstRef r = factor();
        if( r.data() == 0 )
            return r;
        AstRef n(new Ast(Ast::La,t.d_val));
        n->d_subs << r;
        return n;
    }
    if( t.d_type == LaLexer::Tok::LPar )
    {
        AstRef r = laExpr();
        if( r.data() == 0 )
            return r;
        t = d_lex.nextToken();
        if( t.d_type != LaLexer::Tok::RPar )
        {
            d_err = "expecting ')'";
            return AstRef();
        }
        return r;
    }
    d_err = "invalid la_factor";
    return AstRef();
}

LaLexer::LaLexer():d_pos(0)
{

}

void LaLexer::setStr(const QByteArray& str)
{
    d_pos = 0;
    d_str = str;
}

LaLexer::Tok LaLexer::nextToken()
{
    Tok t;
    if( !d_buffer.isEmpty() )
    {
        t = d_buffer.first();
        d_buffer.pop_front();
    }else
        t = nextTokenImp();
    return t;
}

LaLexer::Tok LaLexer::peekToken(quint8 lookAhead)
{
    Q_ASSERT( lookAhead > 0 );
    while( d_buffer.size() < lookAhead )
        d_buffer.push_back( nextTokenImp() );
    return d_buffer[ lookAhead - 1 ];
}

LaLexer::Tok LaLexer::nextTokenImp()
{
    skipWhiteSpace();
    while( d_pos < d_str.size() )
    {
        const char ch = d_str[d_pos];
        if( ch == '(' )
            return token(Tok::LPar);
        if( ch == ')' )
            return token(Tok::RPar);
        if( ch == '|' )
            return token(Tok::Or);
        if( ch == '&' )
            return token(Tok::And);
        if( ch == '!' )
            return token(Tok::Not);
        if( ch == ':' )
            return token(Tok::Colon);
        if( ::isdigit(ch) )
            return index();
        if( ::isalpha(ch) )
            return ident();
        if( ch == '\'' )
            return literal();
        return token( Tok::Invalid, 1, QString("unexpected character '%1' %2").arg(ch).arg(quint8(ch)).toUtf8() );
    }
    return Tok(Tok::Eof);
}

int LaLexer::skipWhiteSpace()
{
    while( d_pos < d_str.size() && ::isspace(d_str[d_pos]) )
        d_pos++;
}

LaLexer::Tok LaLexer::ident()
{
    int off = 1; // off == 0 wurde bereits dem ident zugeordnet
    while( true )
    {
        if( (d_pos+off) >= d_str.size() ||
                ( !::isalnum(d_str[d_pos+off]) && d_str[d_pos+off] != '_' ) )
            break;
        else
            off++;
    }
    return token(Tok::Ident,off, d_str.mid(d_pos, off ));
}

LaLexer::Tok LaLexer::literal()
{
    int off = 1;
    while( true )
    {
        if( (d_pos+off) < d_str.size() && d_str[d_pos+off] == '\\' )
            off++;
        else if( (d_pos+off) >= d_str.size() || d_str[d_pos+off] == '\'' )
            break;
        off++;
    }
    QByteArray str = d_str.mid(d_pos + 1, off - 1 );
    str.replace("\\'", "'");
    str.replace("\\\\", "\\");
    return token( Tok::Literal, off+1, str );
}

LaLexer::Tok LaLexer::index()
{
    int off = 1; // off == 0 wurde bereits dem ident zugeordnet
    while( true )
    {
        if( (d_pos+off) >= d_str.size() || !::isdigit(d_str[d_pos+off]) )
            break;
        else
            off++;
    }
    return token(Tok::Index,off, d_str.mid(d_pos, off ));
}

LaLexer::Tok LaLexer::token(quint8 type, int len, const QByteArray& v)
{
    d_pos += len;
    return Tok( type, v );
}

static QByteArray ws( int level )
{
    QByteArray res;
    for( int i = 0; i < level; i++ )
        res += " | ";
    return res;
}

void LaParser::Ast::dump(int level)
{
    qDebug() << ws(level).constData() << s_typeNames[d_type] << d_val;
    foreach( AstRef a, d_subs )
        a->dump(level+1);
}
