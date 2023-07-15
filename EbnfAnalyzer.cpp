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

#include "EbnfAnalyzer.h"
#include "EbnfErrors.h"
#include "FirstFollowSet.h"
#include <QtDebug>

// https://stackoverflow.com/questions/19529560/left-recursive-grammar-identification

EbnfAnalyzer::EbnfAnalyzer()
{

}

QSet<QString> EbnfAnalyzer::collectAllTerminalStrings(EbnfSyntax* syn)
{
    QSet<QString> res;
    foreach( const Ast::Definition* d, syn->getDefs() )
    {
        res += collectAllTerminalStrings( d->d_node );
    }
    return res;
}

QStringList EbnfAnalyzer::collectAllTerminalProductions(EbnfSyntax* syn)
{
    QStringList res;
    foreach( const Ast::Definition* d, syn->getOrderedDefs() )
    {
        if( d->d_node == 0 && d->d_tok.d_op == EbnfToken::Normal )
            res.append( d->d_tok.d_val.toStr() );
    }
    return res;
}

QSet<QString> EbnfAnalyzer::collectAllTerminalStrings(Ast::Node* node)
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

static inline void resize( EbnfAnalyzer::LlkNodes& l, quint16 curBin )
{
    if( l.size() <= curBin )
    {
        const int max = curBin + 1 - l.size();
        for( int i = 0; i < max; i++ )
            l.append( Ast::NodeRefSet() );
    }
}

static inline int countNotEmpty( const EbnfAnalyzer::LlkNodes& l )
{
    int res = 0;
    foreach( const Ast::NodeRefSet& s, l )
        if( !s.isEmpty() )
            res++;
    return res;
}

static QByteArray ws( int level )
{
    QByteArray res;
    for( int i = 0; i < qAbs(level); i++ )
        res += " | ";
    return res;
}

struct _SubNodeBin
{
    const Ast::Node* d_node;
    quint16 d_from, d_to;
    _SubNodeBin( const Ast::Node* n, int from, int to ):d_node(n),d_from(from),d_to(to){}
};

quint16 EbnfAnalyzer::calcLlkFirstSetImp(quint16 k, quint16 curBin, LlkNodes& res, const Ast::Node* node, FirstFollowSet* tbl, int level)
{
    // Gehe entlang der Blätter des Baums und sammle alle Terminals ein gruppiert in Distanzboxen.
    // Wird eigentlich nur mit node=Sequence aufgerufen, da ja Predicates nur dort vorkommen

    // returns maximum number of symbols covered by this node

    if( node == 0 || node->doIgnore() )
        return 0;

//#ifdef _DEBUG
#if 0
    qDebug() << ws(level).constData() << "visit bin" << curBin << "l/c" <<
                node->d_tok.d_lineNr << node->d_tok.d_colNr << node->toString()
             << "level" << level << "k" << k << "reslen" << res.size(); // TEST
#endif

    if( level > k * 10 )
    {
#ifdef _DEBUG
        qCritical() << ws(level).constData() << "calcLlkFirstSet level depth limit hit at " << level << k * 10;
#endif
        return 0;
    }

    switch( node->d_type )
    {
    case Ast::Node::Terminal:
        resize( res, curBin );
        res[curBin].insert( node ); // hier ist dieser node gemeint, nicht Follow(node)!
//#ifdef _DEBUG
#if 0
        qDebug() << ws(level).constData() << "insert" << curBin <<
                    node->d_tok.d_val.toBa() << node->d_tok.d_lineNr << node->d_tok.d_colNr; // TEST
#endif
        return 1;

    case Ast::Node::Nonterminal:
        if( node->d_def && node->d_def->d_node )
            return calcLlkFirstSetImp( k, curBin, res, node->d_def->d_node, tbl,level+1 );
        else
        {
            // wie Terinal
            resize( res, curBin );
            res[curBin].insert( node );
//#ifdef _DEBUG
#if 0
            qDebug() << ws(level).constData() << "insert" << curBin <<
                        node->d_tok.d_val.toBa() << node->d_tok.d_lineNr << node->d_tok.d_colNr; // TEST
#endif
            return 1;
        }

    case Ast::Node::Sequence:
#if 0
        foreach( Ast::Node* sub, node->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            calcLlkFirstSet( k, curBin++, res, sub, tbl,level+1 );
            if( res.size() >= k )
                break;
        }
        // TODO: repetitions and options
#else
        {
            QList<_SubNodeBin> toVisit;
            int i = curBin;
            int nullable = 0;
            foreach( Ast::Node* sub, node->d_subs )
            {
                if( !sub->doIgnore() )
                {
                    toVisit << _SubNodeBin(sub,i-nullable,i);
                    i++;
                    if( sub->isNullable() )
                        nullable++;
                }
            }
            int max = 0;
            for( int i = 0; i < toVisit.size(); i++ )
            {
                for( int bin = toVisit[i].d_from; bin <= toVisit[i].d_to && bin < k; bin++ )
                {
                    const quint16 tmp = calcLlkFirstSetImp( k, bin, res, toVisit[i].d_node, tbl, level+1 );
                    if( tmp > 1 )
                    {
                        for( int j = i+1; j < toVisit.size(); j++ )
                            toVisit[j].d_to += tmp - 1;
                    }
                    if( tmp > 0 && bin > max )
                        max = bin;
                }
            }
            return max - curBin + 1;
        }
        // TODO: repetition
#endif
        break;
    case Ast::Node::Alternative:
        {
            quint16 count = 0;
            foreach( Ast::Node* sub, node->d_subs )
            {
                if( sub->doIgnore() )
                    continue;
                const quint16 tmp = calcLlkFirstSetImp( k, curBin, res, sub, tbl,level+1 );
                if( tmp > count )
                    count = tmp;
            }
            return count;
        }

    default:
        break;
    }
    return 0;
}

