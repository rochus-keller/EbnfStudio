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

// This replaces the binned approximation in EbnfAnalyzer by exact First_k sequences.
// The sequence-set fixpoint handles ZeroOrMore/ZeroOrOne correctly via closure loop and epsilon.
// Calculation time for exact is factor ~1000 slower.
// Interestingly, the results for Micron, Ao, LisaPascal, Luon, Sdf and Simula67 are exact the same.
// Algol60 finds two issues (instead of 0), Verilog 243 vs 247, Cedar 32 vs 30, FreePascal 14 vs 16,
// Lua 14 vs 12, Oberon 2 vs 0

#include "EbnfAnalyzer2.h"
#include "EbnfErrors.h"
#include "FirstFollowSet.h"
#include "LaParser.h"
#include <QtDebug>
#include <QRegExp>

EbnfAnalyzer2::EbnfAnalyzer2()
{
}

QSet<QString> EbnfAnalyzer2::collectAllTerminalStrings(EbnfSyntax* syn)
{
    QSet<QString> res;
    foreach( const Ast::Definition* d, syn->getDefs() )
    {
        res += collectAllTerminalStrings( d->d_node );
    }
    return res;
}

QStringList EbnfAnalyzer2::collectAllTerminalProductions(EbnfSyntax* syn)
{
    QStringList res;
    foreach( const Ast::Definition* d, syn->getOrderedDefs() )
    {
        if( d->d_node == 0 && d->d_tok.d_op == EbnfToken::Normal )
            res.append( d->d_tok.d_val.toStr() );
    }
    return res;
}

QSet<QString> EbnfAnalyzer2::collectAllTerminalStrings(Ast::Node* node)
{
    QSet<QString> res;
    if( node == 0 )
        return res;
    if( node->d_type == Ast::Node::Terminal )
        res << node->d_tok.d_val.toStr();
    foreach( Ast::Node* sub, node->d_subs )
        res += collectAllTerminalStrings( sub );
    return res;
}

LlkSequenceSet EbnfAnalyzer2::concatK(const LlkSequenceSet& left, const LlkSequenceSet& right, quint16 k)
{
    LlkSequenceSet result;
    if( left.isEmpty() )
        return result;

    foreach( const LlkSequence& l_seq, left )
    {
        if( l_seq.size() >= k )
        {
            LlkSequence truncated = l_seq.mid(0, k);
            result.insert(truncated);
        }else if( l_seq.isEmpty() )
            result += right;
        else
        {
            if( right.isEmpty() )
                result.insert(l_seq);
            else
            {
                foreach( const LlkSequence& r_seq, right )
                {
                    LlkSequence combined = l_seq;
                    for( int i = 0; i < r_seq.size() && combined.size() < k; ++i )
                        combined.append(r_seq[i]);
                    result.insert(combined);
                }
            }
        }
    }
    return result;
}

LlkSequenceSet EbnfAnalyzer2::evaluateNode(const Ast::Node* node, quint16 k, const FirstKMap& currentMap)
{
    LlkSequenceSet resultSet;
    if( node->doIgnore() )
    {
        resultSet.insert(LlkSequence());
        return resultSet;
    }

    switch( node->d_type )
    {
    case Ast::Node::Terminal:
        {
            LlkSequence seq;
            seq.append(Ast::NodeRef(node));
            resultSet.insert(seq);
        }
        break;

    case Ast::Node::Nonterminal:
        if( node->d_def && node->d_def->d_node )
        {
            resultSet = currentMap.value(node->d_def->d_node);
        }else
        {
            LlkSequence seq;
            seq.append(Ast::NodeRef(node));
            resultSet.insert(seq);
        }
        break;

    case Ast::Node::Alternative:
        foreach( Ast::Node* sub, node->d_subs )
        {
            if( !sub->doIgnore() )
                resultSet += currentMap.value(sub);
        }
        break;

    case Ast::Node::Sequence:
        {
            resultSet.insert(LlkSequence());
            foreach( Ast::Node* sub, node->d_subs )
            {
                if( !sub->doIgnore() )
                    resultSet = concatK(resultSet, currentMap.value(sub), k);
            }
        }
        break;

    default:
        break;
    }

    if( node->d_quant == Ast::Node::ZeroOrOne )
        resultSet.insert(LlkSequence());
    else if( node->d_quant == Ast::Node::ZeroOrMore )
    {
        LlkSequenceSet closureSet;
        closureSet.insert(LlkSequence());

        LlkSequenceSet currentAccumulator = resultSet;
        for( int i = 1; i <= k; ++i )
        {
            closureSet += currentAccumulator;
            currentAccumulator = concatK(currentAccumulator, resultSet, k);
        }
        resultSet = closureSet;
    }

    return resultSet;
}

