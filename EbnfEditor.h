#ifndef EBNFEDITOR_H
#define EBNFEDITOR_H

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

#include <QPlainTextEdit>
#include <QSet>
#include <QTimer>
#include <GuiTools/CodeEditor.h>
#include "EbnfSyntax.h"

// adaptiert aus AdaViewer::AdaEditor

class EbnfHighlighter;
class EbnfErrors;

class EbnfEditor : public CodeEditor
{
    Q_OBJECT
public:
    explicit EbnfEditor(QWidget *parent = 0);

    bool loadFromFile(const QString& path );
    bool loadKeywords(const QString& path );
    void newFile();
    bool saveToFile( const QString& path );
    EbnfSyntax* getSyntax() const { return d_syn.data(); }
    EbnfErrors* getErrs() const { return d_errs; }

    bool hasSelection() const;
    QString selectedText() const;

    typedef QList<const Ast::Symbol*> SymList;
    void markNonTerms(const SymList& );
    void updateExtraSelections();

signals:
    void sigSyntaxUpdated();

public slots:

protected:
    void mousePressEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void onUpdateModel();
    void parseText(QByteArray ba);
private:
    EbnfHighlighter* d_hl;
    EbnfErrors* d_errs;
    EbnfSyntaxRef d_syn;
    typedef QSet<EbnfToken::Sym> Keywords;
    Keywords d_origKeyWords;
};

#endif // EBNFEDITOR_H
