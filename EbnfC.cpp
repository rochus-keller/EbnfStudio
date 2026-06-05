/*
* Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>
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

#include "CppGen.h"
#include "EbnfAnalyzer.h"
#include "EbnfAnalyzer2.h"
#include "EbnfErrors.h"
#include "EbnfLexer.h"
#include "EbnfParser.h"
#include "EbnfToken.h"
#include "EbnfVersion.h"
#include "FirstFollowSet.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QtDebug>
#include <QElapsedTimer>

static void printErrors( const EbnfErrors& errs )
{
    foreach( const EbnfErrors::Entry& e, errs.getErrors())
    {
        if( e.d_isErr )
            qCritical() << "ERR" << e.d_line << ":" << e.d_col << ":" << e.d_msg.toUtf8().constData();
        else
            qWarning() << "WRN" << e.d_line << ":" << e.d_col << ":" << e.d_msg.toUtf8().constData();
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("Rochus Keller");
    a.setOrganizationDomain("github.com/rochus-keller/EbnfStudio");
    a.setApplicationName("ebnfc");
    a.setApplicationVersion(EBNF_VERSION);


    QString path;
    bool useAnalyzer2 = false;
    bool compareBoth = false;
    bool doGenerate = false;
    QStringList args = a.arguments();
    for( int i = 1; i < args.size(); i++ ) // arg 0 enthält Anwendungspfad
    {
        QString arg = args[ i ];
        if( arg == "-e" || arg == "--exact" )
        {
            useAnalyzer2 = true;
        }else if( arg == "-cmp" || arg == "--compare" )
        {
            compareBoth = true;
        }else if( arg == "-gen" || arg == "--generate" )
        {
            doGenerate = true;
        }else if( arg[ 0 ] != '-' )
        {
            QFileInfo info( arg );
            const QByteArray suffix = info.suffix().toLower().toUtf8();

            if( suffix == "ebnf" )
            {
                path = arg;
            }
        }
    }

    if( path.isEmpty() )
    {
        qCritical() << "expecting an EBNF file path";
        qCritical() << "usage: ebnfc [options] <file.ebnf>";
        qCritical() << "  -e,   --exact      use the exact LL(k) analyzer (EbnfAnalyzer2)";
        qCritical() << "  -cmp, --compare    run both analyzers and compare results";
        qCritical() << "  -gen, --generate   generate C++ parser code (uses exact sequences with -e)";
        return 1;
    }

    EbnfErrors errs;

    QFile file(path);
    if( !file.open(QIODevice::ReadOnly ) )
        return false;

    EbnfToken::resetSymTbl();

    EbnfLexer lex;
    QFileInfo info(path);
    lex.readKeywordsFromFile( info.absoluteDir().absoluteFilePath( info.completeBaseName() + ".keywords" ) );

    EbnfParser p;
    p.setErrors(&errs);
    lex.setStream( &file );
    if( !p.parse( &lex ) )
    {
        printErrors(errs);
        return 1;
    }else
    {
        EbnfSyntaxRef syn(p.getSyntax());
        if( !syn->finishSyntax() )
        {
            printErrors(errs);
            return 1;
        }

        FirstFollowSet tbl;
        tbl.setSyntax(syn.data());

        if( compareBoth )
        {
            EbnfErrors errs1;
            EbnfErrors errs2;
            QElapsedTimer t;

            qDebug() << "*** Running original EbnfAnalyzer:";
            t.start();
            EbnfAnalyzer::checkForAmbiguity( &tbl, &errs1 );
            qDebug() << "   " << t.elapsed() << "ms";

            qDebug() << "*** Running exact EbnfAnalyzer2:";
            t.start();
            EbnfAnalyzer2::checkForAmbiguity( &tbl, &errs2 );
            qDebug() << "   " << t.elapsed() << "ms";

            qDebug() << "";
            qDebug() << "*** EbnfAnalyzer results:" << errs1.getErrors().size() << "issues";
            printErrors(errs1);

            qDebug() << "";
            qDebug() << "*** EbnfAnalyzer2 results:" << errs2.getErrors().size() << "issues";
            printErrors(errs2);

            qDebug() << "";
            if( errs1.getErrors().size() != errs2.getErrors().size() )
            {
                qDebug() << "*** Difference detected:"
                         << errs1.getErrors().size() << "vs" << errs2.getErrors().size() << "issues ***";
            }else
                qDebug() << "*** Both analyzers report the same number of issues.";

            return ( errs1.getErrors().isEmpty() && errs2.getErrors().isEmpty() ) ? 0 : 1;
        }else if( useAnalyzer2 )
            EbnfAnalyzer2::checkForAmbiguity( &tbl, &errs );
        else
            EbnfAnalyzer::checkForAmbiguity( &tbl, &errs );

        if( doGenerate )
        {
            CppGen gen;
            gen.d_exact = useAnalyzer2;
            gen.generate(path, syn.data(), &tbl);
        }

        if( !errs.getErrors().isEmpty() )
        {
            printErrors(errs);
            return 1;
        }
    }

    return 0;
}