void EbnfAnalyzer2::calculateAllFirstK(quint16 k, EbnfSyntax* syn, FirstKMap& outFirstK)
{
    outFirstK.clear();
    QList<const Ast::Node*> allNodes;

    struct Collector
    {
        static void collect(const Ast::Node* n, QList<const Ast::Node*>& list)
        {
            if( !n )
                return;
            list.append(n);
            foreach( Ast::Node* sub, n->d_subs )
                collect(sub, list);
        }
    };

    foreach( const Ast::Definition* def, syn->getOrderedDefs() )
    {
        if( def->d_node && !def->doIgnore() )
            Collector::collect(def->d_node, allNodes);
    }

    foreach( const Ast::Node* node, allNodes )
        outFirstK.insert(node, LlkSequenceSet());

    bool changed;
    do
    {
        changed = false;
        foreach( const Ast::Node* node, allNodes )
        {
            LlkSequenceSet oldSet = outFirstK.value(node);
            LlkSequenceSet newSet = evaluateNode(node, k, outFirstK);
            if( oldSet != newSet )
            {
                outFirstK[node] = newSet;
                changed = true;
            }
        }
    } while( changed );
}

LlkSequenceSet EbnfAnalyzer2::computeFirstKForNode(quint16 k, const Ast::Node* node, EbnfSyntax* syn)
{
    FirstKMap firstKMap;
    calculateAllFirstK(k, syn, firstKMap);
    return firstKMap.value(node);
}

LlkSequenceSet EbnfAnalyzer2::computeFollowK(quint16 k, Ast::Node* seq, int fromIdx,
                                               const Ast::NodeRefSet& upperFollow,
                                               EbnfSyntax* syn)
{
    FirstKMap firstKMap;
    calculateAllFirstK(k, syn, firstKMap);

    LlkSequenceSet followSet;
    followSet.insert(LlkSequence());

    for( int j = fromIdx; j < seq->d_subs.size(); j++ )
    {
        Ast::Node* n = seq->d_subs[j];
        if( n->doIgnore() )
            continue;
        LlkSequenceSet subSet = firstKMap.value(n);
        if( subSet.isEmpty() )
            subSet = evaluateNode(n, k, firstKMap);
        followSet = concatK(followSet, subSet, k);
    }

    bool allCapped = true;
    foreach( const LlkSequence& s, followSet )
    {
        if( s.size() < k )
        {
            allCapped = false;
            break;
        }
    }
    if( !allCapped )
    {
        foreach( const Ast::NodeRef& ref, upperFollow )
        {
            LlkSequence seq;
            seq.append(ref);
            LlkSequenceSet singleSet;
            singleSet.insert(seq);
            followSet = concatK(followSet, singleSet, k);
        }
    }

    return followSet;
}

Ast::NodeRefSet EbnfAnalyzer2::intersectAll(const EbnfAnalyzer2::LlkNodes& lhs,
                                             const EbnfAnalyzer2::LlkNodes& rhs)
{
    if( lhs.size() != rhs.size() )
        return Ast::NodeRefSet();
    int intersects = 0;
    Ast::NodeRefSet sum;
    for( int i = 0; i < lhs.size(); i++ )
    {
        const Ast::NodeRefSet diff = lhs[i] & rhs[i];
        if( ( lhs[i].isEmpty() && rhs[i].isEmpty() ) || !diff.isEmpty() )
        {
            intersects++;
            sum = diff;
        }
    }
    if( intersects == lhs.size() )
        return sum;
    else
        return Ast::NodeRefSet();
}

void EbnfAnalyzer2::calcLlkFirstSet(quint16 k, LlkNodes& res, const Ast::Node* node, FirstFollowSet* tbl)
{
    EbnfSyntax* syn = tbl->getSyntax();
    FirstKMap firstKMap;
    calculateAllFirstK(k, syn, firstKMap);
    LlkSequenceSet seqs = firstKMap.value(node);
    if( seqs.isEmpty() )
        seqs = evaluateNode(node, k, firstKMap);

    res.clear();
    for( int i = 0; i < k; i++ )
        res.append(Ast::NodeRefSet());

    foreach( const LlkSequence& seq, seqs )
    {
        for( int i = 0; i < seq.size() && i < k; i++ )
            res[i].insert(seq[i]);
    }
}