void EbnfAnalyzer::calcLlkFirstSet2Imp(quint16 k, int curBin, int level, LlkNodes& res,
                                    const Ast::Node* node, FirstFollowSet* tbl, CheckSet& visited)
{
    if( node == 0 || node->doIgnore() )
        return;
    if( visited.contains(node) )
        return;
    else
        visited.insert(node);

//#ifdef _DEBUG
#if 0
    qDebug() << ws(level).constData() << "visit bin" << curBin << "l/c" <<
                node->d_tok.d_lineNr << node->d_tok.d_colNr << node->toString()
             << "level" << level << "k" << k << "res len" << res.size(); // TEST
#endif

    // TODO: ev. separate Funktion um von einem Node zum nächsten zu wandern und im Falle von usedBy und Alternative
    // mehrere parallele Nodes zurückzugeben. Offen ist die Ermittlung des curBin.

    if( level >= 0 )
    {
        // CheckSet visited; // don't check visited downwards
        switch( node->d_type )
        {
        case Ast::Node::Terminal:
            resize( res, curBin );
            res[curBin].insert( node ); // hier ist dieser node gemeint, nicht Follow(node)!
//#ifdef _DEBUG
#if 0
            qDebug() << ws(level).constData() << "insert" << curBin <<
                        node->d_tok.d_val.data() << node->d_tok.d_lineNr << node->d_tok.d_colNr; // TEST
#endif
            break;
        case Ast::Node::Nonterminal:
            if( node->d_def && node->d_def->d_node )
                calcLlkFirstSet2Imp( k, curBin, level + 1, res, node->d_def->d_node, tbl, visited );
            else
            {
                // wie Terinal
                resize( res, curBin );
                res[curBin].insert( node );
//#ifdef _DEBUG
#if 0
                qDebug() << ws(level).constData() << "insert" << curBin <<
                            node->d_tok.d_val.data() << node->d_tok.d_lineNr << node->d_tok.d_colNr; // TEST
#endif
            }
            break;
        case Ast::Node::Sequence:
#if 1
            foreach( Ast::Node* sub, node->d_subs )
            {
                if( sub->doIgnore() )
                    continue;
                calcLlkFirstSet2Imp( k, curBin++, level + 1, res, sub, tbl, visited );
                if( res.size() >= k )
                    break;
            }
            // TODO: repetition, option
#else
#if 1       // TODO the following does not work yet
            {
                QList<const Ast::Node*> toVisit;
                foreach( Ast::Node* sub, node->d_subs )
                {
                    if( !sub->doIgnore() )
                        toVisit << sub;
                }
                for( int i = curBin; i < k && i < toVisit.size(); i++ )
                {
                    for( int j = i; j < toVisit.size(); j++ )
                    {
                        visited.clear();
                        calcLlkFirstSet2Imp( k, i, level + 1, res, toVisit[j], tbl, visited ); // TODO: visited too strong
                        if( !toVisit[j]->isNullable() )
                            break;
                    }
                    if( res.size() >= k )
                        break;
                }
            }
            // TODO: repetition
#else
            for( int i = curBin; i < k; i++ )
            {
                QSet<const Ast::Node*> check = visited;
                int j = 0;
                foreach( Ast::Node* sub, node->d_subs )
                {
                    if( sub->doIgnore() )
                        continue;
                    if( j++ < i )
                        continue;
                    calcLlkFirstSet2Imp( k, i, level + 1, res, sub, tbl, check );
                    if( !sub->isNullable() )
                        break;
                }
                //if( res.size() >= k )
                //    break;
            }
            // TODO: repetition
#endif
#endif
            break;
        case Ast::Node::Alternative:
            foreach( Ast::Node* sub, node->d_subs )
            {
                if( sub->doIgnore() )
                    continue;
                calcLlkFirstSet2Imp( k, curBin, level + 1, res, sub, tbl, visited );
            }
            // TODO: repetition
            break;
        default:
            break;
        }
    }
    // Vermutlich unnötig, auch nach oben zu suchen
    if( level <= 0 && res.size() < k )
    {
        // Vorsicht, es kann sein dass wir beim Weg nach oben wieder dort vorbeikommen, wo man angefangen haben
        curBin = res.size() - 1;
        if( const Ast::Node* next = node->getNext(&curBin) )
        {
            // finde nächsten nach node
            calcLlkFirstSet2Imp(k,curBin,0,res,next,tbl, visited);
            if( res.size() >= k )
                return;
        }else
        {
            // gehe entlang node->d_owner->d_usedBy
            foreach( const Ast::Node* use, node->d_owner->d_usedBy )
            {
                int index = curBin;
                const Ast::Node* next = use->getNext(&index);
                if( next )
                    calcLlkFirstSet2Imp( k, index, 0, res, next, tbl, visited );
                else // obsolet wegen visited: if( level > -10 ) // begrenze die Tiefe, da es hier ab und zu unendlich weitergeht TODO
                    calcLlkFirstSet2Imp( k,index,level - 1 , res, use, tbl, visited );
            }
        }
    }
}

