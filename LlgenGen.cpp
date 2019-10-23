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

#include "LlgenGen.h"
#include "SynTreeGen.h"
#include "GenUtils.h"
#include "EbnfAnalyzer.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDir>
#include <QtDebug>

bool LlgenGen::generate(const QString& atgPath, EbnfSyntax* syn, FirstFollowSet* tbl)
{
    if( syn == 0 || syn->getOrderedDefs().isEmpty() )
        return false;

    const EbnfSyntax::Definition* root = syn->getOrderedDefs()[0];

    QFile f(atgPath);
    f.open( QIODevice::WriteOnly );
    QTextStream out(&f);
    out.setCodec("utf-8");


    out << "// This file was automatically generated by EbnfStudio; don't modify it!" << endl << endl;


    out << "%options \"generate-lexer-wrapper=no\";" << endl << endl;

    out << "%token" << endl;
    const QStringList tokens = GenUtils::orderedTokenList( EbnfAnalyzer::collectAllTerminalStrings( syn ) ) +
            EbnfAnalyzer::collectAllTerminalProductions( syn );
    for( int t = 0; t < tokens.size(); t++ )
    {
        if( t != 0 )
            out << "," << endl;

        out << "\t" << tokenName(tokens[t]);
    }
    out << ";" << endl << endl;

    out << endl;

    out << "%start parser, " << ruleName(root->d_tok.d_val.toStr()) << ";" << endl << endl;

    for( int i = 0; i < syn->getOrderedDefs().size(); i++ )
    {
        const EbnfSyntax::Definition* d = syn->getOrderedDefs()[i];
        if( d->d_tok.d_op == EbnfToken::Skip || ( i != 0 && d->d_usedBy.isEmpty() ) )
            continue;
        if( d->d_node == 0 )
            continue;
        out << ruleName( d->d_tok.d_val.toStr() ) << " : " << endl << "    ";
        writeNode( out, d->d_node, true, tbl );
        out << endl << "    ;" << endl << endl;
    }
    return true;
}

void LlgenGen::writeNode(QTextStream& out, EbnfSyntax::Node* node, bool topLevel, FirstFollowSet* tbl)
{
    if( node == 0 )
        return;

    if( node->d_tok.d_op == EbnfToken::Skip )
        return;
    if( node->d_def && node->d_def->d_tok.d_op == EbnfToken::Skip )
        return;

    switch( node->d_quant )
    {
    case EbnfSyntax::Node::One:
        if( !topLevel && node->d_type == EbnfSyntax::Node::Alternative )
            out << "[ ";
        else if( !topLevel && node->d_type == EbnfSyntax::Node::Sequence && !node->d_tok.d_val.isEmpty() )
            out << "[ ";
        break;
    case EbnfSyntax::Node::ZeroOrOne:
    case EbnfSyntax::Node::ZeroOrMore:
        out << "[ ";
        break;
    }
    switch( node->d_type )
    {
    case EbnfSyntax::Node::Terminal:
        out << tokenName( node->d_tok.d_val.toStr() ) << " ";
        break;
    case EbnfSyntax::Node::Nonterminal:
        if( node->d_def == 0 || node->d_def->d_node == 0 )
            out << tokenName(node->d_tok.d_val.toStr()) << " ";
        else
            out << ruleName(node->d_tok.d_val.toStr()) << " ";
        break;
    case EbnfSyntax::Node::Alternative:
        for( int i = 0; i < node->d_subs.size(); i++ )
        {
            if( i != 0 )
            {
                if( topLevel )
                    out << endl << "    | ";
                else
                    out << "| ";
            }
            writeNode( out, node->d_subs[i], false, tbl );
        }
        break;
    case EbnfSyntax::Node::Sequence:
        for( int i = 0; i < node->d_subs.size(); i++ )
        {
            if( node->d_subs[i]->d_type == EbnfSyntax::Node::Predicate )
                ; // handlePredicate( out, node->d_subs[i], node, tbl );
            else
                writeNode( out, node->d_subs[i], false, tbl );
        }
        break;
    default:
        break;
    }
    switch( node->d_quant )
    {
    case EbnfSyntax::Node::One:
        if( !topLevel && node->d_type == EbnfSyntax::Node::Alternative )
            out << "] ";
        else if( !topLevel && node->d_type == EbnfSyntax::Node::Sequence && !node->d_tok.d_val.isEmpty() )
            out << "] ";
        break;
    case EbnfSyntax::Node::ZeroOrOne:
        out << "]? ";
        break;
    case EbnfSyntax::Node::ZeroOrMore:
        out << "]* ";
        break;
    }
}

QString LlgenGen::tokenName(const QString& str)
{
    QString tok = GenUtils::symToString(str).toUpper();
    if( !tok.isEmpty() && tok[0].isDigit() )
        tok = QChar('T') + tok;
    return tok;
}

QString LlgenGen::ruleName(const QString& str)
{
    return GenUtils::escapeDollars( str ).toLower();
}

void LlgenGen::handlePredicate(QTextStream& out, EbnfSyntax::Node* pred, EbnfSyntax::Node* sequence, FirstFollowSet* tbl)
{
    const QString val = pred->d_tok.d_val.toStr();
    if( val.startsWith("LL:") )
    {
        const int ll = val.mid(3).toInt();
        if( ll <= 1 )
            return;

        EbnfAnalyzer::LlkNodes llkNodes;
        //EbnfAnalyzer::calcLlkFirstSet2( ll, llkNodes,sequence, tbl );
        EbnfAnalyzer::calcLlkFirstSet( ll, llkNodes,sequence, tbl );
        out << "%if( ";
        for( int i = 0; i < llkNodes.size(); i++ )
        {
            if( i != 0 )
                out << "&& ";
            if( llkNodes[i].size() > 1 )
                out << "( ";
            EbnfSyntax::NodeRefSet::const_iterator j;
            for( j = llkNodes[i].begin(); j != llkNodes[i].end(); ++j )
            {
                if( j != llkNodes[i].begin() )
                    out << "|| ";
                out << "peek(" << i+1 << ") == _" << tokenName( (*j).d_node->d_tok.d_val.toStr()) << " ";
            }
            if( llkNodes[i].size() > 1 )
                out << ") ";
        }
        out << ") ";
    }else
        qWarning() << "LlgenGen unknown predicate" << sequence->d_tok.d_val.toStr();
}

LlgenGen::LlgenGen()
{

}

