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

#include "MainWindow.h"
#include "EbnfEditor.h"
#include "EbnfErrors.h"
#include "HtmlSyntax.h"
#include "EbnfAnalyzer.h"
#include "SyntaxTreeMdl.h"
#include "SynTreeGen.h"
#include "GenUtils.h"
#include "CocoGen.h"
#include "LlgenGen.h"
#include "FirstFollowSet.h"
#include "AntlrGen.h"
#include <QFile>
#include <QFileInfo>
#include <QtDebug>
#include <QTreeWidget>
#include <QDockWidget>
#include <QHeaderView>
#include <QSettings>
#include <QShortcut>
#include <QMessageBox>
#include <QFileDialog>
#include <QElapsedTimer>
#include <QApplication>
#include <QDateTime>
#include <QVBoxLayout>
#include <QLabel>
#include <Gui2/AutoMenu.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    d_tbl = new FirstFollowSet(this);
    d_edit = new EbnfEditor(this);
    d_edit->installDefaultPopup();
    setCentralWidget(d_edit);

    setDockNestingEnabled(true);
    setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );
    setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
    setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
    setCorner( Qt::TopLeftCorner, Qt::LeftDockWidgetArea );

    createMenus();
    createIssues();
    createDetails();
    createTree();
    createUsedBy();

    setWindowTitle(tr("EbnfStudio") );

    connect(d_edit, SIGNAL(modificationChanged(bool)), this, SLOT(onCaption()) );
    connect(d_edit, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorChanged()) );
    connect(d_edit->getErrs(), SIGNAL(sigChanged()), this, SLOT(onErrors()) );

    resize( 800, 400 );
    showMaximized();

    QSettings s;
    const QVariant state = s.value( "DockState" );
    if( !state.isNull() )
        restoreState( state.toByteArray() );

}

MainWindow::~MainWindow()
{

}

void MainWindow::open(const QString& path)
{
    d_edit->loadFromFile(path);
    onCaption();
}

void MainWindow::onCaption()
{
    if( d_edit->getPath().isEmpty() )
    {
        setWindowTitle(tr("<unnamed> - EbnfStudio"));
    }else
    {
        QFileInfo info(d_edit->getPath());
        QString star = d_edit->isModified() ? "*" : "";
        setWindowTitle(tr("%1%2 - EbnfStudio").arg(info.fileName()).arg(star) );
    }
}

static bool errorEntryLessThan(const EbnfErrors::Entry &s1, const EbnfErrors::Entry &s2)
{
    return s1.d_line < s2.d_line ||
            ( !(s2.d_line < s1.d_line) && s1.d_col < s2.d_col );
}

void MainWindow::onErrors()
{
    d_errView->clear();
    d_errDetails->clear();
    QList<EbnfErrors::Entry> errs = d_edit->getErrs()->getErrors().toList();
    std::sort(errs.begin(), errs.end(), errorEntryLessThan );
    //qDebug() << errs.size() << "errors found";

    for( int i = 0; i < errs.size(); i++ )
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(d_errView);
        item->setText(1, errs[i].d_msg );
        item->setToolTip(1, item->text(1) );
        if( errs[i].d_isErr )
            item->setIcon(0, QPixmap(":/images/exclamation-red.png") );
        else
            item->setIcon(0, QPixmap(":/images/exclamation-circle.png") );
        item->setText(0, QString("%1 : %2").arg(errs[i].d_line).arg(errs[i].d_col));
        item->setData(0, Qt::UserRole, errs[i].d_line );
        item->setData(0, Qt::UserRole+1, errs[i].d_data );
        item->setData(1, Qt::UserRole, errs[i].d_col );
    }
    if( errs.size() )
        d_errView->parentWidget()->show();
}