void EbnfAnalyzer2::checkForAmbiguity(FirstFollowSet* set, EbnfErrors* err)
{
    EbnfSyntax* syn = set->getSyntax();
    for( int i = 0; i < syn->getOrderedDefs().size(); i++ )
    {
        const Ast::Definition* d = syn->getOrderedDefs()[i];
        if( d->doIgnore() || ( i != 0 && d->d_usedBy.isEmpty() ) || d->d_node == 0 )
            continue;

        try
        {
            checkForAmbiguity( d->d_node, set, err );
        }catch(...)
        {
            qCritical() << "EbnfAnalyzer2::checkForAmbiguity exception";
        }
    }
}

void EbnfAnalyzer2::checkForAmbiguity(Ast::Node* node, FirstFollowSet* set, EbnfErrors* errs, bool recursive)
{
    if( node == 0 || node->doIgnore() )
        return;

    findAmbiguousAlternatives(node, set, errs);
    findAmbiguousOptionals(node, set, errs);

    if( !recursive )
        return;

    switch( node->d_type )
    {
    case Ast::Node::Sequence:
    case Ast::Node::Alternative:
        foreach( Ast::Node* sub, node->d_subs )
        {
            checkForAmbiguity( sub, set, errs, recursive );
        }
        break;
    default:
        break;
    }
}

Ast::ConstNodeList EbnfAnalyzer2::findPath(const Ast::Node* from, const Ast::Node* to)
{
    Ast::ConstNodeList res;
    if( from == 0 )
        return res;
    res << from;
    if( !findPathImp( res, to ) )
        res.clear();
    return res;
}

void EbnfAnalyzer2::findAmbiguousAlternatives(Ast::Node* node, FirstFollowSet* set, EbnfErrors* errs)
{
    if( node->d_type != Ast::Node::Alternative )
        return;

    for( int i = 0; i < node->d_subs.size(); i++ )
    {
        for( int j = i + 1; j < node->d_subs.size(); j++ )
        {
            const Ast::Node* a = node->d_subs[i];
            const Ast::Node* b = node->d_subs[j];
            if( a->doIgnore() || b->doIgnore() )
                continue;

            const Ast::NodeRefSet diff = set->getFirstSet(a) & set->getFirstSet(b);
            if( diff.isEmpty() )
                continue;

            const Ast::Node* predA = EbnfSyntax::firstPredicateOf(a);
            const Ast::Node* predB = EbnfSyntax::firstPredicateOf(b);
            int ll = 0;
            if( predA != 0 )
                ll = predA->getLlk();
            if( predB != 0 )
                ll = qMax( ll, predB->getLlk() );

            // LA predicates use boolean expressions (AND/OR/NOT) over specific lookahead positions
            // to resolve ambiguities that plain LL(k) cannot handle. Accept them as-is;
            // full validation of the boolean expression is future work.
            if( (predA && !predA->getLa().isEmpty()) || (predB && !predB->getLa().isEmpty()) )
                continue;

            if( ll > 0 )
            {
                FirstKMap firstKMap;
                calculateAllFirstK(ll, set->getSyntax(), firstKMap);

                LlkSequenceSet llkA = firstKMap.value(a);
                if( llkA.isEmpty() )
                    llkA = evaluateNode(a, ll, firstKMap);
                LlkSequenceSet llkB = firstKMap.value(b);
                if( llkB.isEmpty() )
                    llkB = evaluateNode(b, ll, firstKMap);

                LlkSequenceSet intersect = llkA & llkB;

                if( intersect.isEmpty() )
                    continue;

                const Ast::Node* pred = predA != 0 ? predA : predB;
                const Ast::Node* other = predA != 0 ? b : a;
                if( pred )
                    errs->warning(EbnfErrors::Analysis, pred->d_tok.d_lineNr, pred->d_tok.d_colNr,
                            QString("predicate not effective for LL(%1)").arg(ll),
                                  QVariant::fromValue(EbnfSyntax::IssueData(EbnfSyntax::IssueData::BadPred,
                                                                            pred,other)));
            }

            if( ll > 0 )
                continue;

            if( !diff.isEmpty() )
            {
                const Ast::Node* aa = EbnfSyntax::firstVisibleElementOf(a);
                Ast::NodeSet diff2 = EbnfSyntax::collectNodes( diff, set->getFirstNodeSet(a) );
                diff2 += EbnfSyntax::collectNodes( diff, set->getFirstNodeSet(b) );
                errs->error(EbnfErrors::Analysis, aa->d_tok.d_lineNr, aa->d_tok.d_colNr,
                            QString("alternatives %1 and %2 are LL(1) ambiguous because of %3")
                            .arg(i+1).arg(j+1).arg(EbnfSyntax::pretty(diff)),
                            QVariant::fromValue(EbnfSyntax::IssueData(
                                                    EbnfSyntax::IssueData::AmbigAlt,a,b,diff2.toList())));
            }
        }
    }
}

