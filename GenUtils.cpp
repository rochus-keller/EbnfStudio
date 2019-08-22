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

#include "GenUtils.h"
#include <QtDebug>
#include <QHash>

GenUtils::GenUtils()
{

}

QByteArray GenUtils::escapeDollars(QByteArray name)
{
    const char dollar = '$';
    if( name.startsWith( dollar ))
        name = "dlr_" + name.mid(1);
    if( name.endsWith(dollar))
        name = name.left(name.size()-1) + "_dlr";
    name.replace(dollar,"_dlr_");
    name.replace('-','_');
    return name;
}

bool GenUtils::containsAlnum( const QByteArray& str )
{
    for( int i = 0; i < str.size(); i++ )
    {
        if( ::isalnum( str[i] ) )
            return true;
    }
    return false;
}

QByteArray GenUtils::symToString(const QByteArray& str)
{
    if( str.isEmpty() )
        return str;

    static QHash<QByteArray,QByteArray> map;

    if( map.isEmpty() )
    {
        map.insert("(*","Latt");
        map.insert("*)", "Ratt");
        map.insert("/*", "Lcmt");
        map.insert("*/", "Rcmt");
        map.insert("<=", "Leq");
        map.insert(">=", "Geq");
        // map.insert("#0", "HashZero");
    }
    QByteArray res = map.value(str);
    if( !res.isEmpty() )
        return res;

    if( containsAlnum(str) )
        return escapeDollars(str);

    int i = 0;
    while( i < str.size() )
    {
        int n = 1;
        while( i + n < str.size() && str[i] == str[i+n] )
            n++;
        if( n > 1 )
            res += QByteArray::number(n);
        res += charToString(str[i]);
        i += n;
    }
    return res;
}

QByteArray GenUtils::charToString(char c)
{
    switch( c )
    {
    case '+':
        return "Plus";
    case '-':
        return "Minus";
    case '!':
        return "Bang";
    case '=':
        return "Eq";
    case '~':
        return "Tilde";
    case '|':
        return "Bar";
    case '&':
        return "Amp";
    case '^':
        return "Hat";
    case '/':
        return "Slash";
    case '%':
        return "Percent";
    case '*':
        return "Star";
    case '<':
        return "Lt";
    case '>':
        return "Gt";
    case '#':
        return "Hash";
    case '@':
        return "At";
    case '?':
        return "Qmark";
    case '(':
        return "Lpar";
    case ')':
        return "Rpar";
    case '[':
        return "Lbrack";
    case ']':
        return "Rbrack";
    case '{':
        return "Lbrace";
    case '}':
        return "Rbrace";
    case ',':
        return "Comma";
    case '.':
        return "Dot";
    case ';':
        return "Semi";
    case ':':
        return "Colon";
    case '$':
        return "Dlr";
    default:
        return QByteArray::number( int(c), 16 );
    }
    return QByteArray();
}

static bool lessThan( const QByteArray& lhs, const QByteArray& rhs )
{
    const bool lhsAlnum = GenUtils::containsAlnum(lhs);
    const bool rhsAlnum = GenUtils::containsAlnum(rhs);
    if( lhsAlnum && rhsAlnum || !lhsAlnum && !rhsAlnum )
        return lhs < rhs;
    else
        return !lhsAlnum && rhsAlnum;
}

QByteArrayList GenUtils::orderedTokenList(const QSet<QByteArray>& tokens, bool applySymToString)
{
    QByteArrayList res = tokens.toList();
    std::sort( res.begin(), res.end(), lessThan );

    if( applySymToString )
    {
        for( int i = 0; i < res.size(); i++ )
            res[i] = symToString( res[i] );
    }
    return res;
}
