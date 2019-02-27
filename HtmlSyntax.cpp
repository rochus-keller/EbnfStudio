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

#include "EbnfSyntax.h"
#include "HtmlSyntax.h"
#include <QTextDocument>
#include <QTextFrame>
#include <QFile>
#include <QTextStream>
#include <QtDebug>
#include <QDir>

HtmlSyntax::HtmlSyntax()
{

}

bool HtmlSyntax::transform(const QString& inPath, const QString& outPath )
{
    // Lese eine HTML-Datei, in welcher die EbnfSyntax als EBNF dargestellt ist, und
    // konvertiere diese in eine Datei, die von EbnfStudio gelesen werden kann.
    // Die Formatierung und Symbolik der HTML-Datei entspricht derjenigen in IEEE 1800,
    // d.h. Terminals sind rot wiedergegeben, und die schwarze Klammerung gehört zur EBNF.
    d_error.clear();
    QFile in(inPath);
    if( !in.open(QIODevice::ReadOnly) )
    {
        d_error = QString("cannot open for reading %1").arg(inPath);
        return false;
    }
    QFile out(outPath);
    if( !out.open(QIODevice::WriteOnly) )
    {
        d_error = QString("cannot open for writing %1").arg(outPath);
        return false;
    }
    QTextDocument doc;

    doc.setHtml( QString::fromUtf8( in.readAll() ) );

    QTextStream ts( &out );
    ts.setCodec("utf-8");

    return reformat( doc, ts );
}

static inline bool containsLetters( const QString& str )
{
    for( int i = 0; i < str.size(); i++ )
    {
        if( str[i].isLetter() )
            return true;
    }
    return false;
}
static inline bool printHex( const QString& str )
{
    QByteArray ba;
    for( int i = 0; i < str.size(); i++ )
        ba += QByteArray::number(str[i].unicode(),16) + " ";
    qDebug() << ba << str;
}

bool HtmlSyntax::reformat(const QTextDocument& doc, QTextStream& out)
{
    QTextFrame* root = doc.rootFrame();
    for( QTextFrame::Iterator i = root->begin(); i != root->end(); ++i )
    {
        if( i.currentBlock().isValid() )
        {
            const QString line = i.currentBlock().text();
            if( !line.trimmed().isEmpty() )
            {
                Q_ASSERT( !line.isEmpty() );
                if( !line[0].isSpace() && line.indexOf("::=") == -1 )
                {
                    out << "// " << line;
                }else
                {
                    QTextBlock::Iterator it = i.currentBlock().begin();
                    while( !it.atEnd() )
                    {
                        const QTextFragment f = it.fragment();
                        const QTextCharFormat tf = f.charFormat();
                        const QStringList lines = f.text().split( QChar(0x2028) ); // 0x2028 kommt als Zeilenumbruch
                        for( int n = 0; n < lines.size(); n++ )
                        {
                            if( n > 0 )
                                out << endl;
                            const QStringList words = lines[n].split( QChar(' ') );
                            for( int k = 0; k < words.size(); k++ )
                            {
                                if( k > 0 )
                                    out << " ";
                                const bool rot = tf.foreground().color() == Qt::red && !containsLetters(words[k]);
                                if( rot )
                                    out << "'";
                                out << words[k];
                                if( rot )
                                    out << "'";
                            }
                        }
                        it++;
                    }
                }
            }
            out << endl;
        }
    }
    return true;
}