bool EbnfAnalyzer::findPath(Ast::ConstNodeList& path, const Ast::Node* to)
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
            if( findPath( path, to ) )
                return true;
            path.pop_back();
        }else if( node == to )
        {
            // wie Terinal
            return true;
        }
        break;
    case Ast::Node::Sequence:
        foreach( Ast::Node* sub, node->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            path.append(sub);
            if( findPath( path, to ) )
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
            if( findPath( path, to ) )
                return true;
            path.pop_back();
        }
        break;
    default:
        break;
    }
    if( path.size() == 1 )
    {
        // Wir sind zuoberst und haben noch nichts gefunden
        if( const Ast::Node* next = node->getNext() )
        {
            // finde nächsten nach node
            path.append(next);
            if( findPath( path, to ) )
                return true;
            path.pop_back();
        }else
        {
            // gehe entlang node->d_owner->d_usedBy
            foreach( const Ast::Node* use, node->d_owner->d_usedBy )
            {
                const Ast::Node* next = use->getNext();
                if( next )
                    path.append(next);
                else
                    path.append(use);
                if( findPath( path, to ) )
                    return true;
                path.pop_back();
            }
        }
    }
    return false;
}

Ast::NodeRefSet EbnfAnalyzer::intersectAll(const EbnfAnalyzer::LlkNodes& lhs, const EbnfAnalyzer::LlkNodes& rhs)
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
            //sum += diff;
            sum = diff; // behalte nur das letzte Diff
        }
    }
    if( intersects == lhs.size() )
        return sum;
    else
        return Ast::NodeRefSet();
}

void EbnfAnalyzer::calcLlkFirstSet(quint16 k, EbnfAnalyzer::LlkNodes& res, const Ast::Node* node, FirstFollowSet* tbl)
{
    calcLlkFirstSetImp( k, 0, res, node, tbl, 0 );
}

void EbnfAnalyzer::calcLlkFirstSet2(quint16 k, EbnfAnalyzer::LlkNodes& res, const Ast::Node* node, FirstFollowSet* tbl)
{
    QSet<const Ast::Node*> visited;
    calcLlkFirstSet2Imp( k, 0, 0,res, node, tbl, visited );
}

void EbnfAnalyzer::checkForAmbiguity(FirstFollowSet* set, EbnfErrors* err)
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
            qCritical() << "EbnfAnalyzer::checkForAmbiguity exception";
        }
    }
}