void EbnfAnalyzer2::findAmbiguousOptionals(Ast::Node* seq, FirstFollowSet* set, EbnfErrors* errs)
{
    if( seq->d_type != Ast::Node::Sequence )
        return;

    Ast::NodeRefSet upperFollow = set->getFollowSet(seq);
    for( int i = 0; i < seq->d_subs.size(); i++ )
    {
        const Ast::Node* a = seq->d_subs[i];
        if( a->doIgnore() || !a->isNullable() )
            continue;
        Ast::NodeRefSet follow;
        bool nonNullableFound = false;
        const Ast::Node* b = 0;
        for( int j = i + 1; j < seq->d_subs.size(); j++ )
        {
            const Ast::Node* n = seq->d_subs[j];
            if( n->doIgnore() )
                continue;
            if( b == 0 )
                b = n;
            follow += set->getFirstSet(n);
            if( !n->isNullable() )
            {
                nonNullableFound = true;
                break;
            }
        }
        if( !nonNullableFound )
            follow += upperFollow;

        const Ast::NodeRefSet diff = set->getFirstSet(a) & follow;
        if( diff.isEmpty() )
            continue;

        const Ast::Node* pred = EbnfSyntax::firstPredicateOf(a);
        int ll = 0;
        if( pred != 0 )
        {
            if( !pred->getLa().isEmpty() )
            {
                // LA predicates use boolean expressions over specific lookahead positions
                // to resolve ambiguities that plain LL(k) cannot handle. Accept them as-is;
                // full validation of the boolean expression is future work.
                continue;
            }

            ll = pred->getLlk();
            if( ll > 0 )
            {
                FirstKMap firstKMap;
                calculateAllFirstK(ll, set->getSyntax(), firstKMap);

                LlkSequenceSet pathTake = firstKMap.value(a);
                if( pathTake.isEmpty() )
                    pathTake = evaluateNode(a, ll, firstKMap);

                // remove epsilon: we only want the "take" sequences, not the "skip" (epsilon) path
                pathTake.remove(LlkSequence());
                if( pathTake.isEmpty() )
                    continue;

                LlkSequenceSet pathSkip;
                pathSkip.insert(LlkSequence());

                for( int j = i + 1; j < seq->d_subs.size(); j++ )
                {
                    Ast::Node* sub = seq->d_subs[j];
                    if( sub->doIgnore() )
                        continue;
                    LlkSequenceSet nextSet = firstKMap.value(sub);
                    if( nextSet.isEmpty() )
                        nextSet = evaluateNode(sub, ll, firstKMap);
                    pathTake = concatK(pathTake, nextSet, ll);
                    pathSkip = concatK(pathSkip, nextSet, ll);
                }

                LlkSequenceSet intersect = pathTake & pathSkip;
                if( intersect.isEmpty() )
                    continue;

                errs->warning(EbnfErrors::Analysis, pred->d_tok.d_lineNr, pred->d_tok.d_colNr,
                            QString("predicate not effective for LL(%1)").arg(ll),
                              QVariant::fromValue(EbnfSyntax::IssueData(
                                EbnfSyntax::IssueData::BadPred,pred, b ? b : seq)) );
            }
        }
        Ast::NodeSet diff2 = EbnfSyntax::collectNodes( diff, set->getFirstNodeSet(a) );
        if( b != 0 )
            diff2 += EbnfSyntax::collectNodes( diff, set->getFirstNodeSet(b) );
        reportAmbig( seq, i, diff, diff2, set, errs );
    }
}

