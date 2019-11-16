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

#include "FirstFollowSet.h"
#include <QtDebug>

FirstFollowSet::FirstFollowSet(QObject *parent) : QObject(parent),d_includeNts(false)
{

}

void FirstFollowSet::setSyntax(EbnfSyntax* syn)
{
    if( syn == d_syn.constData() )
        return;
    clear();
    d_syn = syn;
    calculateFirstSets();
    calculateFollowSets();
}

void FirstFollowSet::setIncludeNts(bool on)
{
    d_includeNts = on;
}

void FirstFollowSet::clear()
{
    d_syn = 0;
    d_first.clear();
    d_follow.clear();
}

Ast::NodeSet FirstFollowSet::getFirstNodeSet(const Ast::Node* node, bool cache) const
{
    Ast::NodeSet res = d_first.value(node);
    if( res.isEmpty() )
    {
        res = calculateFirstSet(node);
        if( !res.isEmpty() && cache )
        {
            FirstFollowSet* set = const_cast<FirstFollowSet*>(this);
            set->d_first[node] = res;
        }
    }
    return res;
}

Ast::NodeRefSet FirstFollowSet::getFirstSet(const Ast::Node* node, bool cache ) const
{
    // TODO: cache if need be
    return EbnfSyntax::nodeToRefSet( getFirstNodeSet( node, cache ) );
}

Ast::NodeRefSet FirstFollowSet::getFirstSet(const Ast::Definition* d) const
{
    if( d == 0 )
        return Ast::NodeRefSet();
    return getFirstSet( d->d_node );
}

Ast::NodeSet FirstFollowSet::getFollowNodeSet(const Ast::Node* node) const
{
    Ast::NodeSet res = d_follow.value(node);
    /* Wenn man die Repeats hier berechnet, kommt nicht dasselbe raus wie wenn man sie in calculateFollowSet2 berechnet!
    if( node->d_quant == Ast::Node::ZeroOrMore && doRepeats )
        // Wenn das Element wiederholt wird, erscheint auch sein eigenes First-Set als Teil des Follow-Sets
        res += getFirstSet(1,node);
        */
    return res;
}

Ast::NodeRefSet FirstFollowSet::getFollowSet(const Ast::Definition* d) const
{
    if( d == 0 )
        return Ast::NodeRefSet();
    return getFollowSet( d->d_node );
}

Ast::NodeRefSet FirstFollowSet::getFollowSet(const Ast::Node* node) const
{
    // TODO: cache if need be
    return EbnfSyntax::nodeToRefSet( getFollowNodeSet( node ) );
}

Ast::NodeSet FirstFollowSet::calculateFirstSet(const Ast::Node* node) const
{
    if( node == 0 || node->doIgnore() )
        return Ast::NodeSet();

    Ast::NodeSet res;
    switch( node->d_type )
    {
    case Ast::Node::Terminal:
        res << node;
        break;
    case Ast::Node::Alternative:
        foreach( Ast::Node* sub, node->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            // eine Alternative kann in jedes der Elemente verzweigen, also ist der Union das First Set
            res += calculateFirstSet( sub );
        }
        break;
    case Ast::Node::Sequence:
        foreach( Ast::Node* sub, node->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            res += calculateFirstSet( sub );
            if( !sub->isNullable() )
                break;
        }
        break;
    case Ast::Node::Nonterminal:
        if( !node->doIgnore() )
        {
            if( node->d_def == 0 || node->d_def->d_node == 0 )
                res << node; // unechtes Terminal
            else
            {
                res = d_first[node->d_def->d_node];
                if( d_includeNts )
                    res += node;
            }
        }
        break;
    case Ast::Node::Predicate:
        // ignore
        Q_ASSERT(false); // wir können nie hier landen wegen oberstem node->ignore
        break;
    }
    // Cache nützt während First-Kalkulation noch nichts,
    // da während der Kalkulation über alle Definitions auch auf Ebene der Nodes einiges ändert.
    return res;
}

void FirstFollowSet::calculateFirstSets()
{
    // Implement algorithm by a fixed-point iteration

    bool changed;
    do
    {
        changed = false;
        foreach( Ast::Definition* d, d_syn->getOrderedDefs() )
        {
            if( d->d_node == 0 // pseudoterminal, wird nicht hier behandelt, sondern beim NT, das darauf zeigt
                    || d->doIgnore() )
                continue;
            Ast::NodeSet newValue = calculateFirstSet(d->d_node);
            if( newValue != d_first[d->d_node] )
            {
                d_first[d->d_node] = newValue;
                changed = true;
            }
        }
    }while( changed );

    // The computation will terminate because
    // - the variables are changed monotonically (using set union)
    // - the largest possible set is finite: T, the set of all tokens
    // - the number of possible changes is therefore finite

    // ich habe verifiziert, dass dieser altorithmus genau denselben output liefert wie der rekursive
    // für den rekursiven galt:
    // 2019-02-05 Output des Algorithmus mit Coco/R verglichen. Stimmt überein.

    // dieser algo brauch 21 ms.
    // der rekursive brauchte 20 ms.
}


