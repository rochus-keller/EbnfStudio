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

#include "SyntaxTreeMdl.h"
#include <QBrush>
#include <QPixmap>
#include <QtDebug>
#include <QTreeView>

SyntaxTreeMdl::SyntaxTreeMdl(QTreeView* parent) :
    QAbstractItemModel(parent)
{

}

QTreeView*SyntaxTreeMdl::getParent() const
{
    return static_cast<QTreeView*>(QObject::parent());
}

void SyntaxTreeMdl::setSyntax( EbnfSyntax* syn )
{
    beginResetModel();
    d_root = Slot();
    d_syn = syn;
    fillTop();
    endResetModel();
}

const EbnfSyntax::Symbol* SyntaxTreeMdl::getSymbol(const QModelIndex& index) const
{
    if( !index.isValid() || d_syn.constData() == 0 )
        return 0;
    Slot* s = static_cast<Slot*>( index.internalPointer() );
    Q_ASSERT( s != 0 );
    return s->d_sym;
}

QModelIndex SyntaxTreeMdl::findSymbol(quint32 line, quint16 col)
{
    return findSymbol( &d_root, line, col );
}

static inline QString _quant( quint8 q, const QString& txt, bool nullable = false, bool repeatable = false )
{
    switch( q )
    {
    case EbnfSyntax::Node::One:
        if( repeatable )
            return QString("{ %1").arg(txt);
        else if( nullable )
            return QString("[ %1").arg(txt);
        else
            return txt;
    case EbnfSyntax::Node::ZeroOrOne:
        return QString("[ %1 ]").arg(txt);
        break;
    case EbnfSyntax::Node::ZeroOrMore:
        return QString("{ %1 }").arg(txt);
    default:
        return "<unknown quantifier>";
    }
}

QVariant SyntaxTreeMdl::data(const QModelIndex& index, int role) const
{
    if( !index.isValid() || d_syn.constData() == 0 )
        return QVariant();

    Slot* s = static_cast<Slot*>( index.internalPointer() );
    Q_ASSERT( s != 0 );
    switch( role )
    {
    case Qt::DisplayRole:
        if( s->d_sym->d_tok.d_type != EbnfToken::Production )
        {
            const EbnfSyntax::Node* node = static_cast<const EbnfSyntax::Node*>( s->d_sym );
            switch( node->d_type )
            {
            case EbnfSyntax::Node::Terminal:
            case EbnfSyntax::Node::Nonterminal:
                return _quant( node->d_quant, node->d_tok.d_val.toStr(), node->isNullable(), node->isRepeatable() );
            case EbnfSyntax::Node::Sequence:
                return _quant( node->d_quant, "seq" );
            case EbnfSyntax::Node::Alternative:
                return _quant( node->d_quant, "alt" );
            default:
                return s->d_sym->d_tok.d_val.toBa();
            }
        }else if( s->d_sym->isRepeatable() )
            return QString("{ %1").arg(s->d_sym->d_tok.d_val.toStr());
        else if( s->d_sym->isNullable() )
            return QString("[ %1").arg(s->d_sym->d_tok.d_val.toStr());
        else
            return s->d_sym->d_tok.d_val.toBa();
        break;
    case Qt::ForegroundRole:
        if( s->d_sym->d_tok.d_type != EbnfToken::Production )
        {
            const EbnfSyntax::Node* node = static_cast<const EbnfSyntax::Node*>( s->d_sym );
            switch( node->d_type )
            {
            case EbnfSyntax::Node::Terminal:
                return QBrush( Qt::red );
            case EbnfSyntax::Node::Predicate:
                return QBrush( Qt::darkCyan );
            case EbnfSyntax::Node::Sequence:
            case EbnfSyntax::Node::Alternative:
                return QBrush( Qt::blue );
            default:
                break;
            }
        }
        break;
    case Qt::FontRole:
        if( s->d_sym->d_tok.d_type != EbnfToken::NonTerm && s->d_sym->d_tok.d_type != EbnfToken::Predicate &&
                s->d_sym->d_tok.d_type != EbnfToken::Production && s->d_sym->d_tok.d_type != EbnfToken::Invalid )
        {
            QFont f = getParent()->font();
            f.setBold(true);
            return f;
        }else if( s->d_sym->d_tok.d_op == EbnfToken::Transparent )
        {
            QFont f = getParent()->font();
            f.setItalic(true);
            return f;
        }
        break;
    case Qt::BackgroundRole:
        if( s->d_sym->doIgnore() && s->d_sym->d_tok.d_type != EbnfToken::Predicate )
            return QBrush( Qt::lightGray );
        break;
    }
    return QVariant();
}

