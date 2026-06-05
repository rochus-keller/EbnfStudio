#ifndef EBNFANALYZER2_H
#define EBNFANALYZER2_H

/*
* Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>
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
#include <QList>
#include <QSet>
#include <QHash>

class FirstFollowSet;

typedef QList<Ast::NodeRef> LlkSequence;

inline uint qHash(const LlkSequence& key, uint seed = 0)
{
    uint hash = seed;
    for( int i = 0; i < key.size(); ++i )
        hash ^= Ast::qHash(key.at(i)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}

typedef QSet<LlkSequence> LlkSequenceSet;

class EbnfAnalyzer2
{
public:
    EbnfAnalyzer2();

    static QSet<QString> collectAllTerminalStrings( EbnfSyntax* ); // identical with EbnfAnalyzer
    static QStringList collectAllTerminalProductions( EbnfSyntax* ); // identical

    typedef QList<Ast::NodeRefSet> LlkNodes;
    static Ast::NodeRefSet intersectAll( const LlkNodes& lhs, const LlkNodes& rhs ); // identical

    static void calcLlkFirstSet(quint16 k, LlkNodes&, const Ast::Node* node, FirstFollowSet* ); // improved over EbnfAnalyzer

    static void checkForAmbiguity( FirstFollowSet*, EbnfErrors*); // improved
    static void checkForAmbiguity( Ast::Node*, FirstFollowSet*, EbnfErrors*, bool recursive = true ); // improved

    static Ast::ConstNodeList findPath( const Ast::Node* from, const Ast::Node* to ); // identicals

    static LlkSequenceSet computeFirstKForNode(quint16 k, const Ast::Node* node, EbnfSyntax* syn);

protected:
    typedef QHash<const Ast::Node*, LlkSequenceSet> FirstKMap;

    static void calculateAllFirstK(quint16 k, EbnfSyntax* syn, FirstKMap& outFirstK);
    static LlkSequenceSet evaluateNode(const Ast::Node* node, quint16 k, const FirstKMap& currentMap);
    static LlkSequenceSet concatK(const LlkSequenceSet& left, const LlkSequenceSet& right, quint16 k);
    static LlkSequenceSet computeFollowK(quint16 k, Ast::Node* seq, int fromIdx,
                                          const Ast::NodeRefSet& upperFollow,
                                          EbnfSyntax* syn);

    static QSet<QString> collectAllTerminalStrings( Ast::Node* );
    static void findAmbiguousAlternatives( Ast::Node*, FirstFollowSet*, EbnfErrors* );
    static void findAmbiguousOptionals( Ast::Node*, FirstFollowSet*, EbnfErrors* );
    static void reportAmbig(Ast::Node* seq, int ambigIdx, const Ast::NodeRefSet& diff,
                            const Ast::NodeSet& ambigSet2, FirstFollowSet*, EbnfErrors* );
    static bool findPathImp( Ast::ConstNodeList& path, const Ast::Node* to );
    static int getMaxLaIndex( const Ast::Node* pred );
    static bool checkLaPredicate(const Ast::Node* pred, quint16 k,
                                  const LlkSequenceSet& pathA, const LlkSequenceSet& pathB);
};

#endif // EBNFANALYZER2_H