void FirstFollowSet::calculateFollowSets()
{
    bool changed;
    do
    {
        changed = false;
        for( int i = 0; i < d_syn->getOrderedDefs().size(); i++ )
        {
            Ast::Definition* def = d_syn->getOrderedDefs()[i];
            if( def->d_node == 0 || def->doIgnore() )
                continue;
            //qDebug() << def->d_tok.d_val;
            //changed |= calculateFollowSet(d);
            changed |= calculateFollowSet2(def->d_node, false);
        }
    }while( changed );

}

static inline bool isNt( const Ast::Node* node )
{
    return node->d_type == Ast::Node::Nonterminal ||
            node->d_type == Ast::Node::Sequence ||
            node->d_type == Ast::Node::Alternative;
}

static bool add(FirstFollowSet::Lookup& lkp, const Ast::Node* node, const Ast::NodeSet& rhs )
{
    if( node->d_def )
    {
        Q_ASSERT(node->d_type == Ast::Node::Nonterminal );
        node = node->d_def->d_node;
    }
    Ast::NodeSet& orig = lkp[node];
    Ast::NodeSet lhs = orig;
    lhs += rhs;
    bool changed = false;
    if( orig != lhs )
    {
        changed = true;
        orig = lhs;
    }
    return changed;
#if 0
    Ast::NodeSet lhs = lkp[node];
    lhs += rhs;
    bool changed = false;
    if( lhs != lkp[node] )
    {
        changed = true;
        lkp[node] = lhs;
    }
    return changed;
#endif
}

bool FirstFollowSet::calculateFollowSet2(const Ast::Node* node, bool addRepetitions )
{
    // Dieser Algorithmus produziert ein identisches Follow-Set mit Coco/R wenn addRepetitions
    // habe vollständigen 1:1-Vergleich gemacht.

    if( node == 0 || node->doIgnore() )
        return false;

    // NT ---
    //      NT | T | SEQ | ALT
    //               SEQ ---
    //                     NT | T | SEQ | ALT

    bool changed = false;
    switch( node->d_type )
    {
    case Ast::Node::Nonterminal:
        if( node == node->d_owner->d_node )
        {
            // Spezialfall, wenn Definition nur ein NT enthält
            Ast::NodeSet follow = d_follow[node];
            // vereine das Follow Set dieses Nonterminals mit demjenigen dieser Production, in der sich das
            // Nonterminal gerade befindet
            changed |= add( d_follow, node, follow );
            // Wiederholung wird weiter unten behandelt
        }
        break;
    case Ast::Node::Alternative:
    case Ast::Node::Sequence:
        for( int i = 0; i < node->d_subs.size(); i++ )
        {
            // gehe durch alle Elemente der Sequence oder Alternative
            Ast::Node* a = node->d_subs[i];
            if( a->doIgnore() || !isNt(a))
                continue;
            // hier werden nur die Elemente von Seq und Alt betrachtet, die Nonterminals sind.
            // Vorsicht: da wir in einer EBNF sind, ist jede Sequence und Alternative selber
            // eine Production bzw. Nonterminal! Siehe https://stackoverflow.com/questions/2466484/converting-ebnf-to-bnf
            Ast::NodeSet follow;
            bool foundNn = false;
            if( node->d_type == Ast::Node::Sequence )
                for( int j = i + 1; j < node->d_subs.size(); j++ )
                {
                    // Berechne im Falle einer Sequence das Follow Set ab dem aktuellen NT.
                    const Ast::Node* b = node->d_subs[j];
                    if( b->doIgnore() )
                        continue;
                    const Ast::NodeSet first = getFirstNodeSet(b);
                    follow += first;
                    if( !b->isNullable() )
                    {
                        // Jedes nullable b + das erste non-nullable b gehören zum Follow Set
                        foundNn = true;
                        break;
                    }
                }
            if( !foundNn )
            {
                // wenn es sich um ein Element einer Alternative handelt oder in einer Sequence alles bis ans
                // Ende nullable war, wird das übergeordnete Follow Set runtergeholt
                const Ast::NodeSet outer = d_follow[node];
                follow += outer;
            }
            changed |= add( d_follow, a, follow );

            // go down
            changed |= calculateFollowSet2(a,addRepetitions);
        }
        break;
    default:
        break;
    }
    // das muss hier auf Ebene von node gemacht werden, da sonst Definition.d_node nicht berücksichtigt wird.
    if( isNt(node) && node->d_quant == Ast::Node::ZeroOrMore ) //  && addRepetitions )
    {
        // Wenn das Element wiederholt wird, erscheint auch sein eigenes First-Set als Teil des Follow-Sets
        changed |= add( d_follow, node, getFirstNodeSet(node) );
        // NOTE: wenn eine Alternative wiederholt, kann jedes Element potentiell vorkommen, es muss also das first
        // von jedem Sub ins follow der Alternative. Das wird von getFirstSet(Alternative) bereits berücksichtigt
    }
    return changed;
}