void MainWindow::onErrorsDblClicked()
{
    QTreeWidgetItem* item = d_errView->currentItem();
    if( item )
    {
        const bool blocked = d_edit->blockSignals(true);
        d_edit->setCursorPosition( item->data(0, Qt::UserRole ).toInt() - 1,
                                   item->data(1, Qt::UserRole ).toInt() - 1, true );
        d_edit->blockSignals(blocked);
        onCursorChanged();
        d_edit->setFocus();

        d_errText->setText(QString("<html><a href='%1'>%1</a> %2</html>").arg(item->text(0)).arg(item->text(1).toHtmlEscaped() ));
        d_errDetails->clear();
        EbnfSyntax::IssueData d = item->data(0,Qt::UserRole+1).value<EbnfSyntax::IssueData>();
        if( d.d_type )
        {
            foreach( const EbnfSyntax::Node* r, d.d_list )
            {
                QTreeWidgetItem* i = new QTreeWidgetItem(d_errDetails);
                i->setText( 0, r->d_tok.d_val.toStr() );
                i->setText( 1, r->d_owner->d_tok.d_val.toStr());
                i->setToolTip( 1, i->text(1));
                i->setData(0,Qt::UserRole, QPoint(r->d_tok.d_colNr,r->d_tok.d_lineNr) );
            }
            if( d.d_type != EbnfSyntax::IssueData::LeftRec )
                d_errDetails->sortItems(0,Qt::AscendingOrder);
            d_errDetails->resizeColumnToContents(0);
        }
    }
}

void MainWindow::onCursorChanged()
{
    int line, col;
    d_edit->getCursorPosition( &line, &col );
    line += 1;
    col += 1;
    for( int i = 0; i < d_errView->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem* item = d_errView->topLevelItem(i);
        const int itemLine = item->data(0, Qt::UserRole ).toInt();
        if( itemLine == line && item->data(1, Qt::UserRole ).toInt() <= col )
        {
            d_errView->setCurrentItem(item);
            d_errView->scrollToItem( item ); // QAbstractItemView::EnsureVisible
            d_errView->parentWidget()->show();
        }
        if( itemLine > line )
            break;
        // TODO: allenfalls binary search fall nötig
    }
    QModelIndex index = d_mdl->findSymbol( line, col );
    if( index.isValid() )
    {
        d_mdl->getParent()->setCurrentIndex(index);
        d_mdl->getParent()->scrollTo(index,QAbstractItemView::PositionAtCenter);
        const EbnfSyntax::Symbol* sym = d_mdl->getSymbol(index);
        d_usedBy->clear();
        if( sym )
        {
#if 1
            EbnfSyntax::ConstNodeList backrefs = d_edit->getSyntax()->getBackRefs(sym);
            foreach( const EbnfSyntax::NodeRef& r, backrefs )
            {
                const EbnfSyntax::Node* n = r.d_node;
                QTreeWidgetItem* i = new QTreeWidgetItem(d_usedBy);
                i->setText( 0, QString("%1 (%2:%3)").arg(n->d_owner->d_tok.d_val.toStr())
                            .arg(n->d_tok.d_lineNr).arg(n->d_tok.d_colNr));
                i->setToolTip( 0, i->text(0) );
                i->setData( 0, Qt::UserRole, QPoint( n->d_tok.d_colNr, n->d_tok.d_lineNr ) );
            }
            d_usedBy->sortItems(0,Qt::AscendingOrder);
            if( sym->d_tok.d_type == EbnfToken::Production || sym->d_tok.d_type == EbnfToken::NonTerm )
            {
                d_edit->markNonTerms( backrefs );
            }else
                d_edit->clearNonTerms();

#else
            const EbnfSyntax::Definition* d = 0;
            if( sym->d_tok.d_type == EbnfToken::Production )
                d = static_cast<const EbnfSyntax::Definition*>( sym );
            else if( sym->d_tok.d_type == EbnfToken::NonTerm )
                d = static_cast<const EbnfSyntax::Node*>( sym )->d_def;
            if( d )
            {
                foreach( const EbnfSyntax::Node* n, d->d_usedBy )
                {
                    QTreeWidgetItem* i = new QTreeWidgetItem(d_usedBy);
                    i->setText( 0, QString("%1 (%2:%3)").arg(n->d_owner->d_tok.d_val.toStr())
                                .arg(n->d_tok.d_lineNr).arg(n->d_tok.d_colNr));
                    i->setData( 0, Qt::UserRole, QPoint( n->d_tok.d_colNr, n->d_tok.d_lineNr ) );
                }
                d_usedBy->sortByColumn(0);
                EbnfEditor::NodeList l;
                foreach( const EbnfSyntax::NodeRef& r, d->d_usedBy )
                    l << r.d_node;
                d_edit->markNonTerms( l );
            }else
                d_edit->clearNonTerms();
#endif
        }else
            d_edit->clearNonTerms();
    }
}

