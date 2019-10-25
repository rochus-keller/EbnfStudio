#ifndef EBNFPARSER_H
#define EBNFPARSER_H

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
#include <QStack>
#include <QStringList>
#include "EbnfToken.h"
#include "EbnfSyntax.h"

class EbnfLexer;
class EbnfErrors;

class EbnfParser : public QObject
{
public:
    explicit EbnfParser(QObject *parent = 0);

    void setErrors( EbnfErrors* e ) { d_errs = e; }

    bool parse( EbnfLexer* );
    EbnfSyntax* getSyntax();


protected:
    EbnfToken nextToken();
    bool error( const EbnfToken& t, const QString& msg = QString() );
    EbnfSyntax::Node* parseFactor();
    EbnfSyntax::Node* parseExpression();
    EbnfSyntax::Node* parseTerm();
    bool checkCardinality(EbnfSyntax::Node*);
    void handlePpSym(const EbnfToken&);
    bool txOn() const;
    void txlog(quint32 line, bool lastOn);

private:
    EbnfLexer* d_lex;
    EbnfSyntaxRef d_syn;
    EbnfSyntax::Definition* d_def;
    EbnfToken d_cur;
    EbnfErrors* d_errs;
    EbnfSyntax::Defines d_defines;
    enum IfState { InIf, IfActive, InElse };
    QStack< QPair<quint8,bool> > d_ifState; // ifState, txOn
};

#endif // EBNFPARSER_H
