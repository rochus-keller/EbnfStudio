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

#include "EbnfEditor.h"
#include "EbnfErrors.h"
#include "EbnfHighlighter.h"
#include "EbnfLexer.h"
#include "EbnfParser.h"
#include <GuiTools/AutoMenu.h>
#include <QPainter>
#include <QtDebug>
#include <QFile>
#include <QScrollBar>
#include <QBuffer>
#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QPrinter>
#include <QFileDialog>
#include <QFileInfo>
#include <QPrintDialog>
#include <QSettings>
#include <QFontDialog>
#include <QShortcut>
#include <QTextBlock>
#include <QMessageBox>

EbnfEditor::EbnfEditor(QWidget *parent) :
    CodeEditor(parent)
{
    d_errs = new EbnfErrors(this);
    d_hl = new EbnfHighlighter( document() );
	updateTabWidth();

}

void EbnfEditor::markNonTerms(const SymList& syms)
{
    d_nonTerms.clear();
    QTextCharFormat format;
    format.setBackground( QColor(247,245,243) );
    foreach( const Ast::Symbol* s, syms )
    {
        if( s == 0 )
            continue;
        QTextCursor c( document()->findBlockByNumber( s->d_tok.d_lineNr - 1) );
        c.setPosition( c.position() + s->d_tok.d_colNr - 1 );
        c.setPosition( c.position() + s->d_tok.d_val.size(), QTextCursor::KeepAnchor );

        QTextEdit::ExtraSelection sel;
        sel.format = format;
        sel.cursor = c;

        d_nonTerms << sel;
    }
    updateExtraSelections();
}

void EbnfEditor::updateExtraSelections()
{
    ESL sum;

    if( d_syn.constData() != 0 && !d_syn->getIdol().isEmpty() )
    {
        bool on = true;
        quint32 startLine = 1;
        for( int i = 0; i < d_syn->getIdol().size(); i++ )
        {
            on = !on;
            if( !on )
                startLine = d_syn->getIdol()[i];
            else
            {
                QTextEdit::ExtraSelection line;
                line.format.setBackground(QColor(Qt::lightGray).lighter(120));
                line.format.setProperty(QTextFormat::FullWidthSelection, true);
                for( int l = startLine; l <= d_syn->getIdol()[i]; l++ )
                {
                    QTextCursor c( document()->findBlockByNumber(l - 1) );
                    line.cursor = c;
                    sum << line;
                }
            }
        }
    }

    QTextEdit::ExtraSelection line;
    line.format.setBackground(QColor(Qt::yellow).lighter(170));
    line.format.setProperty(QTextFormat::FullWidthSelection, true);
    line.cursor = textCursor();
    line.cursor.clearSelection();
    sum << line;

    sum << d_nonTerms;

    if( !d_errs->getErrors().isEmpty() )
    {
        QTextCharFormat errorFormat;
        errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        errorFormat.setUnderlineColor(Qt::magenta);
        EbnfErrors::EntryList::const_iterator i;
        for( i = d_errs->getErrors().begin(); i != d_errs->getErrors().end(); ++i )
        {
            QTextCursor c( document()->findBlockByNumber((*i).d_line - 1) );

            c.setPosition( c.position() + (*i).d_col - 1 );
            c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

            QTextEdit::ExtraSelection sel;
            sel.format = errorFormat;
            sel.cursor = c;
            sel.format.setToolTip((*i).d_msg);

            sum << sel;
        }
    }

    sum << d_link;

    setExtraSelections(sum);
}

void EbnfEditor::mousePressEvent(QMouseEvent* e)
{
    if( !d_link.isEmpty() )
    {
        QTextCursor cur = cursorForPosition(e->pos());
        pushLocation( Location( cur.blockNumber(), cur.positionInBlock() ) );

        QApplication::restoreOverrideCursor();
        d_link.clear();
        setCursorPosition( d_linkLineNr, 0, true );
    }else if( QApplication::keyboardModifiers() == Qt::ControlModifier )
    {
        QTextCursor cur = cursorForPosition(e->pos());
        const Ast::Symbol* sym = d_syn->findSymbolBySourcePos(cur.blockNumber() + 1,cur.positionInBlock() + 1);
        if( sym )
        {
            const Ast::Definition* d = d_syn->getDef( sym->d_tok.d_val );
            if( d )
            {
                pushLocation( Location( cur.blockNumber(), cur.positionInBlock() ) );
                setCursorPosition( d->d_tok.d_lineNr - 1, 0, true );
            }
       }
    }else
        QPlainTextEdit::mousePressEvent(e);
}