QModelIndex SyntaxTreeMdl::parent ( const QModelIndex & index ) const
{
    if( index.isValid() )
    {
        Slot* s = static_cast<Slot*>( index.internalPointer() );
        Q_ASSERT( s != 0 );
        if( s->d_parent == &d_root )
            return QModelIndex();
        // else
        Q_ASSERT( s->d_parent != 0 );
        Q_ASSERT( s->d_parent->d_parent != 0 );
        return createIndex( s->d_parent->d_parent->d_children.indexOf( s->d_parent ), 0, s->d_parent );
    }else
        return QModelIndex();
}

int SyntaxTreeMdl::rowCount ( const QModelIndex & parent ) const
{
    if( parent.isValid() )
    {
        Slot* s = static_cast<Slot*>( parent.internalPointer() );
        Q_ASSERT( s != 0 );
        return s->d_children.size();
    }else
        return d_root.d_children.size();
}

QModelIndex SyntaxTreeMdl::index ( int row, int column, const QModelIndex & parent ) const
{
    const Slot* s = &d_root;
    if( parent.isValid() )
    {
        s = static_cast<Slot*>( parent.internalPointer() );
        Q_ASSERT( s != 0 );
    }
    if( row < s->d_children.size() && column < columnCount( parent ) )
        return createIndex( row, column, s->d_children[row] );
    else
        return QModelIndex();
}

Qt::ItemFlags SyntaxTreeMdl::flags( const QModelIndex & index ) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; //  | Qt::ItemIsDragEnabled;
}

void SyntaxTreeMdl::fillTop()
{
    if( d_syn.constData() == 0 )
        return;
    typedef QMap<QByteArray,EbnfSyntax::Definition*> Sorter;
    Sorter sorter;
    for( EbnfSyntax::Definitions::const_iterator i = d_syn->getDefs().begin(); i != d_syn->getDefs().end(); ++i )
        sorter.insert( i.key().toBa().toLower(), i.value() );

    for( Sorter::const_iterator j = sorter.begin(); j != sorter.end(); ++j )
    {
        Slot* s = new Slot();
        s->d_parent = &d_root;
        s->d_sym = j.value();
        d_root.d_children.append( s );

        fill( s, j.value()->d_node );
    }
}

QModelIndex SyntaxTreeMdl::findSymbol(SyntaxTreeMdl::Slot* slot, quint32 line, quint16 col) const
{
    for( int i = 0; i < slot->d_children.size(); i++ )
    {
        Slot* s = slot->d_children[i];
        if( s->d_sym->d_tok.d_lineNr == line && s->d_sym->d_tok.d_colNr <= col &&
                col < ( s->d_sym->d_tok.d_colNr + s->d_sym->d_tok.d_len ) )
            return createIndex( i, 0, s );
        QModelIndex index = findSymbol( s, line, col );
        if( index.isValid() )
            return index;
    }
    return QModelIndex();

}

void SyntaxTreeMdl::fill(Slot* super, const EbnfSyntax::Node* sym )
{
    if( sym == 0 )
        return;

    Slot* s = new Slot();
    s->d_parent = super;
    s->d_sym = sym;
    super->d_children.append( s );


    foreach( const EbnfSyntax::Node* sub, sym->d_subs )
    {
        fill(s, sub );
    }
}