void HtmlSyntax::writeCompressedDefs(QTextStream& out, EbnfSyntax::Node* node, HtmlSyntax::Stack& stack, int level, bool all)
{
    if( node == 0 )
        return;

    if( node->d_tok.d_op == EbnfToken::Skip )
        return;
    if( node->d_def && node->d_def->d_tok.d_op == EbnfToken::Skip )
        return;

    switch( node->d_quant )
    {
    case EbnfSyntax::Node::One:
        if( level > 0 && node->d_type == EbnfSyntax::Node::Alternative )
            out << "( ";
        else if( level > 0 && node->d_type == EbnfSyntax::Node::Sequence && !node->d_tok.d_val.isEmpty() )
            out << "( ";
        break;
    case EbnfSyntax::Node::ZeroOrOne:
        out << "[ ";
        break;
    case EbnfSyntax::Node::ZeroOrMore:
        out << "{ ";
        break;
    }

    switch( node->d_type )
    {
    case EbnfSyntax::Node::Terminal:
        out << "<b>" << node->d_tok.d_val.toStr().toHtmlEscaped() << "</b> ";
        break;
    case EbnfSyntax::Node::Nonterminal:
        if( node->d_def == 0 || node->d_def->d_node == 0 )
            out << node->d_tok.d_val.toStr().toHtmlEscaped() << " ";
        else
        {
            const EbnfSyntax::Definition* ref = node->d_def;
            Q_ASSERT( ref != 0 );
            const bool transp = node->d_tok.d_op == EbnfToken::Transparent ||
                    ref->d_tok.d_op == EbnfToken::Transparent;
            if( all || !transp )
            {
                if( transp )
                    out << "<em>";
                out << "<a href=\"#" << node->d_tok.d_val.toStr() << "\">";
                out << node->d_tok.d_val.toStr().toHtmlEscaped();
                out << "</a>";
                if( transp )
                    out << "</em> ";
                else
                    out << " ";
            }else if( stack.contains(ref) )
            {
                out << "... ";
            }else
            {
                stack.push_back(ref);
                //out << "<br>&nbsp;&nbsp;&nbsp;&nbsp;";
                writeCompressedDefs( out, ref->d_node, stack, level + 1, all );
                stack.pop_back();
            }
        }
        break;
    case EbnfSyntax::Node::Alternative:
        for( int i = 0; i < node->d_subs.size(); i++ )
        {
            if( i != 0 )
            {
                if( level == 0 )
                    out << "<br>   | ";
                else
                    out << "| ";
            }
            writeCompressedDefs( out, node->d_subs[i], stack, level + 1, all );
        }
        break;
    case EbnfSyntax::Node::Sequence:
        for( int i = 0; i < node->d_subs.size(); i++ )
        {
            writeCompressedDefs( out, node->d_subs[i], stack, level + 1, all );
        }
        break;
    }

    switch( node->d_quant )
    {
    case EbnfSyntax::Node::One:
        if( level > 0 && node->d_type == EbnfSyntax::Node::Alternative )
            out << ") ";
        else if( level > 0 && node->d_type == EbnfSyntax::Node::Sequence && !node->d_tok.d_val.isEmpty() )
            out << ") ";
        break;
    case EbnfSyntax::Node::ZeroOrOne:
        out << "] ";
        break;
    case EbnfSyntax::Node::ZeroOrMore:
        out << "} ";
        break;
    }
}

bool HtmlSyntax::generateHtml(const QString& ebnfPath, EbnfSyntax* syn, bool all )
{
    QFileInfo info(ebnfPath);

    QFile f( info.absoluteDir().absoluteFilePath( info.completeBaseName() + ".html") );
    f.open( QIODevice::WriteOnly );
    QTextStream out(&f);
    out.setCodec("utf-8");
    out.setCodec( "UTF-8" );
    out << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">" << endl;
    out << "<html><META http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">" << endl;
    out << "<head><title>Verilog05 Compressed EbnfSyntax</title>" << endl;
    //writeCss(out);
    out << "<style type=\"text/css\" media=\"screen, projection, print\">" << endl;
    out << "body{font-family:Arial,sans-serif;font-size:medium;orphans:2;widows:2;}" << endl;
    out << "b{color:red}" << endl;
    out << "em{color:green}" << endl;
    out << "a[href^=\"#\"]{text-decoration:none}";
    out << "</style>" << endl;
    out << "</head><body>" << endl;
    out << "<p>This file was automatically generated by EbnfStudio; don't modify it!</p>" << endl;
    out << "<dl>" << endl;

    for( int i = 0; i < syn->getOrderedDefs().size(); i++ )
    {
        const EbnfSyntax::Definition* d = syn->getOrderedDefs()[i];

        if( d->d_tok.d_op == EbnfToken::Skip || ( i != 0 && d->d_usedBy.isEmpty() ) )
            continue;
        if( !all && d->d_tok.d_op == EbnfToken::Transparent && i != 0 )
            continue;
        if( d->d_node == 0 )
            continue;
        out << "<dt>";
        if( d->d_tok.d_op == EbnfToken::Transparent )
            out << "<em>";
        out << "<a name=\"" << d->d_tok.d_val.toStr() << "\">";
        out << d->d_tok.d_val.toStr();
        out << "</a>";
        if( d->d_tok.d_op == EbnfToken::Transparent )
            out << "</em>";
        out << "&nbsp;:</dt>" << endl;
        out << "<dd>";
        Stack stack;
        writeCompressedDefs( out, d->d_node, stack, 0, all );
        out << "</dd><br>" << endl;
    }
    out << "</dl></body></html>";

}

