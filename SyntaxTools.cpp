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

#include "SyntaxTools.h"
#include <QStringList>

int SyntaxTools::startOfNws(const QByteArray& line)
{
    int res = 0;
    for( int i = 0; i < line.size(); i++ )
    {
        if( ::isspace(line[i]) )
            res++;
        else
            break;
    }
    if( res == line.size() )
        return -1;
    else
        return res;
}

SyntaxTools::IeeeLineKind SyntaxTools::guessKindOfIeeeLine(const QByteArray& line)
{
    const int eqPos = line.indexOf("::=");
    const int cmtPos = line.indexOf("//");
    const int off = startOfNws(line);
    if( off < 0 )
        return EmptyLine;
    if( eqPos != -1 && ( cmtPos == -1 || eqPos < cmtPos ) )
        return StartProduction;
    else if( cmtPos == off )
        return CommentLine;
    else if( off == 0 )
        return TextLine;
    else
        return ContinueProduction;
}

static inline char get( const QByteArray& line, int i )
{
    if( i < line.size() )
        return line[i];
    else
        return 0;
}

static inline bool isEbnfChar( char ch )
{
    return ch == '|' || ch == '[' || ch == ']' || ch == '{' || ch == '}';
}

QByteArrayList SyntaxTools::tokenizeIeeeLine(const QByteArray& line)
{
    QByteArrayList res;
    int i = 0;
    while( i < line.size() )
    {
        const char ch = line[i];
        if( ::isspace(ch) )
        {
            int j = i + 1;
            while( j < line.size() && ::isspace(line[j]) )
                j++;
            i = j - 1;
        }else if( ch == ':' && get(line,i+1) == ':' && get(line,i+2) == '=' )
        {
            res += QByteArray("::=");
            i += 2;
        }else if( ::isalpha(ch) || ch == '$' )
        {
            int j = i + 1;
            while( j < line.size() && ( ::isalnum(line[j]) || line[j] == '_' || line[j] == '$' ) )
                j++;
            res += line.mid(i,j-i);
            i = j-1;
        }else if( isEbnfChar(ch) )
            res += QByteArray(1,ch);
        else
        {
            int j = i + 1;
            while( j < line.size() && !::isspace(line[j]) )
                j++;
            QByteArray sym = line.mid(i,j-i);
            sym.replace('\'', "\\'" );
            sym = '\'' + sym + '\'';
            res += sym;
            i = j-1;
        }
        i++;
    }
    return res;
}

void SyntaxTools::transformIeeeEbnf(QIODevice& in, QIODevice& out)
{
    while( !in.atEnd() )
    {
        const QByteArray line = in.readLine();
        const IeeeLineKind k = guessKindOfIeeeLine(line);
        switch( k )
        {
        case ContinueProduction:
        case StartProduction:
            if( k == StartProduction )
                out.write("\n");
            else
                out.write("\t");
            foreach( const QByteArray& str, tokenizeIeeeLine(line) )
            {
                out.write(str);
                out.write(" ");
            }
            out.write("\n");
            break;
        case EmptyLine:
            // ignore;
            break;
        case CommentLine:
            out.write(line);
            out.write("\n");
            break;
        default:
            out.write("\n");
            out.write("// ");
            out.write(line);
            break;
        }
    }
}

void SyntaxTools::transformAlgolEbnf(QIODevice& in, QIODevice& out)
{
    while( !in.atEnd() )
    {
        const QByteArray line = in.readLine();
        const IeeeLineKind k = guessKindOfIeeeLine(line);
        switch( k )
        {
        case ContinueProduction:
        case StartProduction:
            if( k == StartProduction )
                out.write("\n");
            else
                out.write("\t");
            foreach( const QString& str, tokenizeAlgolLine(line) )
            {
                out.write(str.toUtf8());
                out.write(" ");
            }
            out.write("\n");
            break;
        case EmptyLine:
            // ignore;
            break;
        case CommentLine:
            out.write(line);
            out.write("\n");
            break;
        default:
            out.write("\n");
            out.write("// ");
            out.write(line);
            break;
        }
    }
}

static inline ushort get( const QString& line, int i )
{
    if( i < line.size() )
        return line[i].unicode();
    else
        return 0;
}

QByteArrayList SyntaxTools::tokenizeAlgolLine(const QString& line)
{
    static QString toReplace = QString::fromUtf8("×÷≤≥≠↑≡⊃∨∧¬");
    // see https://en.wikipedia.org/wiki/ALGOL_60 and http://csci.csusb.edu/dick/samples/algol60.syntax.html
    static const char* replaceWith[] = {
        "*", "%", "<=", ">=", "<>", "^", "==", "=>", "\\\\/", "/\\\\", "~"
    };
    QByteArrayList res;
    int i = 0;
    while( i < line.size() )
    {
        const QChar c = line[i];
        const ushort ch = c.unicode();
        if( ::isspace(ch) )
        {
            int j = i + 1;
            while( j < line.size() && line[j].isSpace() )
                j++;
            i = j - 1;
        }else if( ch == ':' && get(line,i+1) == ':' && get(line,i+2) == '=' )
        {
            res += QByteArray("::=");
            i += 2;
        }else if( ch == '<' && ::isalpha(get(line,i+1)) )
        {
            int j = i + 1;
            while( j < line.size() && get(line,j) != '>' )
                j++;
            QString nt = line.mid(i+1,j-i-1);
            nt.replace(' ', '_');
            res += nt.toUtf8();
            i = j;
        }else if( ch == '\'' && ::isalpha(get(line,i+1)) )
        {
            int j = i + 1;
            while( j < line.size() && get(line,j) != '\'' )
                j++;
            const QString nt = line.mid(i+1,j-i-1);
            res += nt.toUtf8();
            i = j;
        }else if( ::isalpha(ch) )
        {
            int j = i + 1;
            while( j < line.size() && ( line[j].isLetterOrNumber() || line[j] == '_' ) )
                j++;
            res += line.mid(i,j-i).toUtf8();
            i = j-1;
        }else if( isEbnfChar(ch) )
            res += QByteArray(1,ch);
        else
        {
            int j = i + 1;
            while( j < line.size() && !line[j].isSpace() && !isEbnfChar(line[j].unicode()) && line[j].unicode() != '<')
                j++;
            QString sym = line.mid(i,j-i);
            sym.replace('\'', "\\'" );
            for( int n = 0; n < toReplace.size(); n++ )
                sym.replace(toReplace[n],replaceWith[n]);

            sym = '\'' + sym + '\'';
            res += sym.toUtf8();
            i = j-1;
        }
        i++;
    }
    return res;
}
