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
#include "LaParser.h"
#include <QTextStream>
#include <QtDebug>

// Ursprünglich aus Ada::Syntax adaptiert; stark modifiziert

const char* Ast::Node::s_typeName[] =
{
    "Terminal",
    "Nonterminal",
    "Sequence",
    "Alternative",
    "Predicate",
};

static bool error( EbnfErrors* errs, EbnfErrors::Source s,const EbnfToken& tok, const QString& msg )
{
    if( errs == 0 )
        return true;
    errs->error( s, tok.d_lineNr, tok.d_colNr, msg );
    return false;
}

QString EbnfSyntax::pretty(const Ast::NodeRefSet& s)
{
    QStringList l;
    foreach( const Ast::NodeRef& r, s )
    {
        l.append( "'" + r.d_node->d_tok.d_val.toStr() + "'" );
    }
    qSort(l);
    return l.join(' ');
}

QString EbnfSyntax::pretty(const Ast::NodeSet& s)
{
    QStringList l;
    foreach( const Ast::Node* r, s )
    {
        l.append( "'" + r->d_tok.d_val.toStr() + "'" );
    }
    qSort(l);
    return l.join(' ');
}

Ast::NodeRefSet EbnfSyntax::nodeToRefSet(const Ast::NodeSet& s)
{
    Ast::NodeRefSet res;
    foreach( const Ast::Node* n, s )
        res.insert( Ast::NodeRef(n) );
//    if( s.size() != res.size() )
//    {
//        qDebug() << "**** EbnfSyntax::nodeToRefSet size deviation";
//        qDebug() << "NodeSet:" << pretty(s);
//        qDebug() << "NodeRefSet:" << pretty(res);
//    }
    return res;
}

Ast::NodeSet EbnfSyntax::collectNodes(const Ast::NodeRefSet& pattern, const Ast::NodeSet& set)
{
    Ast::NodeSet res;
    foreach( const Ast::Node* n, set )
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
    foreach( Ast::Definition* d, d_order )
        d->dump();
	qDebug() << "******** End Dump";
}

void EbnfSyntax::clear()
{
    foreach( Ast::Definition* d, d_defs )
		delete d;
    d_defs.clear();
    d_order.clear();
    foreach( Ast::Definition* d, d_pragmas )
        delete d;
    d_pragmas.clear();
    d_finished = false;
    d_idol.clear();
}

static bool isTerminalOrSeqOfTerminals( const Ast::Node* n )
{
    if( n == 0 )
        return false;
    if( n->d_type != Ast::Node::Sequence && n->d_type != Ast::Node::Terminal &&
            n->d_type != Ast::Node::Nonterminal )
        return false;
    foreach( const Ast::Node* sub, n->d_subs )
    {
        if( sub->d_type != Ast::Node::Terminal && sub->d_type != Ast::Node::Nonterminal )
            return false;
    }
    return true;
}