void EbnfEditor::mouseMoveEvent(QMouseEvent* e)
{
    QPlainTextEdit::mouseMoveEvent(e);
    if( QApplication::keyboardModifiers() == Qt::ControlModifier && d_syn.constData() )
    {
        QTextCursor cur = cursorForPosition(e->pos());
        const Ast::Symbol* sym = d_syn->findSymbolBySourcePos(cur.blockNumber() + 1, cur.positionInBlock() + 1);
        const bool alreadyArrow = !d_link.isEmpty();
        d_link.clear();
        if( sym )
        {
            //qDebug() << "Sym" << sym->d_tok.d_type << sym->d_tok.d_val;
            const int off = cur.positionInBlock() + 1 - sym->d_tok.d_colNr;
            cur.setPosition(cur.position() - off);
            cur.setPosition( cur.position() + sym->d_tok.d_len, QTextCursor::KeepAnchor );
            const Ast::Definition* d = d_syn->getDef( sym->d_tok.d_val );

            if( d )
            {
                QTextEdit::ExtraSelection sel;
                sel.cursor = cur;
                sel.format.setFontUnderline(true);
                d_link << sel;
                d_linkLineNr = d->d_tok.d_lineNr - 1;
                if( !alreadyArrow )
                    QApplication::setOverrideCursor(Qt::ArrowCursor);
            }
        }
        if( alreadyArrow && d_link.isEmpty() )
            QApplication::restoreOverrideCursor();
        updateExtraSelections();
    }else if( !d_link.isEmpty() )
    {
        QApplication::restoreOverrideCursor();
        d_link.clear();
        updateExtraSelections();
    }
}

void EbnfEditor::onUpdateModel()
{
    parseText( toPlainText().toUtf8() );
}

void EbnfEditor::parseText(QByteArray ba)
{
    QBuffer buf(&ba);
    buf.open(QIODevice::ReadOnly );
    EbnfLexer l;
    l.setKeywords( d_origKeyWords );
    EbnfParser p;
    p.setErrors(d_errs);
    d_errs->clear();
    d_nonTerms.clear();
    d_syn = 0;
    l.setStream( &buf );
    if( !p.parse( &l ) )
    {
        //qDebug() << "parsing" << d_path << "not successful," << d_errs->getErrCount() << "errors";
        // qDebug() << p.getErrors();
    }else
    {
        d_syn = p.getSyntax();
        if( d_syn->finishSyntax() )
        {
            //qDebug() << "parsing" << d_path << "successful, no errors";
        }else
        {
            //qDebug() << "parsing" << d_path << "not successful," << d_errs->getErrCount() << "errors";
            // qDebug() << p.getSyntax()->getErrors();
        }
        if( l.getKeywords().size() != d_hl->getKeywords().size() )
        {
            d_hl->setKeywords( l.getKeywords() );
            d_rehighlightLock = true;
            d_hl->rehighlight(); // triggert onTextChanged
            d_rehighlightLock = false;
        }
    }
    emit sigSyntaxUpdated();
    updateExtraSelections();
    //p.getSyntax()->dump();
}

bool EbnfEditor::loadFromFile(const QString &path)
{
    QFile file(path);
    if( !file.open(QIODevice::ReadOnly ) )
        return false;
    EbnfToken::resetSymTbl();
    loadKeywords(path);
    setPlainText( QString::fromUtf8( file.readAll() ) );
    d_path = path;
    d_backHisto.clear();
    d_forwardHisto.clear();
    document()->setModified( false );
    emit modificationChanged(false);
    return true;
}

bool EbnfEditor::loadKeywords(const QString& path)
{
    EbnfLexer l;
    QFileInfo info(path);
    l.readKeywordsFromFile( info.absoluteDir().absoluteFilePath( info.completeBaseName() + ".keywords" ) );
    d_origKeyWords = l.getKeywords();
    d_hl->setKeywords( d_origKeyWords );
    return true;
}

void EbnfEditor::newFile()
{
    setPlainText(QString());
    d_path.clear();
    d_backHisto.clear();
    d_forwardHisto.clear();
    document()->setModified( false );
    emit modificationChanged(false);
}

bool EbnfEditor::saveToFile(const QString& path)
{
    QFile file(path);
    if( !file.open(QIODevice::WriteOnly ) )
    {
        QMessageBox::critical(this,tr("Save EBNF File"), tr("Cannot save file to '%1'. %2").
                              arg(path).arg(file.errorString()));
        return false;
    }

    file.write( toPlainText().toUtf8() );
    document()->setModified( false );
    d_path = path;
    return true;
}