void MainWindow::onSave()
{
    ENABLED_IF( d_edit->isModified() );

    if( !d_edit->getPath().isEmpty() )
        d_edit->saveToFile( d_edit->getPath() );
    else
        onSaveAs();
}

void MainWindow::onSaveAs()
{
    ENABLED_IF(true);

    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), QString(),
                                                          tr("*.ebnf") );

    if (fileName.isEmpty())
        return;

    d_edit->saveToFile(fileName);
    onCaption();
}

void MainWindow::onOpen()
{
    ENABLED_IF( true );

    if( !checkSaved( tr("New File")) )
        return;

    const QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                                                          tr("*.ebnf") );
    if (fileName.isEmpty())
        return;

    d_edit->loadFromFile(fileName);
    onCaption();
}

void MainWindow::onNew()
{
    ENABLED_IF(true);

    if( !checkSaved( tr("New File")) )
        return;

    d_edit->newFile();
    onCaption();
}

void MainWindow::onTransformHtml()
{
    ENABLED_IF(true);

    const QString path = QFileDialog::getOpenFileName( this, tr("Transform HTML Syntax"), QString(), "*.html" );
    if( path.isEmpty() )
        return;

    QFileInfo info(path);

    HtmlSyntax s;
    s.transform( path, info.absoluteDir().absoluteFilePath( info.completeBaseName() + ".ebnf" ) );

}

void MainWindow::onSyntaxUpdated()
{
    d_mdl->setSyntax( d_edit->getSyntax() );
}

void MainWindow::onTreeDblClicked()
{
    const EbnfSyntax::Symbol* sym = d_mdl->getSymbol( d_mdl->getParent()->currentIndex() );
    if( sym && sym->d_tok.d_type != EbnfToken::Invalid )
    {
        const bool blocked = d_edit->blockSignals(true);
        d_edit->setCursorPosition( sym->d_tok.d_lineNr - 1,
                                   sym->d_tok.d_colNr - 1, true );
        d_edit->blockSignals(blocked);
        onCursorChanged();
        d_edit->setFocus();
    }
}

static void expandSel( QTreeView* t, const QModelIndex& index )
{
    if( !index.isValid() )
        return;
    t->setExpanded(index, true);
    for( int i = 0; i < t->model()->rowCount(index); i++ )
        expandSel( t, t->model()->index(i,0,index) );
}

void MainWindow::onExpandSelected()
{
    ENABLED_IF(true);
    expandSel( d_mdl->getParent(), d_mdl->getParent()->currentIndex() );
}

void MainWindow::onGenSynTree()
{
    ENABLED_IF( !d_edit->getPath().isEmpty() );

    SynTreeGen::generateTree( d_edit->getPath(), d_edit->getSyntax() );
//    QSet<QByteArray> res = EbnfAnalyzer::collectAllTerminalStrings(d_edit->getSyntax());
//    for( QSet<QByteArray>::const_iterator i = res.begin(); i != res.end(); ++i )
    //        qDebug() << (*i) << SynTreeGen::symToString((*i));
}

void MainWindow::onGenTt()
{
    ENABLED_IF( !d_edit->getPath().isEmpty() );

    SynTreeGen::generateTt( d_edit->getPath(), d_edit->getSyntax(), "Vl", true );
}