bool EbnfSyntax::addDef(Ast::Definition* d)
{
    if( d_errs )
        d_errs->resetErrCount();
    d_finished = false;
    if( d_defs.contains( d->d_tok.d_val ) )
    {
        error( d_errs, EbnfErrors::Semantics, d->d_tok,
                           QObject::tr("duplicate production '%1'").arg(d->d_tok.d_val.toStr()) );
        return false;
    }else
    {
        if( d->d_tok.d_val.toBa().startsWith('%') )
        {
            Ast::Definition*& def = d_pragmas[ d->d_tok.d_val ];
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

bool EbnfSyntax::addPragma(const EbnfToken& name, Ast::Node* ex)
{
    if( ex == 0 )
        return false;
    if( !name.d_val.toBa().startsWith('%') )
    {
        if( d_errs )
            d_errs->warning( EbnfErrors::Semantics, name.d_lineNr, name.d_colNr,
                           QObject::tr("invalid pragma '%1'").arg(name.d_val.toStr()) );
        delete ex;
        return false;
    }
    Ast::Definition*& def = d_pragmas[ name.d_val ];
    if( def == 0 )
        def = new Ast::Definition(name);

    if( def->d_node == 0 )
    {
        def->d_node = ex;
        return true;
    }
    if( def->d_node->d_type != Ast::Node::Sequence )
    {
        Ast::Node* n = def->d_node;
        def->d_node = new Ast::Node( Ast::Node::Sequence, def );
        def->d_node->d_subs.append(n);
        n->d_parent = def->d_node;
    }
    if( ex->d_type == Ast::Node::Sequence )
    {
        def->d_node->d_subs += ex->d_subs;
        foreach( Ast::Node* n, ex->d_subs )
        {
            n->d_parent = def->d_node;
            n->d_owner = def;
        }
        ex->d_subs.clear();
        delete ex;
    }else
    {
        def->d_node->d_subs.append(ex);
        ex->d_parent = def->d_node;
        ex->d_owner = def;
    }
    return true;
}

const Ast::Definition*EbnfSyntax::getDef(const EbnfToken::Sym& name) const
{
    return d_defs.value(name);
}

EbnfSyntax::SymList EbnfSyntax::getPragma(const QByteArray& name) const
{
    const Ast::Definition* d = d_pragmas.value( EbnfToken::getSym(name) );
    SymList res;
    if( d && d->d_node )
    {
        if( d->d_node->d_type == Ast::Node::Terminal )
            res << d->d_node->d_tok.d_val;
        else
        {
            foreach( const Ast::Node* sub, d->d_node->d_subs )
                res << sub->d_tok.d_val;
        }
    }
    return res;
}

EbnfToken::Sym EbnfSyntax::getPragmaFirst(const QByteArray& name) const
{
    SymList str = getPragma(name);
    if( !str.isEmpty() )
        return str.first();
    else
        return EbnfToken::Sym();
}

void EbnfSyntax::addIdol(qint32 line)
{
    d_idol.append(line);
}

bool EbnfSyntax::finishSyntax()
{
    if( d_finished )
        return true;
    if( !resolveAllSymbols() )
        return false;
    checkReachability();
    calculateNullable();
    calcLeftRecursion();
    checkPragmas();
    checkPredicates();
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
        Ast::Definition* d = (*i);
        if( d->d_node && !d->doIgnore() )
        {
            //qDebug() << Node::s_typeName[(*i)->d_node->d_type] << (*i)->d_node->d_tok.toString(false);
            resolveAllSymbols( d->d_node );
        }
	}
    for( int j = 1; j < d_order.size(); j++ ) // ignore first
    {
        Ast::Definition* d = d_order[j];
        if( !d->doIgnore() && d->d_usedBy.isEmpty() && d->d_node != 0 )
        {
            if( d_errs )
                d_errs->warning( EbnfErrors::Semantics, d->d_tok.d_lineNr, d->d_tok.d_colNr,
                                 QObject::tr("unused production '%1'").arg(d->d_tok.d_val.c_str() ));
        }
    }

    if( d_errs )
        return d_errs->getErrCount() == 0;
    else
        return true;
}

void EbnfSyntax::calculateNullable()
{
    // Implement algorithm by a fixed­‐point iteration

    foreach( Ast::Definition* d, d_order )
    {
        d->d_nullable = false;
        d->d_repeatable = false;
    }

    bool changed;
    do
    {
        changed = false;
        foreach( Ast::Definition* d, d_order )
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

void EbnfSyntax::checkReachability()
{
    bool changed;
    do
    {
        changed = false;
        foreach( Ast::Definition* d, d_order )
        {
            if( d->d_usedBy.isEmpty() || d->doIgnore() )
                continue; // unused already checked

            bool atLeastOneIsReachable = false;
            foreach( const Ast::Node* n, d->d_usedBy )
            {
                if( !n->doIgnore() && !n->d_owner->doIgnore() )
                {
                    atLeastOneIsReachable = true;
                    break;
                }
            }
            const bool notReachable = !atLeastOneIsReachable;
            // qDebug() << d->d_tok.d_val.c_str() << atLeastOneIsReachable << d->doIgnore() << d->d_notReachable;
            if( notReachable != d->d_notReachable )
            {
                if( d_errs )
                    d_errs->warning( EbnfErrors::Semantics, d->d_tok.d_lineNr, d->d_tok.d_colNr,
                                     QObject::tr("production not reachable '%1'").arg(d->d_tok.d_val.c_str() ));
                changed = true;
                d->d_notReachable = notReachable;
            }
        }
    }while( changed );
    foreach( Ast::Definition* d, d_order )
    {
        if( d->doIgnore() )
            continue;
        if( d_errs != 0 && d->d_node != 0 && !d->d_node->isAnyReachable() )
            d_errs->warning( EbnfErrors::Semantics, d->d_tok.d_lineNr, d->d_tok.d_colNr,
                             QObject::tr("production has not reachable content '%1'").arg(d->d_tok.d_val.c_str() ));
    }

}

static inline bool isHit( const Ast::Symbol* sym, quint32 line, quint16 col )
{
    return sym->d_tok.d_lineNr == line &&
            sym->d_tok.d_colNr <= col && col <= ( sym->d_tok.d_colNr + sym->d_tok.d_len );
}

const Ast::Symbol*EbnfSyntax::findSymbolBySourcePos(quint32 line, quint16 col, bool nonTermOnly) const
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

Ast::ConstNodeList EbnfSyntax::getBackRefs(const Ast::Symbol* sym) const
{
    if( sym == 0 )
        return Ast::ConstNodeList();
    else
        return d_backRefs.value( sym->d_tok.d_val );
}

const Ast::Node*EbnfSyntax::firstVisibleElementOf(const Ast::Node* node)
{
    if( node == 0 || node->doIgnore() )
        return 0;

    switch( node->d_type )
    {
    case Ast::Node::Nonterminal:
    case Ast::Node::Terminal:
        return node;
    case Ast::Node::Sequence:
    case Ast::Node::Alternative:
        foreach( Ast::Node* n, node->d_subs )
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

const Ast::Node*EbnfSyntax::firstPredicateOf(const Ast::Node* node)
{
    if( node == 0 )
        return 0;

    switch( node->d_type )
    {
    case Ast::Node::Nonterminal:
    case Ast::Node::Terminal:
        return 0;
    case Ast::Node::Sequence:
    case Ast::Node::Alternative:
        foreach( Ast::Node* n, node->d_subs )
        {
            if( n->d_type == Ast::Node::Predicate )
                return n;
            const Ast::Node* res = firstPredicateOf(n);
            if( res )
                return res;
        }
        break;
    default:
        break;
    }
    return 0;
}

bool EbnfSyntax::resolveAllSymbols(Ast::Node *node)
{
	Q_ASSERT( node->d_owner != 0 );
    switch( node->d_type )
    {
    case Ast::Node::Nonterminal:
        {
            Definitions::const_iterator i = d_defs.find( node->d_tok.d_val );
            if( i != d_defs.end() )
            {
                Ast::Definition* def = i.value();
                def->d_usedBy.insert(node);
                node->d_def = def;
                d_backRefs[node->d_tok.d_val].append(node);
                if( def->doIgnore() )
                    error( d_errs, EbnfErrors::Semantics, node->d_tok,
                                   QObject::tr("'%1' uses skipped '%2'").
                                        arg(node->d_owner->d_tok.d_val.toStr()).arg(def->d_tok.d_val.toStr()) );
            }else
            {
                error( d_errs, EbnfErrors::Semantics, node->d_tok,
                                   QObject::tr("'%1' references unknown '%2'").
                                        arg(node->d_owner->d_tok.d_val.toStr()).arg(node->d_tok.d_val.toStr()) );
            }
        }
        break;
    case Ast::Node::Terminal:
    case Ast::Node::Predicate:
        d_backRefs[node->d_tok.d_val].append(node);
        break;
    default:
        break;
    }
    foreach( Ast::Node* sub, node->d_subs )
    {
        resolveAllSymbols( sub );
    }
    if( d_errs )
        return d_errs->getErrCount() == 0;
    else
        return true;
}

const Ast::Symbol*EbnfSyntax::findSymbolBySourcePosImp(const Ast::Node* node, quint32 line, quint16 col, bool nonTermOnly) const
{
    if( node == 0 )
        return 0;
    if( node->d_tok.d_lineNr > line )
        return 0;
    //qDebug() << "Node" << node->d_tok.toString() << node->d_tok.d_lineNr << node->d_tok.d_colNr << node->d_tok.d_len;
    if( node->d_type == Ast::Node::Terminal && !nonTermOnly && isHit( node, line, col ) )
        return node;
    else if( node->d_type == Ast::Node::Nonterminal && isHit( node, line, col ) )
        return node;
    // else
    foreach( const Ast::Node* n, node->d_subs )
    {
        // TODO: binary search falls nötig
        const Ast::Symbol* res = findSymbolBySourcePosImp( n, line, col, nonTermOnly );
        if( res )
            return res;
    }
    return 0;
}

void EbnfSyntax::calcLeftRecursion()
{
    foreach( Ast::Definition* d, d_order )
    {
        d->d_directLeftRecursive = false;
        d->d_indirectLeftRecursive = false;
        if( d->doIgnore() || d->d_node == 0 )
            continue;
        Ast::NodeList path;
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

void EbnfSyntax::markLeftRecursion(Ast::Definition* start, Ast::Node* cur, Ast::NodeList& path)
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
    case Ast::Node::Alternative:
        foreach( Ast::Node* sub, cur->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            markLeftRecursion(start,sub,path);
        }
        break;
    case Ast::Node::Sequence:
        foreach( Ast::Node* sub, cur->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            // gehe bis zum ersten das nicht optional ist; diese alle sind potentielle Starts
            markLeftRecursion(start,sub,path);
            if( !sub->isNullable() )
                break;
        }
        break;
    case Ast::Node::Nonterminal:
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
                    Ast::ConstNodeList l;
                    foreach( Ast::Node* n, path )
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
                foreach( Ast::Node* n, path )
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
    case Ast::Node::Terminal:
    case Ast::Node::Predicate:
        // ignore
        break;
    }
}

void EbnfSyntax::checkPragmas()
{
    if( d_errs == 0 )
        return;
    foreach( const Ast::Definition* d, d_pragmas )
    {
        if( !isTerminalOrSeqOfTerminals( d->d_node ) )
        {
            d_errs->error( EbnfErrors::Semantics, d->d_tok.d_lineNr, d->d_tok.d_colNr,
                           QObject::tr("right hand side of pragma '%1' must be a sequence of (non)terminals").arg(d->d_tok.d_val.toStr()) );
        }
    }
}

Ast::NodeRefSet EbnfSyntax::calcStartsWithNtSet(Ast::Node* node)
{
    if( node == 0 || node->doIgnore() )
        return Ast::NodeRefSet();

    node->d_leftRecursive = false;
    Ast::NodeRefSet res;
    switch( node->d_type )
    {
    case Ast::Node::Alternative:
        foreach( Ast::Node* sub, node->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            res += calcStartsWithNtSet( sub );
        }
        break;
    case Ast::Node::Sequence:
        foreach( Ast::Node* sub, node->d_subs )
        {
            if( sub->doIgnore() )
                continue;
            // gehe bis zum ersten das nicht optional ist; diese alle sind potentielle Starts
            res += calcStartsWithNtSet( sub );
            if( !sub->isNullable() )
                break;
        }
        break;
    case Ast::Node::Nonterminal:
        if( !node->doIgnore() )
        {
            if( node->d_def != 0 && node->d_def->d_node != 0 )
                res << node;
        }
        break;
    case Ast::Node::Terminal:
    case Ast::Node::Predicate:
        // ignore
        break;
    }
    return res;
}

void EbnfSyntax::checkPredicates()
{
    if( d_errs == 0 )
        return;
    foreach( const Ast::Definition* d, d_defs )
    {
        if( !d->doIgnore() && d->d_node != 0 )
            checkPredicates(d->d_node);
    }
}

static bool checkLaAst( EbnfSyntax* syn, LaParser::Ast* ast, const EbnfToken& tok )
{
    if( ast->d_type == LaParser::Ast::Ident )
    {
        const EbnfToken::Sym sym = EbnfToken::getSym(ast->d_val);
        if( syn->getKeywords().contains(sym) )
            return true;
        const Ast::Definition* d = syn->getDef(sym);
        if( d == 0 )
            return error( syn->getErrs(), EbnfErrors::Semantics, tok,
                          QString("unknown terminal '%1'").arg(ast->d_val.constData()) );
        if( d->d_node != 0 )
            return error( syn->getErrs(), EbnfErrors::Semantics, tok,
                          QString("symbol '%1' is not a terminal").arg(ast->d_val.constData()) );
        if( d->doIgnore() )
            return error( syn->getErrs(), EbnfErrors::Semantics, tok,
                          QString("referencing skipped terminal '%1'").arg(ast->d_val.constData()) );
    }else
    {
        foreach( LaParser::AstRef sub, ast->d_subs )
            checkLaAst( syn, sub.data(), tok );
    }
    return true;
}

void EbnfSyntax::checkPredicates(Ast::Node* node)
{
    Q_ASSERT( node != 0 );
    if( node->d_type == Ast::Node::Predicate )
    {
        const QByteArray val = node->d_tok.d_val.toBa().trimmed();
        if( val.startsWith("LL:") )
        {
            bool ok;
            const quint32 p = val.mid(3).trimmed().toUInt(&ok);
            if( !ok || p < 1 )
                error( d_errs, EbnfErrors::Semantics, node->d_tok, "invalid LL predicate" );
        }else if( val.startsWith("LL(") )
        {
            if( val.endsWith(')') )
            {
                bool ok;
                const quint32 p = val.mid(3, val.size() - 3 - 1).trimmed().toUInt(&ok);
                if( ok && p >= 1)
                    return;
            }
            error( d_errs, EbnfErrors::Semantics, node->d_tok, "invalid LL predicate" );
        }else if( val.startsWith("LA:") )
        {
            LaParser p;
            bool res = p.parse(val.mid(3));
//            if( p.getLaExpr().constData() )
//                p.getLaExpr()->dump();
            if( !res )
                error( d_errs, EbnfErrors::Syntax, node->d_tok, QString("invalid LA predicate: %1").arg(p.getErr()) );
            else
            {
                Q_ASSERT( p.getLaExpr().constData() != 0 );
                checkLaAst( this, p.getLaExpr().data(), node->d_tok );
            }
        }else
            error( d_errs, EbnfErrors::Syntax, node->d_tok, "unknown predicate" );
    }else
        foreach( Ast::Node* sub, node->d_subs )
            checkPredicates(sub);
}

bool Ast::Definition::doIgnore() const
{
    return d_tok.d_op == EbnfToken::Skip || d_notReachable;
}

void Ast::Definition::dump() const
{
    qDebug() << d_tok.d_val.toStr() << "::=";
	if( d_node )
		d_node->dump(1);
	else
		qDebug() << "    No nodes";
}

Ast::Node::~Node()
{
	foreach( Node* n, d_subs )
        delete n;
}

bool Ast::Node::doIgnore() const
{
    return d_type == Predicate || d_tok.d_op == EbnfToken::Skip ||
            ( d_def != 0 && d_def->doIgnore() && d_tok.d_op != EbnfToken::Keep );
}

bool Ast::Node::isNullable() const
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

bool Ast::Node::isRepeatable() const
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

bool Ast::Node::isAnyReachable() const
{
    if( doIgnore() )
        return false;
    switch( d_type )
    {
    case Nonterminal:
        return !d_def->doIgnore();
    case Terminal:
        return true;
    case Sequence:
        foreach( Node* n, d_subs )
        {
            if( n->isAnyReachable() )
                return true;
        }
        break;
    case Alternative:
        foreach( Node* n, d_subs )
        {
            if( !n->isAnyReachable() )
                return false;
        }
        return true;
    default:
        break;
    }
    return false;
}

const Ast::Node*Ast::Node::getNext(int* index) const
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

int Ast::Node::getLlk() const
{
    const QByteArray val = d_tok.d_val;
    if( d_type != Predicate || !val.startsWith("LL:") )
        return 0;
    return val.mid(3).toUInt(); // validity was already checked in EbnfParser
}

int Ast::Node::getLl() const
{
    QByteArray val = d_tok.d_val;
    if( d_type != Predicate || !val.startsWith("LL(") )
        return 0;
    const int pos = val.indexOf(')',3);
    if( pos == -1 )
        return 0;
    val = val.mid(3,pos-3);
    return val.toUInt();
}

QByteArray Ast::Node::getLa() const
{
    const QByteArray val = d_tok.d_val;
    if( val.startsWith("LA:") )
        return val.mid(3);
    else
        return QByteArray();
}

void Ast::Node::dump(int level) const
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

QString Ast::Node::toString() const
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

bool Ast::NodeRef::operator==(const Ast::NodeRef& rhs) const
{
    if( d_node == 0 || rhs.d_node == 0 )
        return false;
    if( d_node == rhs.d_node || d_node->d_tok.d_val == rhs.d_node->d_tok.d_val )
        return true;
    else
        return false; // wegen Lexer::d_symTbl nicht nötig: d_node->d_tok.d_val == rhs.d_node->d_tok.d_val;
}


Ast::Definition::~Definition()
{
    if( d_node ) delete d_node;
}