void EbnfAnalyzer::checkForAmbiguity(Ast::Node* node, FirstFollowSet* set, EbnfErrors* errs , bool recursive)
{
    if( node == 0 || node->doIgnore() )
        return;

    findAmbiguousAlternatives(node,set,errs);
    findAmbiguousOptionals(node,set,errs);

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

Ast::ConstNodeList EbnfAnalyzer::findPath(const Ast::Node* from, const Ast::Node* to)
{
    Ast::ConstNodeList res;
    if( from == 0 )
        return res;
    res << from;
    if( !findPath( res, to ) )
        res.clear();
    return res;
}

void EbnfAnalyzer::findAmbiguousAlternatives(Ast::Node* node, FirstFollowSet* set, EbnfErrors* errs)
{
    if( node->d_type != Ast::Node::Alternative )
        return;

    // Check each pair of alternatives
    for(int i = 0; i < node->d_subs.size(); i++)
    {
        for(int j = i + 1; j < node->d_subs.size(); j++)
        {
            const Ast::Node* a = node->d_subs[i];
            const Ast::Node* b = node->d_subs[j];
            if( a->doIgnore() || b->doIgnore() )
                continue;
            // TODO: wenn eine Alternative Nullable ist, müsste auch noch ihre Follow mitberücksichtigt werden!
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

            // TODO: each alternative might have a different predicate type LL or LA
            // currently just assume everything is ok if an LA predicate is present
            if( (predA && !predA->getLa().isEmpty()) || (predB && !predB->getLa().isEmpty()) )
                continue;

            if( ll > 0 )
            {
                EbnfAnalyzer::LlkNodes llkA;
                calcLlkFirstSet2( ll,  llkA, a, set );
                EbnfAnalyzer::LlkNodes llkB;
                calcLlkFirstSet2( ll, llkB, b, set );

                const Ast::NodeRefSet diff = intersectAll( llkA, llkB );
                if( llkA.size() == llkB.size() && llkA.size() == ll && diff.isEmpty() )
                    continue;

                const Ast::Node* pred = predA != 0 ? predA : predB;
                const Ast::Node* other = predA != 0 ? b : a;
                if( pred )
                    errs->warning(EbnfErrors::Analysis, pred->d_tok.d_lineNr, pred->d_tok.d_colNr,
                            QString("predicate not effective for LL(%1)").arg(ll),
                                  QVariant::fromValue(EbnfSyntax::IssueData(EbnfSyntax::IssueData::BadPred,
                                                                            pred,other)));

            }

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

void EbnfAnalyzer::findAmbiguousOptionals(Ast::Node* seq, FirstFollowSet* set, EbnfErrors* errs)
{
    // ZeroOrOne and ZeroOrMore haben genau dieselben Mehrdeutigkeitskriterien. Es ist egal, ob
    // a nur ein oder mehrmals wiederholt wird; in jedem Fall ist die Schnittmenge zwischen First(a) und Follow(a)
    // massgebend!

    if( seq->d_type != Ast::Node::Sequence )
        return;

    Ast::NodeRefSet upperFollow = set->getFollowSet(seq);
    for(int i = 0; i < seq->d_subs.size(); i++)
    {
        const Ast::Node* a = seq->d_subs[i];
        if( a->doIgnore() || !a->isNullable() )
            continue;
        Ast::NodeRefSet follow; // = set->getFollowSet(1,a);
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

        // else
        const Ast::Node* pred = EbnfSyntax::firstPredicateOf(a);
        int ll = 0;
        if( pred != 0 )
        {
            // TODO: currently just assume everything is ok if an LA predicate is present
            if( !pred->getLa().isEmpty() )
                continue;

            ll = pred->getLlk();
            if( ll > 0 )
            {
                EbnfAnalyzer::LlkNodes llkA;
                calcLlkFirstSet2( ll,  llkA, a, set );

                EbnfAnalyzer::LlkNodes llkB;
                if( b == 0 )
                {
                    QSet<const Ast::Node*> visited;
                    calcLlkFirstSet2Imp(ll,0,-1,llkB, seq, set, visited );
                }else
                    calcLlkFirstSet2( ll, llkB, b, set );

                const Ast::NodeRefSet diff = intersectAll( llkA, llkB );
                if( llkA.size() == llkB.size() && llkA.size() == ll && diff.isEmpty() )
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
//        if( diff.size() != diff2.size() )
//            qDebug() << "findAmbigOptions different diff size";
        reportAmbig( seq, i, diff, diff2, set, errs );
    }
}

void EbnfAnalyzer::reportAmbig(Ast::Node* sequence, int ambigIdx, const Ast::NodeRefSet& ambigSet,
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