void MainWindow::onGenHtml()
{
    ENABLED_IF( !d_edit->getPath().isEmpty() );

    HtmlSyntax gen;
    gen.generateHtml( d_edit->getPath(), d_edit->getSyntax() );
}

void MainWindow::onGenCoco()
{
    ENABLED_IF( !d_edit->getPath().isEmpty() );

    CocoGen gen;
    QFileInfo info(d_edit->getPath());
    gen.generate( info.absoluteDir().absoluteFilePath( info.completeBaseName() + ".atg"),
                  d_edit->getSyntax(), d_tbl, true );
    SynTreeGen::generateTt( d_edit->getPath(), d_edit->getSyntax(), "Vl", false );
    SynTreeGen::generateTree( d_edit->getPath(), d_edit->getSyntax(), "Vl", true );
}

void MainWindow::onGenAntlr()
{
    ENABLED_IF( !d_edit->getPath().isEmpty() );
    QFileInfo info(d_edit->getPath());
    AntlrGen::generate( info.absoluteDir().absoluteFilePath( info.completeBaseName() + ".g"), d_edit->getSyntax() );
}

void MainWindow::onGenLlgen()
{
    ENABLED_IF( !d_edit->getPath().isEmpty() );
    QFileInfo info(d_edit->getPath());
    LlgenGen::generate( info.absoluteDir().absoluteFilePath( info.completeBaseName() + ".g"), d_edit->getSyntax(), d_tbl );
}

void MainWindow::onOutputFirstSet()
{
    ENABLED_IF( !d_edit->getPath().isEmpty() );

    QFileInfo info(d_edit->getPath());
    QFile f( info.absoluteDir().absoluteFilePath( info.completeBaseName() + ".first") );
    f.open(QIODevice::WriteOnly);
    QTextStream out(&f);

    QElapsedTimer t;
    t.start();
    d_tbl->setSyntax( d_edit->getSyntax() );
    for( int i = 0; i < d_edit->getSyntax()->getOrderedDefs().size(); i++ )
    {
        const EbnfSyntax::Definition* d = d_edit->getSyntax()->getOrderedDefs()[i];
        if( d->doIgnore() || ( i != 0 && d->d_usedBy.isEmpty() ) )
            continue;
        if( d->d_node == 0 )
            continue;

        out << GenUtils::escapeDollars(d->d_tok.d_val) << endl;
//        out << "first: ";
//        EbnfSyntax::NodeSet ns = d_tbl->getFirstSet(1, d );
        out << "follow: ";
        EbnfSyntax::NodeRefSet ns = d_tbl->getFollowSet( d );
        QByteArrayList l;
        for( EbnfSyntax::NodeRefSet::const_iterator j = ns.begin(); j != ns.end(); ++j )
            l << GenUtils::symToString( (*j).d_node->d_tok.d_val );
        qSort(l);
        foreach( const QByteArray& a, l )
            out << a << " ";
        out << endl << endl;
    }
    qDebug() << "onOutputFirstSet" << t.elapsed() << "ms";
}

void MainWindow::onUsedByDblClicked()
{
    QTreeWidgetItem* item = d_usedBy->currentItem();
    if( item )
    {
        const bool blocked = d_edit->blockSignals(true);
        const QPoint p = item->data(0,Qt::UserRole).toPoint();
        d_edit->setCursorPosition( p.y() - 1, p.x() - 1, true );
        d_edit->blockSignals(blocked);
        onCursorChanged();
        d_edit->setFocus();
    }
}

void MainWindow::onFindAmbig()
{
    ENABLED_IF(true);

    EbnfAnalyzer a;
    d_tbl->setSyntax(d_edit->getSyntax());
    a.checkForAmbiguity( d_tbl, d_edit->getErrs() );
    d_edit->updateExtraSelections();
}

