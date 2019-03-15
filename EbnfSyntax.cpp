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
#include "EbnfErrors.h"
#include <QTextStream>
#include <QtDebug>

// Ursprünglich aus Ada::Syntax adaptiert; stark modifiziert

const char* EbnfSyntax::Node::s_typeName[] =
{
    "Terminal",
    "Nonterminal",
    "Sequence",
    "Alternative",
    "Predicate",
};

QString EbnfSyntax::pretty(const EbnfSyntax::NodeRefSet& s)
{
    QByteArrayList l;
    foreach( const NodeRef& r, s )
    {
        l.append( "'" + r.d_node->d_tok.d_val.toBa() + "'" );
    }
    qSort(l);
    return l.join(' ');
}

QString EbnfSyntax::pretty(const EbnfSyntax::NodeSet& s)
{
    QByteArrayList l;
    foreach( const Node* r, s )
    {
        l.append( "'" + r->d_tok.d_val.toBa() + "'" );
    }
    qSort(l);
    return l.join(' ');
}

EbnfSyntax::NodeRefSet EbnfSyntax::nodeToRefSet(const EbnfSyntax::NodeSet& s)
{
    NodeRefSet res;
    foreach( const Node* n, s )
        res.insert( n );
//    if( s.size() != res.size() )
//    {
//        qDebug() << "**** EbnfSyntax::nodeToRefSet size deviation";
//        qDebug() << "NodeSet:" << pretty(s);
//        qDebug() << "NodeRefSet:" << pretty(res);
//    }
    return res;
}

EbnfSyntax::NodeSet EbnfSyntax::collectNodes(const EbnfSyntax::NodeRefSet& pattern, const EbnfSyntax::NodeSet& set)
{
    EbnfSyntax::NodeSet res;
    foreach( const Node* n, set )
    {
        if( pattern.contains(n) )
            res << n;
    }
    return res;
}

EbnfSyntax::EbnfSyntax(EbnfErrors* errs):d_finished(false),d_errs(errs)
{

}

EbnfSyntax::~EbnfSyntax()
{
    clear();
}

void EbnfSyntax::dump() const
{
	qDebug() << "******** Begin Dump";
    foreach( Definition* d, d_order )
        d->dump();
	qDebug() << "******** End Dump";
}

void EbnfSyntax::clear()
{
	foreach( Definition* d, d_defs )
		delete d;
    d_defs.clear();
    d_order.clear();
    foreach( Definition* d, d_pragmas )
        delete d;
    d_pragmas.clear();
    d_finished = false;
}

static bool isTerminalOrSeqOfTerminals( const EbnfSyntax::Node* n )
{
    if( n == 0 )
        return false;
    if( n->d_type != EbnfSyntax::Node::Sequence && n->d_type != EbnfSyntax::Node::Terminal )
        return false;
    foreach( const EbnfSyntax::Node* sub, n->d_subs )
    {
        if( sub->d_type != EbnfSyntax::Node::Terminal )
            return false;
    }
    return true;
}

bool EbnfSyntax::addDef(EbnfSyntax::Definition* d)
{
    if( d_errs )
        d_errs->resetErrCount();
    d_finished = false;
    if( d_defs.contains( d->d_tok.d_val ) )
    {
        if( d_errs )
            d_errs->error( EbnfErrors::Semantics, d->d_tok.d_lineNr, d->d_tok.d_colNr,
                           QObject::tr("duplicate production '%1'").arg(d->d_tok.d_val.toStr()) );
        return false;
    }else
    {
        if( d->d_tok.d_val.toBa().startsWith('%') )
        {
            Definition*& def = d_pragmas[ d->d_tok.d_val ];
            if( def != 0 )
            {
                if( d_errs )
                    d_errs->warning( EbnfErrors::Semantics, d->d_tok.d_lineNr, d->d_tok.d_colNr,
                                   QObject::tr("overwriting pragma '%1'").arg(d->d_tok.d_val.toStr()) );
                delete def;
            }
            def = d;
        }else
        {
            d_defs.insert( d->d_tok.d_val, d );
            d_order.append( d );
        }
        return true;
    }
}

const EbnfSyntax::Definition*EbnfSyntax::getDef(const EbnfToken::Sym& name) const
{
    return d_defs.value(name);
}

