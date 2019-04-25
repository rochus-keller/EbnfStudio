#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

#include <QMainWindow>

class EbnfEditor;
class QTreeWidget;
class SyntaxTreeMdl;
class FirstFollowSet;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void open( const QString& path );

protected slots:
    void onCaption();
    void onErrors();
    void onErrorsDblClicked();
    void onCursorChanged();
    void onSave();
    void onSaveAs();
    void onOpen();
    void onNew();
    void onTransformHtml();
    void onTransformIeeeEbnf();
    void onSyntaxUpdated();
    void onTreeDblClicked();
    void onExpandSelected();
    void onGenSynTree();
    void onGenTt();
    void onGenHtml();
    void onGenCoco();
    void onGenAntlr();
    void onGenLlgen();
    void onOutputFirstSet();
    void onUsedByDblClicked();
    void onFindAmbig();
    void onReloadKeywords();
    void onAbout();
    void onDetailsDblClicked();
    void onLink(const QString&);
    void onPathDblClicked();

protected:
    void createMenus();
    void createIssues();
    void createTree();
    void createDetails();
    void createPathView();
    void createUsedBy();
    bool checkSaved( const QString& title );

    // overrides
    void closeEvent ( QCloseEvent * event );
private:
    EbnfEditor* d_edit;
    QTreeWidget* d_errView;
    QTreeWidget* d_usedBy;
    QTreeWidget* d_errDetails;
    QTreeWidget* d_pathView;
    QLabel* d_errText;
    SyntaxTreeMdl* d_mdl;
    FirstFollowSet* d_tbl;
};

#endif // MAINWINDOW_H