void MainWindow::onReloadKeywords()
{
    ENABLED_IF(!d_edit->getPath().isEmpty());

    d_edit->loadKeywords(d_edit->getPath());
}

void MainWindow::onAbout()
{
    ENABLED_IF(true);

    QMessageBox::about( this, qApp->applicationName(),
      tr("<html>Release: %1   Date: %2<br><br>"

      "EbnfStudio can be used to edit and analyze EBNF files.<br>"
      "See <a href=\"https://github.com/rochus-keller/EbnfStudio\">"
         "here</a> for more information.<br><br>"

      "Author: Rochus Keller, me@rochus-keller.info<br><br>"

      "Terms of use:<br>"
      "The software and documentation are provided <em>as is</em>, without warranty of any kind, "
      "expressed or implied, including but not limited to the warranties of merchantability, "
      "fitness for a particular purpose and noninfringement. In no event shall the author or copyright holders be "
      "liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, "
      "arising from, out of or in connection with the software or the use or other dealings in the software.</html>" ).
                        arg( qApp->applicationVersion() ).arg( QDateTime::currentDateTime().toString("yyyy-MM-dd") ));
}

void MainWindow::onDetailsDblClicked()
{
    QTreeWidgetItem* item = d_errDetails->currentItem();
    if( item )
    {
        const bool blocked = d_edit->blockSignals(true);
        QPoint p = item->data(0,Qt::UserRole).toPoint();
        d_edit->setCursorPosition( p.y() - 1, p.x() - 1, true );
        d_edit->blockSignals(blocked);
        onCursorChanged();
        d_edit->setFocus();
    }
}

void MainWindow::onLink(const QString& link)
{
    QStringList l = link.split(QChar(':') );
    Q_ASSERT(l.size() == 2);
    const bool blocked = d_edit->blockSignals(true);
    d_edit->setCursorPosition( l.first().toInt() - 1, l.last().toInt() - 1, true );
    d_edit->blockSignals(blocked);
    onCursorChanged();
    d_edit->setFocus();
}