QByteArrayList EbnfSyntax::getPragma(const QByteArray& name) const
{
    const Definition* d = d_pragmas.value( EbnfToken::getSym(name) );
    QByteArrayList res;
    if( d && d->d_node )
    {
        if( d->d_node->d_type == Node::Terminal )
            res << d->d_node->d_tok.d_val.toBa();
        else
        {
            foreach( const Node* sub, d->d_node->d_subs )
                res << sub->d_tok.d_val.toBa();
        }
    }
    return res;
}

QByteArray EbnfSyntax::getPragmaFirst(const QByteArray& name) const
{
    QByteArrayList str = getPragma(name);
    if( !str.isEmpty() )
        return str.first();
    else
        return QByteArray();
}

bool EbnfSyntax::finishSyntax()
{
    if( d_finished )
        return true;
    if( !resolveAllSymbols() )
        return false;
    calculateNullable();
    calcLeftRecursion();
    checkPragmas();
    d_finished = true;
    return true;
}

bool EbnfSyntax::resolveAllSymbols()
{
    if( d_errs )
        d_errs->resetErrCount();
    EbnfSyntax::OrderedDefs::const_iterator i;
    for( i = d_order.begin(); i != d_order.end(); ++i )
	{
        (*i)->d_usedBy.clear();
	}
    d_backRefs.clear();
    for( i = d_order.begin(); i != d_order.end(); ++i )
	{
        Node* node = (*i)->d_node;
        if( node )
        {
            //qDebug() << Node::s_typeName[(*i)->d_node->d_type] << (*i)->d_node->d_tok.toString(false);
            resolveAllSymbols( node );
        }
	}
//    for( BackRefs::const_iterator i = d_backRefs.begin(); i != d_backRefs.end(); ++i )
//        qDebug() << i.key().toStr() << (void*)i.key().data(); //  << pretty(i.value());
    if( d_errs )
        return d_errs->getErrCount() == 0;
    else
        return true;
}

void EbnfSyntax::calculateNullable()
{
    // Implement algorithm by a fixed­‐point iteration

    foreach( Definition* d, d_order )
    {
        d->d_nullable = false;
        d->d_repeatable = false;
    }

    bool changed;
    do
    {
        changed = false;
        foreach( Definition* d, d_order )
        {
            const bool nullable = ( d->d_node == 0 ? false : d->d_node->isNullable() );
            const bool repeatable = ( d->d_node == 0 ? false : d->d_node->isRepeatable() );
            if( nullable != d->d_nullable || repeatable != d->d_repeatable )
            {
                d->d_nullable = nullable;
                d->d_repeatable = repeatable;
                changed = true;
            }
        }
    }while( changed );

    // The computation will terminate because
    // - the variables are only changed monotonically (from false to true)
    // - the number of possible changes is finite (from all false to all true)

#if 0
    // Vergleich mit Coco/R gibt gleiches Resultat
    foreach( Definition* d, d_order )
    {
        if( !d->doIgnore() && d->isNullable() )
            qDebug() << d->d_tok.d_val << "is nullable";
    }
#endif
}

static inline bool isHit( const EbnfSyntax::Symbol* sym, quint32 line, quint16 col )
{
    return sym->d_tok.d_lineNr == line &&
            sym->d_tok.d_colNr <= col && col <= ( sym->d_tok.d_colNr + sym->d_tok.d_len );
}

const EbnfSyntax::Symbol*EbnfSyntax::findSymbolBySourcePos(quint32 line, quint16 col, bool nonTermOnly) const
{
    //qDebug() << "find" << line << col;
    for( int i = d_order.size() - 1; i >= 0; i-- )
    {
        if( d_order[i]->d_tok.d_lineNr <= line )
        {

            if( !nonTermOnly && isHit( d_order[i], line, col ) )
                return d_order[i];
            else
                return findSymbolBySourcePosImp( d_order[i]->d_node, line, col, nonTermOnly );
        }
    }
    return 0;
}

EbnfSyntax::ConstNodeList EbnfSyntax::getBackRefs(const Symbol* sym) const
{
    if( sym == 0 )
        return ConstNodeList();
    else
        return d_backRefs.value( sym->d_tok.d_val );
}

const EbnfSyntax::Node*EbnfSyntax::firstVisibleElementOf(const EbnfSyntax::Node* node)
{
    if( node == 0 || node->doIgnore() )
        return 0;

    switch( node->d_type )
    {
    case EbnfSyntax::Node::Nonterminal:
    case EbnfSyntax::Node::Terminal:
        return node;
    case EbnfSyntax::Node::Sequence:
    case EbnfSyntax::Node::Alternative:
        foreach( Node* n, node->d_subs )
        {
            if( n->doIgnore() )
                continue;
            return firstVisibleElementOf(n);
        }
        break;
    default:
        break;
    }
    return 0;
}

