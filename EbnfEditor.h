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
#include "EbnfSyntax.h"

// adaptiert aus AdaViewer::AdaEditor

class EbnfHighlighter;
class EbnfErrors;

class EbnfEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit EbnfEditor(QWidget *parent = 0);
    static QFont defaultFont();

    bool loadFromFile(const QString& path );
    bool loadKeywords(const QString& path );
    void newFile();
    bool saveToFile( const QString& path );
    QString getPath() const { return d_path; }
    EbnfSyntax* getSyntax() const { return d_syn.data(); }
    EbnfErrors* getErrs() const { return d_errs; }

    void paintHandleArea(QPaintEvent *event);
    int handleAreaWidth();
    int lineAt( const QPoint& ) const;

    QString textLine( int i ) const;
    int lineCount() const;
    void ensureLineVisible( int line );
    void setPositionMarker( int line ); // -1..unsichtbar
    void setSelection(int lineFrom,int indexFrom, int lineTo,int indexTo);
    void selectLines( int lineFrom, int lineTo );
    bool hasSelection() const;
    QString selectedText() const;

    void getCursorPosition(int *line,int *col = 0);
    void setCursorPosition(int line, int col, bool center = false);
    typedef QList<const EbnfSyntax::Node*> NodeList;
    void markNonTerms(const NodeList& );
    void clearNonTerms();
    void updateExtraSelections();

    bool isUndoAvailable() const { return d_undoAvail; }
    bool isRedoAvailable() const { return d_redoAvail; }
    bool isCopyAvailable() const { return d_copyAvail; }

    bool isModified() const;

    void indent();
    void setIndentation(int);
    void unindent();

    void installDefaultPopup();

    void setShowNumbers( bool on );
    bool showNumbers() const { return d_showNumbers; }

signals:
    void sigSyntaxUpdated();

public slots:
    void handleEditUndo();
    void handleEditRedo();
    void handleEditCut();
    void handleEditCopy();
    void handleEditPaste();
    void handleEditSelectAll();
    void handleFind();
    void handleFindAgain();
    void handleReplace();
    void handleGoto();
    void handleIndent();
    void handleUnindent();
    void handleSetIndent();
    void handleFixIndent();
    void handlePrint();
    void handleExportPdf();
    void handleShowLinenumbers();
    void handleSetFont();
    void handleGoBack();
    void handleGoForward();
protected:
    friend class _HandleArea;
    struct Location
    {
        // Qt-Koordinaten
        int d_line;
        int d_col;
        bool operator==( const Location& rhs ) { return d_line == rhs.d_line && d_col == rhs.d_col; }
        Location(int l, int c ):d_line(l),d_col(c){}
    };
    void pushLocation( const Location& );
    void paintIndents( QPaintEvent *e );
    void updateTabWidth();
    void find(bool fromTop);
    void parseText(QByteArray);

    // overrides
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *e);
    bool viewportEvent( QEvent * event );
    void keyPressEvent ( QKeyEvent * e );
    void mousePressEvent(QMouseEvent * e);
    void mouseMoveEvent(QMouseEvent * e);

    // To override
    virtual void numberAreaDoubleClicked( int ) {}
private slots:
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);
    void onUndoAvail(bool on) { d_undoAvail = on; }
    void onRedoAvail(bool on) { d_redoAvail = on; }
    void onCopyAvail(bool on) { d_copyAvail = on; }
    void onUpdateCursor();
    void onTextChanged();
    void onUpdateModel();
    void onUpdateLocation();
private:
    QWidget* d_numberArea;
    int d_curPos; // Zeiger für die aktuelle Ausführungsposition oder -1
    QString d_find;
    QString d_path;
    EbnfHighlighter* d_hl;
    EbnfErrors* d_errs;
    EbnfSyntaxRef d_syn;
    QTimer d_typingLatency;
    QTimer d_cursorLatency;
    typedef QList<QTextEdit::ExtraSelection> ESL;
    ESL d_link;
    ESL d_nonTerms;
    int d_linkLineNr;
    QList<Location> d_backHisto; // d_backHisto.last() ist aktuell angezeigtes Objekt
    QList<Location> d_forwardHisto;
    bool d_pushBackLock;
    bool d_undoAvail;
    bool d_redoAvail;
    bool d_copyAvail;
    bool d_showNumbers;
};

#endif // EBNFEDITOR_H
