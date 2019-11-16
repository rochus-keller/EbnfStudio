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

#include "EbnfParser.h"
#include "EbnfLexer.h"
#include "EbnfErrors.h"
#include "LaParser.h"

EbnfParser::EbnfParser(QObject *parent) : QObject(parent),d_lex(0),d_def(0),d_errs(0)
{

}

bool EbnfParser::parse(EbnfLexer* lex)
{
    Q_ASSERT( lex != 0 );
    d_lex = lex;
    if( d_errs )
        d_errs->resetErrCount();

    d_syn = new EbnfSyntax(d_errs);
    d_def = 0;

    EbnfToken t = nextToken();
    while( t.isValid() )
    {
        if( t.d_type == EbnfToken::Production )
        {
            EbnfToken op = nextToken();
            if( op.d_type == EbnfToken::Assig )
            {
                d_def = new Ast::Definition(t);
                if( !d_syn->addDef( d_def ) )
                {
                    delete d_def;
                    return false;
                }
                t = nextToken();
                if( t.d_type != EbnfToken::Production && t.d_type != EbnfToken::Eof )
                    d_def->d_node = parseExpression();
            }else if( op.d_type == EbnfToken::AddTo )
            {
                if( !t.d_val.toBa().startsWith('%') )
                    return error( op, tr("only pragmas support '+='") );
                d_def = 0;
                nextToken();
                if( d_cur.d_type != EbnfToken::Production && d_cur.d_type != EbnfToken::Eof )
                {
                    d_syn->addPragma( t, parseExpression() );
                    if( t.d_val.toBa() == "%keywords" )
                        d_lex->addKeywords( d_syn->getPragma( t.d_val ).toSet() );
                }
            }else
               return error( t, tr("expecing '::=' or '+='") );
        }else
            return error( t );
        t = d_cur;
    }

    d_syn->setDefines( d_defines );
    d_syn->setKeywords(d_lex->getKeywords());

    if( t.isErr() )
        error( t );

    if( d_errs )
        return d_errs->getErrCount() == 0;
    else
        return true;
}

EbnfSyntax* EbnfParser::getSyntax()
{
    return d_syn.data();
}

EbnfToken EbnfParser::nextToken()
{
    Q_ASSERT( d_lex != 0 );
    EbnfToken t = d_lex->nextToken();
    d_cur = t;
    while( t.isValid() &&
           ( t.d_type == EbnfToken::Comment || EbnfToken::isPpType(t.d_type) || !txOn() ) )
    {
        if( EbnfToken::isPpType(t.d_type) )
            handlePpSym(t);
        t = d_lex->nextToken();
        d_cur = t;
    }
    return t;
}

bool EbnfParser::error(const EbnfToken& t, const QString& msg)
{
    if( d_errs == 0 )
        return false;
    if( t.d_type == EbnfToken::Invalid )
        d_errs->error( EbnfErrors::Syntax, t.d_lineNr, t.d_colNr, t.d_val.toStr() );
    else if( msg.isEmpty() )
        d_errs->error( EbnfErrors::Syntax, t.d_lineNr, t.d_colNr,
                        QString("unexpected symbol '%1'").arg(t.toString()) );
    else
        d_errs->error( EbnfErrors::Syntax, t.d_lineNr, t.d_colNr, msg );
    return false;
}