const EbnfSyntax::Node*EbnfSyntax::firstPredicateOf(const EbnfSyntax::Node* node)
{
    if( node == 0 )
        return 0;

    switch( node->d_type )
    {
    case EbnfSyntax::Node::Nonterminal:
    case EbnfSyntax::Node::Terminal:
        return 0;
    case EbnfSyntax::Node::Sequence:
    case EbnfSyntax::Node::Alternative:
        foreach( Node* n, node->d_subs )
        {
            if( n->d_type == EbnfSyntax::Node::Predicate )
                return n;
            const Node* res = firstPredicateOf(n);
            if( res )
                return res;
        }
        break;
    default:
        break;
    }
    return 0;
}

bool EbnfSyntax::resolveAllSymbols(EbnfSyntax::Node *node)
{
	Q_ASSERT( node->d_owner != 0 );
    switch( node->d_type )
    {
    case Node::Nonterminal:
        {
            Definitions::const_iterator i = d_defs.find( node->d_tok.d_val );
            if( i != d_defs.end() )
            {
                Definition* def = i.value();
                def->d_usedBy.insert(node);
                node->d_def = def;
                d_backRefs[node->d_tok.d_val].append(node);
            }else
            {
                if( d_errs )
                    d_errs->error( EbnfErrors::Semantics, node->d_tok.d_lineNr, node->d_tok.d_colNr,
                                   QObject::tr("'%1' references unknown '%2'").
                                        arg(node->d_owner->d_tok.d_val.toStr()).arg(node->d_tok.d_val.toStr()) );
            }
        }
        break;
    case Node::Terminal:
    case Node::Predicate:
        d_backRefs[node->d_tok.d_val].append(node);
        break;
    default:
        break;
    }
    foreach( Node* sub, node->d_subs )
    {
        resolveAllSymbols( sub );
    }
    if( d_errs )
        return d_errs->getErrCount() == 0;
    else
        return true;
}

const EbnfSyntax::Symbol*EbnfSyntax::findSymbolBySourcePosImp(const EbnfSyntax::Node* node, quint32 line, quint16 col, bool nonTermOnly) const
{
    if( node == 0 )
        return 0;
    if( node->d_tok.d_lineNr > line )
        return 0;
    //qDebug() << "Node" << node->d_tok.toString() << node->d_tok.d_lineNr << node->d_tok.d_colNr << node->d_tok.d_len;
    if( node->d_type == Node::Terminal && !nonTermOnly && isHit( node, line, col ) )
        return node;
    else if( node->d_type == Node::Nonterminal && isHit( node, line, col ) )
        return node;
    // else
    foreach( const Node* n, node->d_subs )
    {
        // TODO: binary search falls nötig
        const EbnfSyntax::Symbol* res = findSymbolBySourcePosImp( n, line, col, nonTermOnly );
        if( res )
            return res;
    }
    return 0;
}

void EbnfSyntax::calcLeftRecursion()
{
    foreach( Definition* d, d_order )
    {
        d->d_directLeftRecursive = false;
        d->d_indirectLeftRecursive = false;
        if( d->doIgnore() || d->d_node == 0 )
            continue;
        NodeList path;
        markLeftRecursion(d,d->d_node,path);
    }
    /*
    QList< QPair<Node,NodeSet> > startsWith;
    foreach( Definition* d, d_order )
    {
        d->d_directLeftRecursive = false;
        if( d->doIgnore() || d->d_node == 0 )
            continue;
        startsWith << qMakePair( Node(Node::Nonterminal,d,d->d_tok), calcStartsWithNtSet(d->d_node) );
        NodeSet::iterator i = startsWith.last().second.find(&startsWith.last().first);
        if( i != startsWith.last().second.end() )
        {
            const_cast<Node*>((*i).d_node)->d_leftRecursive = true;
            startsWith.last().first.d_owner->d_directLeftRecursive = true;
            qDebug() << "direct left recursion" << startsWith.last().first.d_owner->d_tok.d_val;
            startsWith.last().second.erase(i);
        }
    }
    */

    /*
    bool changed;
    do
    {
        changed = false;
        foreach( Definition* d, d_order )
        {
            const bool nullable = ( d->d_node == 0 ? false : d->d_node->isNullable() );
            const bool repeatable = ( d->d_node == 0 ? false : d->d_node->isRepeatable() );
            if( nullable != d->d_nullable || repeatable != d->d_repeatable )
            {
                d->d_nullable = nullable;
                d->d_repeatable = repeatable;
                changed = true;
            }
        }
    }while( changed );
    */

}

