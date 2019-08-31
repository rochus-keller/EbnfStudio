#ifndef SYNTAXTOOLS_H
#define SYNTAXTOOLS_H

#include <QIODevice>

class SyntaxTools
{
public:
    static int startOfNws( const QByteArray& line );
    enum IeeeLineKind { StartProduction, ContinueProduction, EmptyLine, CommentLine, TextLine };
    static IeeeLineKind guessKindOfIeeeLine( const QByteArray& line );
    static QByteArrayList tokenizeIeeeLine(const QByteArray& line );
    static void transformIeeeEbnf( QIODevice& in, QIODevice& out );
private:
    SyntaxTools(){}
};

#endif // SYNTAXTOOLS_H
