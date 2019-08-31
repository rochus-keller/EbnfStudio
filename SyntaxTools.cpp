#include "SyntaxTools.h"

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
        const SyntaxTools::IeeeLineKind k = SyntaxTools::guessKindOfIeeeLine(line);
        switch( k )
        {
        case SyntaxTools::ContinueProduction:
        case SyntaxTools::StartProduction:
            if( k == SyntaxTools::StartProduction )
                out.write("\n");
            else
                out.write("\t");
            foreach( const QByteArray& str, SyntaxTools::tokenizeIeeeLine(line) )
            {
                out.write(str);
                out.write(" ");
            }
            out.write("\n");
            break;
        case SyntaxTools::EmptyLine:
            // ignore;
            break;
        case SyntaxTools::CommentLine:
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