void EbnfAnalyzer2::reportAmbig(Ast::Node* sequence, int ambigIdx, const Ast::NodeRefSet& ambigSet,
                                const Ast::NodeSet& ambigSet2, FirstFollowSet* set, EbnfErrors* errs)
{
    Ast::Node* a = sequence->d_subs[ambigIdx];
    const Ast::Node* start = EbnfSyntax::firstVisibleElementOf(sequence);
    QString ofSeq;
    if( start && start != a )
        ofSeq = QString("start. w. '%1' ").arg(start->d_tok.d_val.toStr());

    const Ast::Node* next = 0;
    bool fullAmbig = false;
    for( int j = ambigIdx + 1; j < sequence->d_subs.size(); j++ )
    {
        const Ast::NodeRefSet diffDiff = ambigSet & set->getFirstSet(sequence->d_subs[j]);
        if( !diffDiff.isEmpty() )
        {
            if( diffDiff == ambigSet )
                fullAmbig = true;
            next = EbnfSyntax::firstVisibleElementOf(sequence->d_subs[j]);
            break;
        }
    }
    QString ambig = "w. successors";
    if( next )
    {
        QString dots;
        if( !fullAmbig )
            dots = "...";
        ambig = QString("w. '%1'%2 ").arg(next->d_tok.d_val.toStr()).arg(dots);
    }else
        next = sequence;

    errs->warning(EbnfErrors::Analysis, a->d_tok.d_lineNr, a->d_tok.d_colNr,
                QString("opt. elem. %1 of seq. %2is LL(1) ambig. %3 because of %4")
                .arg(ambigIdx+1).arg(ofSeq).arg(ambig).arg(EbnfSyntax::pretty(ambigSet)),
                  QVariant::fromValue(EbnfSyntax::IssueData(
                                          EbnfSyntax::IssueData::AmbigOpt,a,next,ambigSet2.toList())));
}

bool EbnfAnalyzer2::findPathImp(Ast::ConstNodeList& path, const Ast::Node* to)
{
    if( path.isEmpty() )
        return false;
    const Ast::Node* node = path.last();
    if( node->doIgnore() )
        return false;

    switch( node->d_type )
    {
    case Ast::Node::Terminal:
        if( node == to )
            return true;
        break;
    case Ast::Node::Nonterminal:
        if( node->d_def && node->d_def->d_node )
        {
            if( path.contains(node->d_def->d_node) )
                return false;
            path.append(node->d_def->d_node);
            if( findPathImp( path, to ) )
                return true;
            path.pop_back();
        }else if( node == to )
        {
            return true;
        }
        break;
    case Ast::Node::Sequence:
        foreach( Ast::Node* sub, node->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            path.append(sub);
            if( findPathImp( path, to ) )
                return true;
            path.pop_back();
            if( !sub->isNullable() )
                break;
        }
        break;
    case Ast::Node::Alternative:
        foreach( Ast::Node* sub, node->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            path.append(sub);
            if( findPathImp( path, to ) )
                return true;
            path.pop_back();
        }
        break;
    default:
        break;
    }
    if( path.size() == 1 )
    {
        if( const Ast::Node* next = node->getNext() )
        {
            path.append(next);
            if( findPathImp( path, to ) )
                return true;
            path.pop_back();
        }else
        {
            foreach( const Ast::Node* use, node->d_owner->d_usedBy )
            {
                const Ast::Node* next = use->getNext();
                if( next )
                    path.append(next);
                else
                    path.append(use);
                if( findPathImp( path, to ) )
                    return true;
                path.pop_back();
            }
        }
    }
    return false;
}

int EbnfAnalyzer2::getMaxLaIndex(const Ast::Node* pred)
{
    if( pred == 0 || pred->d_type != Ast::Node::Predicate )
        return 0;

    const QByteArray la = pred->getLa();
    if( la.isEmpty() )
        return 0;

    // parse the LA expression and walk the AST to find the maximum index
    LaParser p;
    if( !p.parse(la) )
        return 0;

    struct MaxIndexWalker
    {
        static int walk( const LaParser::Ast* ast )
        {
            if( ast == 0 )
                return 0;
            int maxIdx = 0;
            if( ast->d_type == LaParser::Ast::La )
                maxIdx = ast->d_val.toInt();
            foreach( const LaParser::AstRef& sub, ast->d_subs )
            {
                const int subIdx = walk(sub.constData());
                if( subIdx > maxIdx )
                    maxIdx = subIdx;
            }
            return maxIdx;
        }
    };

    return MaxIndexWalker::walk(p.getLaExpr().constData());
}

bool EbnfAnalyzer2::checkLaPredicate(const Ast::Node* pred, quint16 k,
                                      const LlkSequenceSet& pathA, const LlkSequenceSet& pathB)
{
    Q_UNUSED(pred);
    Q_UNUSED(k);
    LlkSequenceSet intersect = pathA & pathB;
    return intersect.isEmpty();
}