void EbnfSyntax::markLeftRecursion(EbnfSyntax::Definition* start, Node* cur, EbnfSyntax::NodeList& path)
{
    if( cur == 0 || cur->doIgnore() )
        return;

    if( cur->d_owner == start )
    {
        cur->d_pathToDef.clear();
        cur->d_leftRecursive = false;
    }
    switch( cur->d_type )
    {
    case EbnfSyntax::Node::Alternative:
        foreach( EbnfSyntax::Node* sub, cur->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            markLeftRecursion(start,sub,path);
        }
        break;
    case EbnfSyntax::Node::Sequence:
        foreach( EbnfSyntax::Node* sub, cur->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            // gehe bis zum ersten das nicht optional ist; diese alle sind potentielle Starts
            markLeftRecursion(start,sub,path);
            if( !sub->isNullable() )
                break;
        }
        break;
    case EbnfSyntax::Node::Nonterminal:
        if( !cur->doIgnore() && cur->d_def != 0 && cur->d_def->d_node != 0 )
        {
            if( cur->d_def == start )
            {
                if( cur->d_owner == start )
                    start->d_directLeftRecursive = true;
                else
                    start->d_indirectLeftRecursive = true;
                cur->d_leftRecursive = true;
                cur->d_pathToDef = path;
                if( d_errs )
                {
                    ConstNodeList l;
                    foreach( EbnfSyntax::Node* n, path )
                        l.append(n);
                    d_errs->error( EbnfErrors::Semantics, cur->d_tok.d_lineNr, cur->d_tok.d_colNr,
                                   QString("%1 left recursion with '%2'").
                                   arg(start->d_directLeftRecursive?"direct":"indirect").arg(start->d_tok.d_val.toStr()),
                                   QVariant::fromValue(EbnfSyntax::IssueData(
                                                           EbnfSyntax::IssueData::LeftRec,start->d_node, cur,l)));
                }
            }else
            {
                bool recursion = false;
                foreach( Node* n, path )
                {
                    if( n == cur || n->d_def == cur->d_def )
                    {
                        recursion = true;
                        break;
                    }
                }
                if( !recursion )
                {
                    path.push_back(cur);
                    markLeftRecursion(start,cur->d_def->d_node,path);
                    path.pop_back();
                }
            }
        }
        break;
    case EbnfSyntax::Node::Terminal:
    case EbnfSyntax::Node::Predicate:
        // ignore
        break;
    }
}

void EbnfSyntax::checkPragmas()
{
    foreach( const Definition* d, d_pragmas )
    {
        if( !isTerminalOrSeqOfTerminals( d->d_node ) )
        {
            d_errs->error( EbnfErrors::Semantics, d->d_tok.d_lineNr, d->d_tok.d_colNr,
                           QObject::tr("right hand side of pragma '%1' must be a sequence of terminals").arg(d->d_tok.d_val.toStr()) );
        }
    }
}

EbnfSyntax::NodeRefSet EbnfSyntax::calcStartsWithNtSet(EbnfSyntax::Node* node)
{
    if( node == 0 || node->doIgnore() )
        return NodeRefSet();

    node->d_leftRecursive = false;
    NodeRefSet res;
    switch( node->d_type )
    {
    case EbnfSyntax::Node::Alternative:
        foreach( EbnfSyntax::Node* sub, node->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            res += calcStartsWithNtSet( sub );
        }
        break;
    case EbnfSyntax::Node::Sequence:
        foreach( EbnfSyntax::Node* sub, node->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            // gehe bis zum ersten das nicht optional ist; diese alle sind potentielle Starts
            res += calcStartsWithNtSet( sub );
            if( !sub->isNullable() )
                break;
        }
        break;
    case EbnfSyntax::Node::Nonterminal:
        if( !node->doIgnore() )
        {
            if( node->d_def != 0 && node->d_def->d_node != 0 )
                res << node;
        }
        break;
    case EbnfSyntax::Node::Terminal:
    case EbnfSyntax::Node::Predicate:
        // ignore
        break;
    }
    return res;
}

bool EbnfSyntax::Definition::doIgnore() const
{
    return d_tok.d_op == EbnfToken::Skip;
}

