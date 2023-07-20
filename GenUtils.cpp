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

GenUtils::TokMap GenUtils::s_tokMap;

GenUtils::GenUtils()
{

}

QString GenUtils::escapeDollars(QString name)
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

bool GenUtils::containsAlnum( const QString& str )
{
    // '$' is not considered isLetterOrNumber
    for( int i = 0; i < str.size(); i++ )
    {
        if( str[i].isLetterOrNumber() )
            return true;
    }
    return false;
}

bool GenUtils::looksLikeKeyword(const QString& str)
{
    if( str.isEmpty() || !str[0].isLetter() )
        return false;
    for( int i = 1; i < str.size(); i++ )
    {
        if( !str[i].isLetterOrNumber() && str[i] != '_' )
            return false;
    }
    return true;
}

QString GenUtils::symToString(const QString& str)
{
    if( str.isEmpty() )
        return str;

    if( s_tokMap.isEmpty() )
    {
        s_tokMap.insert("(*","Latt");
        s_tokMap.insert("*)", "Ratt");
        s_tokMap.insert("/*", "Lcmt");
        s_tokMap.insert("*/", "Rcmt");
        s_tokMap.insert("<=", "Leq");
        s_tokMap.insert(">=", "Geq");
    }
    TokMap::const_iterator it = s_tokMap.find(str);
    if( it != s_tokMap.end() )
        return it.value();

    if( containsAlnum(str) ) // as soon as there
        return escapeDollars(str);

    QString res;
    int i = 0;
    while( i < str.size() )
    {
        int n = 1;
        while( i + n < str.size() && str[i] == str[i+n] )
            n++;
        if( n > 1 )
            res += QString::number(n);
        res += charToString(str[i]);
        i += n;
    }
    return res;
}

QString GenUtils::charToString(QChar c)
{
    TokMap::const_iterator i = s_tokMap.find(c);
    if( i != s_tokMap.end() )
        return i.value();

    switch( c.unicode() )
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
    case '"':
        return "Quote";
    default:
        return QString::number( c.unicode(), 16 );
    }
    return QString();
}

static bool lessThan( const QString& lhs, const QString& rhs )
{
    const bool lhsAlnum = GenUtils::containsAlnum(lhs);
    const bool rhsAlnum = GenUtils::containsAlnum(rhs);
    if( (lhsAlnum && rhsAlnum) || (!lhsAlnum && !rhsAlnum) )
        return lhs < rhs;
    else
        return !lhsAlnum && rhsAlnum;
}

QStringList GenUtils::orderedTokenList(const QSet<QString>& tokens, bool applySymToString)
{
    QStringList res = tokens.toList();
    std::sort( res.begin(), res.end(), lessThan );

    if( applySymToString )
    {
        for( int i = 0; i < res.size(); i++ )
            res[i] = symToString( res[i] );
    }
    return res;
}
