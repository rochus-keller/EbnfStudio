#ifndef EBNFLEXER_H
#define EBNFLEXER_H

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

#include <QObject>
#include <QSet>
#include "EbnfToken.h"

class QIODevice;

class EbnfLexer : public QObject
{
public:
    typedef QSet<EbnfToken::Sym> Keywords;
    explicit EbnfLexer(QObject *parent = 0);

    void setStream( QIODevice* );

    void setKeywords( const Keywords& kw ) { d_kw = kw; }
    void addKeywords( const Keywords& kw ) { d_kw += kw; }
    const Keywords& getKeywords() const { return d_kw; }
    bool readKeywordsFromFile( const QString& path );

    EbnfToken nextToken();
    EbnfToken peekToken(quint8 lookAhead = 1);
    QList<EbnfToken> tokens( const QString& code );
    QList<EbnfToken> tokens( const QByteArray& code );
protected:
    EbnfToken nextTokenImp();
    int skipWhiteSpace();
    EbnfToken token(EbnfToken::TokenType tt, int len = 1, const QByteArray &val = QByteArray());
    int lookAhead(int off = 1) const;
    EbnfToken ident();
    EbnfToken attribute();
    EbnfToken literal();
    EbnfToken ppsym();
    EbnfToken::Handling readOp();
    void nextLine();

private:
    QIODevice* d_in;
    quint32 d_lineNr; // current line, starting with 1
    quint16 d_colNr;  // current column (left of char), starting with 0
    QString d_line;
    EbnfToken d_lastToken;
    QList<EbnfToken> d_buffer;
    Keywords d_kw;
};

#endif // EBNFLEXER_H