void EbnfSyntax::Definition::dump() const
{
    qDebug() << d_tok.d_val.toStr() << "::=";
	if( d_node )
		d_node->dump(1);
	else
		qDebug() << "    No nodes";
}

EbnfSyntax::Node::~Node()
{
	foreach( Node* n, d_subs )
        delete n;
}

bool EbnfSyntax::Node::doIgnore() const
{
    return d_type == Predicate || d_tok.d_op == EbnfToken::Skip ||
            ( d_def != 0 && d_def->doIgnore() && d_tok.d_op != EbnfToken::Keep );
}

bool EbnfSyntax::Node::isNullable() const
{
    if( d_quant != One )
        return true;
    switch( d_type )
    {
    case Nonterminal:
        if( d_def )
            return d_def->isNullable();
        else
            return false;
    case Sequence:
        foreach( Node* n, d_subs )
        {
            if( n->doIgnore() )
                continue;
            if( !n->isNullable() )
                return false;
        }
        return true;
    case Alternative:
        foreach( Node* n, d_subs )
        {
            if( n->doIgnore() )
                continue;
            if( n->isNullable() )
                return true;
        }
        return false;
    default:
        break;
    }
    return false;
}

bool EbnfSyntax::Node::isRepeatable() const
{
    if( d_quant == ZeroOrMore )
        return true;

    Node* n = 0;
    switch( d_type )
    {
    case Nonterminal:
        if( d_def )
            return d_def->isRepeatable();
        else
            return false;
    case Sequence:
    case Alternative:
        foreach( Node* sub, d_subs )
        {
            if( sub->doIgnore() )
                continue;
            if( n != 0 )
                return false;
            n = sub;
        }
        if( n )
            return n->isRepeatable();
        else
            return false;
    default:
        break;
    }
    return false;
}

const EbnfSyntax::Node*EbnfSyntax::Node::getNext(int* index) const
{
    Node* parent = d_parent;
    const Node* me = this;
    while( parent )
    {
        if( parent->d_type == Sequence ) // bei  Alternative gehe direkt eine Stufe nach oben
        {
            const int pos = parent->d_subs.indexOf(const_cast<Node*>(me));
            Q_ASSERT( pos != -1 );
            if( pos + 1 < parent->d_subs.size() )
            {
                if( index )
                    *index += 1;
                return parent->d_subs[pos+1];
            }
        }
        me = parent;
        parent = parent->d_parent;
    }
    return 0;
}

int EbnfSyntax::Node::getLlk() const
{
    const QByteArray val = d_tok.d_val;
    if( d_type != Predicate || !val.startsWith("LL:") )
        return 0;
    const int ll = val.mid(3).toInt();
    if( ll <= 1 )
        return 0;
    else
        return ll;
}

void EbnfSyntax::Node::dump(int level) const
{
	QString str;
	switch( d_type )
	{
    case Terminal:
        str = QString("terminal '%1'").arg( d_tok.d_val.toStr() );
		break;
    case Nonterminal:
        str = QString("nonterminal '%1'").arg( d_tok.d_val.toStr() );
		break;
	case Alternative:
        str = "alternative";
		break;
	case Sequence:
        str = "sequence";
		break;
    case Predicate:
        str = QString("predicate %1").arg( d_tok.d_val.toStr() );
        break;
	}
	switch( d_quant )
	{
	case One:
		break;
	case ZeroOrOne:
		str = QString("[%1]").arg(str);
		break;
	case ZeroOrMore:
		str = QString("{%1}").arg(str);
		break;
	}

    qDebug() << QString( level * 4, QChar(' ') ).toUtf8().constData() << str.toUtf8().constData();
	foreach( Node* n, d_subs )
        n->dump(level+1);
}

QString EbnfSyntax::Node::toString() const
{
    switch( d_type )
    {
    case Terminal:
    case Nonterminal:
    case Predicate:
        return d_tok.d_val.toStr();
    case Sequence:
        return QLatin1String("<seq>");
    case Alternative:
        return QLatin1String("<alt>");
    }
    return QString();
}



bool EbnfSyntax::NodeRef::operator==(const EbnfSyntax::NodeRef& rhs) const
{
    if( d_node == 0 || rhs.d_node == 0 )
        return false;
    if( d_node == rhs.d_node || d_node->d_tok.d_val == rhs.d_node->d_tok.d_val )
        return true;
    else
        return false; // wegen Lexer::d_symTbl nicht nötig: d_node->d_tok.d_val == rhs.d_node->d_tok.d_val;
}
