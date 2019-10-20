#ifndef EBNFANALYZER_H
#define EBNFANALYZER_H

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
#include <QSet>

class FirstFollowSet;

class EbnfAnalyzer
{
public:
    EbnfAnalyzer();

    static QSet<QString> collectAllTerminalStrings( EbnfSyntax* );
    static QStringList collectAllTerminalProductions( EbnfSyntax* );

    typedef QList<EbnfSyntax::NodeRefSet> LlkNodes;
    static void calcLlkFirstSet(quint16 k, quint16 curBin, LlkNodes&, const EbnfSyntax::Node* node, FirstFollowSet* );
    static EbnfSyntax::NodeRefSet intersectAll( const LlkNodes& lhs, const LlkNodes& rhs );
    static void calcLlkFirstSet2(quint16 k, LlkNodes&, const EbnfSyntax::Node* node, FirstFollowSet* );

    static void checkForAmbiguity( FirstFollowSet*, EbnfErrors*);
    static void checkForAmbiguity( EbnfSyntax::Node*, FirstFollowSet*, EbnfErrors*, bool recursive = true );

    static EbnfSyntax::ConstNodeList findPath( const EbnfSyntax::Node* from, const EbnfSyntax::Node* to );
protected:
    static QSet<QString> collectAllTerminalStrings( EbnfSyntax::Node* );
    static void findAmbiguousAlternatives( EbnfSyntax::Node*, FirstFollowSet*, EbnfErrors* );
    static void findAmbiguousOptionals( EbnfSyntax::Node*, FirstFollowSet*, EbnfErrors* );
    static void reportAmbig(EbnfSyntax::Node* seq, int ambigIdx, const EbnfSyntax::NodeRefSet& diff, const EbnfSyntax::NodeSet& ambigSet2, FirstFollowSet*, EbnfErrors* );
    static void calcLlkFirstSet2Imp(quint16 k, int curBin, int level, LlkNodes&, const EbnfSyntax::Node* node,
                                    FirstFollowSet*, QSet<const EbnfSyntax::Node*>& visited );
    static bool findPath( EbnfSyntax::ConstNodeList& path, const EbnfSyntax::Node* to );
};

#endif // EBNFANALYZER_H