Ast::Node*EbnfParser::parseFactor()
{
    // factor ::= keyword | delimiter | category | "[" expression "]" | "{" expression "}"

    Ast::Node* node = 0;

    switch( d_cur.d_type )
    {
    case EbnfToken::Keyword:
    case EbnfToken::Literal:
        node = new Ast::Node( Ast::Node::Terminal, d_def, d_cur );
        nextToken();
        break;
    case EbnfToken::NonTerm:
        node = new Ast::Node( Ast::Node::Nonterminal, d_def, d_cur );
        nextToken();
        break;
    case EbnfToken::LBrack:
        {
            nextToken();
            node = parseExpression();
            if( node == 0 )
            {
                return 0;
            }
            if( d_cur.d_type != EbnfToken::RBrack )
            {
                error( d_cur, "expecting ']'" );
                delete node;
                return 0;
            }
            if( !checkCardinality(node) )
                return 0;
            node->d_quant = Ast::Node::ZeroOrOne;
            nextToken();
        }
        break;
    case EbnfToken::LPar:
        {
            nextToken();
            node = parseExpression();
            if( node == 0 )
            {
                return 0;
            }
            if( d_cur.d_type != EbnfToken::RPar )
            {
                error( d_cur, "expecting ')'" );
                delete node;
                return 0;
            }
            if( !checkCardinality(node) )
                return 0;
            node->d_quant = Ast::Node::One;
            nextToken();
        }
        break;
    case EbnfToken::LBrace:
        {
            nextToken();
            node = parseExpression();
            if( node == 0 )
            {
                return 0;
            }
            if( d_cur.d_type != EbnfToken::RBrace )
            {
                error( d_cur, "expecting '}'" );
                delete node;
                return 0;
            }
            if( !checkCardinality(node) )
                return 0;
            node->d_quant = Ast::Node::ZeroOrMore;
            nextToken();
        }
        break;
    default:
        error( d_cur, "expecting keyword, delimiter, category, '{' or '['" );
        return 0;
    }
    return node;
}

Ast::Node*EbnfParser::parseExpression()
{
    // expression ::= term { "|" term }

    Ast::Node* node = 0;

    const EbnfToken first = d_cur;
    switch( d_cur.d_type )
    {
    case EbnfToken::Keyword:
    case EbnfToken::Literal:
    case EbnfToken::NonTerm:
    case EbnfToken::LBrack:
    case EbnfToken::LPar:
    case EbnfToken::LBrace:
    case EbnfToken::Predicate:
        node = parseTerm();
        break;
    default:
        error( d_cur, "expecting term" );
        return 0;
    }
    if( node == 0 )
        return 0;
    Ast::Node* alternative = 0;
    while( d_cur.d_type == EbnfToken::Bar )
    {
        nextToken();
        if( alternative == 0 )
        {
            alternative = new Ast::Node( Ast::Node::Alternative, d_def );
            alternative->d_tok.d_lineNr = first.d_lineNr;
            alternative->d_tok.d_colNr = first.d_colNr;
            alternative->d_subs.append(node);
            node->d_parent = alternative;
            node = alternative;
        }
        Ast::Node* n = parseTerm();
        if( n == 0 )
        {
            delete node;
            return 0;
        }
        alternative->d_subs.append(n);
        n->d_parent = alternative;
    }
    return node;
}

Ast::Node*EbnfParser::parseTerm()
{
    // term ::= [ predicate ] factor { factor }

    Ast::Node* node = 0;

    EbnfToken pred;
    if( d_cur.d_type == EbnfToken::Predicate )
    {
        pred = d_cur;
        nextToken();
    }

    EbnfToken first = d_cur;
    switch( d_cur.d_type )
    {
    case EbnfToken::Keyword:
    case EbnfToken::Literal:
    case EbnfToken::NonTerm:
    case EbnfToken::LBrack:
    case EbnfToken::LBrace:
    case EbnfToken::LPar:
        node = parseFactor();
        break;
    default:
        error( d_cur, "expecting factor" );
        return 0;
    }
    if( node == 0 )
        return 0;

    Ast::Node* sequence = 0;
    if( pred.isValid() )
    {
        sequence = new Ast::Node( Ast::Node::Sequence, d_def );
        sequence->d_tok.d_lineNr = first.d_lineNr;
        sequence->d_tok.d_colNr = first.d_colNr;
        sequence->d_subs.append(new Ast::Node( Ast::Node::Predicate, d_def, pred ));
        sequence->d_subs.back()->d_parent = sequence;
        sequence->d_subs.append(node);
        node->d_parent = sequence;
        node = sequence;
    }

    while( d_cur.d_type == EbnfToken::Keyword ||
           d_cur.d_type == EbnfToken::Literal ||
           d_cur.d_type == EbnfToken::NonTerm ||
           d_cur.d_type == EbnfToken::LBrack ||
           d_cur.d_type == EbnfToken::LPar ||
           d_cur.d_type == EbnfToken::LBrace )
    {
        if( sequence == 0 )
        {
            sequence = new Ast::Node( Ast::Node::Sequence, d_def );
            sequence->d_tok.d_lineNr = node->d_tok.d_lineNr;
            sequence->d_tok.d_colNr = node->d_tok.d_colNr;
            sequence->d_subs.append(node);
            node->d_parent = sequence;
            node = sequence;
        }
        Ast::Node* n = parseFactor();
        if( n == 0 )
        {
            delete node;
            return 0;
        }
        sequence->d_subs.append(n);
        n->d_parent = sequence;
    }
    return node;
}

