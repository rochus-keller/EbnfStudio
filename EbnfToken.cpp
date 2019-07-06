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

#include "EbnfToken.h"

QByteArray EbnfToken::toString(bool labeled) const
{
    switch( d_type )
    {
    case Invalid:
        return "<invalid>";
    case Production:
        if( labeled )
            return "production " + d_val.toBa();
        else
            return d_val;
    case Assig:
        return "::=";
    case AddTo:
        return "+=";
    case NonTerm:
        if( labeled )
            return "nonterminal " + d_val.toBa();
        else
            return d_val;
    case Keyword:
        if( labeled )
            return "keyword " + d_val.toBa();
        else
            return d_val;
    case Literal:
        return '\'' + d_val.toBa() + '\'';
    case Bar:
        return "|";
    case LPar:
        return "(";
    case RPar:
        return ")";
    case LBrack:
        return "[";
    case RBrack:
        return "]";
    case LBrace:
        return "{";
    case RBrace:
        return "}";
    case Predicate:
        if( labeled )
            return "predicate \\" + d_val.toBa() + "\\";
        else
            return "\\" + d_val.toBa() + "\\";
    case Comment:
        if( labeled )
            return "comment " + d_val.toBa();
        else
            return d_val;
    case Eof:
        return "<eof>";
    case PpDefine:
        return "#define";
    case PpUndef:
        return "#undef";
    case PpIfdef:
        return "#ifdef";
    case PpIfndef:
        return "#ifndef";
    case PpElse:
        return "#else";
    case PpEndif:
        return "#endif";
    }
    return QByteArray();
}

QHash<QByteArray,EbnfToken::Sym> EbnfToken::s_symTbl;

QByteArray EbnfToken::Sym::toBa() const
{
    if( d_str == 0 )
        return QByteArray();
    else
        return QByteArray::fromRawData( d_str, ::strlen(d_str) );
}

QString EbnfToken::Sym::toStr() const
{
    if( d_str == 0 )
        return QString();
    else
        return QString::fromUtf8( d_str );
}

const char*EbnfToken::Sym::c_str() const
{
    if( d_str == 0 )
        return "";
    else
        return d_str;
}

int EbnfToken::Sym::size() const
{
    if( d_str == 0 )
        return 0;
    else
        return ::strlen(d_str);
}

bool EbnfToken::Sym::isEmpty() const
{
    return d_str == 0;
}

EbnfToken::Sym EbnfToken::getSym(const QByteArray& str)
{
    Sym& sym = s_symTbl[str];
    if( sym.d_str == 0 )
        sym.d_str = str.constData();

    //qDebug() << str << (void*)(atom.constData());

    return sym;
}

void EbnfToken::resetSymTbl()
{

}

bool EbnfToken::isPpType(int tt)
{
    switch(tt)
    {
    case PpDefine:
    case PpUndef:
    case PpIfdef:
    case PpIfndef:
    case PpElse:
    case PpEndif:
        return true;
    default:
        return false;
    }
}
