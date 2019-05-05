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

#include "EbnfHighlighter.h"
#include "EbnfLexer.h"
#include <QtDebug>

EbnfHighlighter::EbnfHighlighter(QTextDocument* doc):QSyntaxHighlighter(doc)
{
    for( int i = 0; i < C_Max; i++ )
    {
        d_format[i].setFontWeight(QFont::Normal);
        d_format[i].setForeground(Qt::black);
    }
    d_format[C_Cmt].setForeground(QColor(0, 128, 0)); //QColor(153, 153, 136));

    d_format[C_Pred].setForeground(QColor(0, 153, 153));
    d_format[C_Ebnf].setForeground(Qt::blue); // QColor(68, 85, 136));
    d_format[C_Ebnf].setFontWeight(QFont::Bold);

    d_format[C_Lit].setForeground( Qt::red ); // QColor(153, 0, 0));
    d_format[C_Lit].setFontWeight(QFont::Bold);
    d_format[C_Kw].setForeground( Qt::red ); // QColor(153, 0, 115));
    d_format[C_Kw].setFontWeight(QFont::Bold);

    d_format[C_Nt].setProperty( NonTermProp, true );
    d_format[C_Nt].setToolTip( "Nonterinal Symbol" );


    d_format[C_Prod].setForeground( QColor(128, 0, 128) );
    d_format[C_Prod].setFontWeight(QFont::Bold);

    d_format[C_Gray].setForeground( Qt::gray );

    d_format[C_Pragma].setForeground( Qt::darkGray );
    d_format[C_Pragma].setFontWeight(QFont::Bold);

    const QColor brown( 147, 39, 39 );
    d_format[C_Pp].setForeground( brown );
    d_format[C_Pp].setFontWeight(QFont::Bold);

}

void EbnfHighlighter::highlightBlock(const QString& text)
{
    EbnfLexer lex;
    lex.setKeywords(d_kw);
    QList<EbnfToken> toks = lex.tokens(text);

    foreach( const EbnfToken& t, toks )
    {
        if( EbnfToken::isPpType(t.d_type) )
        {
            const int cmtPos = text.indexOf(QLatin1String("//"));
            if( cmtPos == -1 )
                setFormat( 0, text.size(), d_format[C_Pp] );
            else
            {
                setFormat( 0, cmtPos, d_format[C_Pp] );
                setFormat( cmtPos, text.size() - cmtPos, d_format[C_Cmt] );
            }
            continue;
        }

        QTextCharFormat f;
        switch( t.d_type )
        {
        case EbnfToken::Invalid:
            break;
        case EbnfToken::Production:
            if( text.startsWith(QChar('%') ) )
                f = d_format[C_Pragma];
            else
                f = d_format[C_Prod];
            break;
        case EbnfToken::NonTerm:
            f = d_format[C_Nt];
            break;
        case EbnfToken::Keyword:
            f = d_format[C_Kw];
            break;
        case EbnfToken::Literal:
            f = d_format[C_Lit];
            break;
        case EbnfToken::Predicate:
            f = d_format[C_Pred];
            break;
        case EbnfToken::Comment:
            f = d_format[C_Cmt];
            break;
        case EbnfToken::Assig:
        case EbnfToken::Bar:
        case EbnfToken::LPar:
        case EbnfToken::RPar:
        case EbnfToken::LBrack:
        case EbnfToken::RBrack:
        case EbnfToken::LBrace:
        case EbnfToken::RBrace:
        case EbnfToken::AddTo:
            f = d_format[C_Ebnf];
            break;
        default:
            break;
        }
        if( f.isValid() )
        {
            if( t.d_type == EbnfToken::Literal )
            {
                setFormat( t.d_colNr - 1, 1, d_format[C_Gray] );
                setFormat( t.d_colNr, t.d_len-2, f );
                setFormat( t.d_colNr + t.d_len-2, 1, d_format[C_Gray] );
            }else
                setFormat( t.d_colNr-1, t.d_len, f );
        }
   }
}