bool EbnfParser::checkCardinality(Ast::Node* node)
{
    if( node->d_quant != Ast::Node::One )
    {
        error( d_cur, "contradicting nested quantifiers" );
        delete node;
        return false;
    }
    if( node->d_type != Ast::Node::Sequence && node->d_type != Ast::Node::Alternative )
    {
        Q_ASSERT( node->d_subs.isEmpty() );
        return true;
    }
    if( node->d_subs.isEmpty() )
    {
        error( d_cur, "container with zero items" );
        delete node;
        return false;
    }
    if( node->d_subs.size() == 1 &&
            ( node->d_subs.first()->d_type == Ast::Node::Sequence
              || node->d_subs.first()->d_type == Ast::Node::Alternative))
    {
        error( d_cur, "container containing only one other sequence or alternative" );
        delete node;
        return false;
    }
    return true;
}

void EbnfParser::handlePpSym(const EbnfToken& t)
{
    switch( t.d_type )
    {
    case EbnfToken::PpDefine:
        d_defines.insert(t.d_val);
        break;
    case EbnfToken::PpUndef:
        d_defines.remove(t.d_val);
        break;
    case EbnfToken::PpIfdef:
        {
            const bool lastTx = txOn();
            const bool curTx = lastTx && d_defines.contains(t.d_val);
            d_ifState.push( qMakePair(quint8(curTx ? IfActive : InIf),curTx) );
            txlog(t.d_lineNr + 1, lastTx );
        }
        break;
    case EbnfToken::PpIfndef:
        {
            const bool lastTx = txOn();
            const bool curTx = ( lastTx ) && !d_defines.contains(t.d_val);
            d_ifState.push( qMakePair(quint8(curTx ? IfActive : InIf),curTx) );
            txlog(t.d_lineNr + 1, lastTx );
        }
        break;
    case EbnfToken::PpElse:
        if( d_ifState.isEmpty() || ( d_ifState.top().first != InIf && d_ifState.top().first != IfActive ) )
            error( t, "#else not expected");
        else
        {
            const bool lastTx = txOn();
            const bool curTx = ( d_ifState.size() == 1 || d_ifState[d_ifState.size()-2].second ) &&
                    d_ifState.top().first != IfActive;
            d_ifState.top().first = InElse;
            d_ifState.top().second = curTx;
            txlog( d_cur.d_lineNr + ( curTx ? -1 : 1 ), lastTx );
        }
        break;
    case EbnfToken::PpEndif:
        if( d_ifState.isEmpty() )
            error(t,"#endif not expected");
        else
        {
            const bool lastTx = txOn();
            d_ifState.pop();
            txlog( d_cur.d_lineNr - 1, lastTx );
        }
        break;
    default:
        break;
    }
}

bool EbnfParser::txOn() const
{
    return d_ifState.isEmpty() || d_ifState.top().second;
}

void EbnfParser::txlog(quint32 line, bool lastOn )
{
    const bool on = txOn();
    if( lastOn != on )
    {
        d_syn->addIdol(line);
    }
}