void MainWindow::createMenus()
{
    Gui2::AutoMenu* file = new Gui2::AutoMenu( tr("File"), this, true );
    file->addCommand( "New", this, SLOT(onNew()), tr("CTRL+N"), true );
    file->addCommand( "Open...", this, SLOT(onOpen()), tr("CTRL+O"), true );
    file->addCommand( "Save", this, SLOT(onSave()), tr("CTRL+S"), true );
    file->addCommand( "Save as...", this, SLOT(onSaveAs()) );
    file->addSeparator();
    file->addCommand( "Reload Keywords", this, SLOT(onReloadKeywords()) );
    file->addSeparator();
    file->addCommand( "Print...", d_edit, SLOT(handlePrint()), tr("CTRL+P"), true );
    file->addCommand( "Export PDF...", d_edit, SLOT(handleExportPdf()), tr("CTRL+SHIFT+P"), true );
    file->addSeparator();
    file->addAction(tr("Quit"), this, SLOT(close()), tr("CTRL+Q") );

    Gui2::AutoMenu* edit = new Gui2::AutoMenu( tr("Edit"), this, true );
    edit->addCommand( "Undo", d_edit, SLOT(handleEditUndo()), tr("CTRL+Z"), true );
    edit->addCommand( "Redo", d_edit, SLOT(handleEditRedo()), tr("CTRL+Y"), true );
    edit->addSeparator();
    edit->addCommand( "Cut", d_edit, SLOT(handleEditCut()), tr("CTRL+X"), true );
    edit->addCommand( "Copy", d_edit, SLOT(handleEditCopy()), tr("CTRL+C"), true );
    edit->addCommand( "Paste", d_edit, SLOT(handleEditPaste()), tr("CTRL+V"), true );
    edit->addCommand( "Select all", d_edit, SLOT(handleEditSelectAll()), tr("CTRL+A"), true  );
    //edit->addCommand( "Select Matching Brace", d_edit, SLOT(handleSelectBrace()) );
    edit->addSeparator();
    edit->addCommand( "Find...", d_edit, SLOT(handleFind()), tr("CTRL+F"), true );
    edit->addCommand( "Find again", d_edit, SLOT(handleFindAgain()), tr("F3"), true );
    edit->addCommand( "Replace...", d_edit, SLOT(handleReplace()), tr("CTRL+R"), true );
    edit->addSeparator();
    edit->addCommand( "&Goto...", d_edit, SLOT(handleGoto()), tr("CTRL+G"), true );
    edit->addCommand( "Go Back", d_edit, SLOT(handleGoBack()), tr("ALT+Left"), true );
    edit->addCommand( "Go Forward", d_edit, SLOT(handleGoForward()), tr("ALT+Right"), true );
    edit->addSeparator();
    edit->addCommand( "Indent", d_edit, SLOT(handleIndent()) );
    edit->addCommand( "Unindent", d_edit, SLOT(handleUnindent()) );
    edit->addCommand( "Fix Indents", d_edit, SLOT(handleFixIndent()) );
    edit->addCommand( "Set Indentation Level...", d_edit, SLOT(handleSetIndent()) );

    Gui2::AutoMenu* analyze = new Gui2::AutoMenu( tr("Analyze"), this, true );
    //analyze->addCommand( "Calculate First Set", this, SLOT(onOutputFirstSet()) );
    analyze->addCommand( "Find ambiguities", this, SLOT(onFindAmbig() ), tr("CTRL+SHIFT+A"), true );

    Gui2::AutoMenu* generate = new Gui2::AutoMenu( tr("Generate"), this, true );
    generate->addCommand( "Generate SynTree", this, SLOT(onGenSynTree()) );
    generate->addCommand( "Generate TokenTypes", this, SLOT(onGenTt()) );
    generate->addCommand( "Generate Html", this, SLOT(onGenHtml()) );
    generate->addCommand( "Generate Coco/R", this, SLOT(onGenCoco()), tr("CTRL+SHIFT+R"), true );
    generate->addCommand( "Generate ANTLR", this, SLOT(onGenAntlr()) );
    generate->addCommand( "Generate LLgen", this, SLOT(onGenLlgen()) );

    Gui2::AutoMenu* tools = new Gui2::AutoMenu( tr("Tools"), this, true );
    tools->addCommand( "Transform HTML Syntax...", this, SLOT(onTransformHtml()) );

    Gui2::AutoMenu* window = new Gui2::AutoMenu( tr("Window"), this, true );
    window->addCommand( "Set &Font...", d_edit, SLOT(handleSetFont()) );
    window->addCommand( "Show &Linenumbers", d_edit, SLOT(handleShowLinenumbers()) );

    Gui2::AutoMenu* help = new Gui2::AutoMenu( tr("Help"), this, true );
    help->addCommand( "&About...", this, SLOT(onAbout()) );
}

void MainWindow::createIssues()
{
    QDockWidget* dock = new QDockWidget( tr("Issues"), this );
    dock->setObjectName("Issues");
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
    d_errView = new QTreeWidget(dock);
    d_errView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Preferred);
    d_errView->setAlternatingRowColors(true);
    d_errView->setHeaderHidden(true);
    d_errView->setSortingEnabled(false);
    d_errView->setAllColumnsShowFocus(true);
    d_errView->setRootIsDecorated(false);
    d_errView->setColumnCount(2);
    d_errView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    d_errView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    dock->setWidget(d_errView);
    addDockWidget( Qt::BottomDockWidgetArea, dock );
    connect(d_errView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onErrorsDblClicked()) );
    connect( new QShortcut( tr("ESC"), this ), SIGNAL(activated()), dock, SLOT(hide()) );
}

void MainWindow::createTree()
{
    QDockWidget* dock = new QDockWidget( tr("Syntax Tree"), this );
    dock->setObjectName("SyntaxTree");
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
    QTreeView* tree = new QTreeView(dock);
    d_mdl = new SyntaxTreeMdl(tree);
    tree->setAlternatingRowColors(true);
    tree->setExpandsOnDoubleClick(false);
    tree->setHeaderHidden(true);
    tree->setSortingEnabled(false);
    tree->setAllColumnsShowFocus(true);
    tree->setRootIsDecorated(true);
    tree->setModel(d_mdl);
    dock->setWidget(tree);
    addDockWidget( Qt::LeftDockWidgetArea, dock );
    connect( d_edit, SIGNAL(sigSyntaxUpdated()), this, SLOT(onSyntaxUpdated()) );
    connect( tree, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onTreeDblClicked()) );
    Gui2::AutoMenu* pop = new Gui2::AutoMenu( tree, true );
    pop->addAction(tr("Expand all"), tree, SLOT(expandAll()) );
    pop->addCommand(tr("Expand current"), this, SLOT(onExpandSelected()), tr("CTRL+E"), true );
    pop->addAction(tr("Collapse all"), tree, SLOT(collapseAll()) );

}

void MainWindow::createDetails()
{
    QDockWidget* dock = new QDockWidget( tr("Issue Details"), this );
    dock->setObjectName("IssueDetails");
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
    QWidget* pane = new QWidget(dock);
    QVBoxLayout* vbox = new QVBoxLayout(pane);
    vbox->setMargin(0);
    d_errText = new QLabel(dock);
    d_errText->setWordWrap(true);
    d_errText->setTextInteractionFlags(Qt::TextBrowserInteraction);
    d_errText->setFrameShape(QFrame::Box);
    connect( d_errText, SIGNAL(linkActivated(QString)), this, SLOT(onLink(QString)) );
    vbox->addWidget(d_errText);
    d_errDetails = new QTreeWidget(dock);
    d_errDetails->setAlternatingRowColors(true);
    d_errDetails->setHeaderHidden(true);
    d_errDetails->setSortingEnabled(false);
    d_errDetails->setAllColumnsShowFocus(true);
    d_errDetails->setRootIsDecorated(false);
    d_errDetails->setColumnCount(2);
    d_errDetails->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    d_errDetails->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    vbox->addWidget(d_errDetails);
    dock->setWidget(pane);
    addDockWidget( Qt::BottomDockWidgetArea, dock );
    connect(d_errDetails, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onDetailsDblClicked()) );
}

void MainWindow::createUsedBy()
{
    QDockWidget* dock = new QDockWidget( tr("Used By"), this );
    dock->setObjectName("UsedBy");
    dock->setAllowedAreas( Qt::AllDockWidgetAreas );
    dock->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable );
    d_usedBy = new QTreeWidget(dock);
    d_usedBy->setAlternatingRowColors(true);
    d_usedBy->setHeaderHidden(true);
    d_usedBy->setSortingEnabled(true);
    d_usedBy->setAllColumnsShowFocus(true);
    d_usedBy->setRootIsDecorated(false);
    dock->setWidget(d_usedBy);
    addDockWidget( Qt::LeftDockWidgetArea, dock );
    connect(d_usedBy, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onUsedByDblClicked()) );
}

bool MainWindow::checkSaved(const QString& title)
{
    if( d_edit->isModified() )
    {
        switch( QMessageBox::critical( this, title, tr("The file has not been saved; do you want to save it?"),
                               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes ) )
        {
        case QMessageBox::Yes:
            if( !d_edit->getPath().isEmpty() )
                return d_edit->saveToFile(d_edit->getPath());
            else
            {
                const QString path = QFileDialog::getSaveFileName( this, title, QString(), "*.ebnf" );
                if( path.isEmpty() )
                    return false;
                return d_edit->saveToFile(path);
            }
            break;
        case QMessageBox::No:
            return true;
        default:
            return false;
        }
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QSettings s;
    s.setValue( "DockState", saveState() );
    event->setAccepted(checkSaved( tr("Quit Application")));
}
