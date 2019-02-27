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
#include <QApplication>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("Rochus Keller");
    a.setOrganizationDomain("github.com/rochus-keller/EbnfStudio");
    a.setApplicationName("EbnfStudio");
    a.setApplicationVersion("0.6");

    QIcon icon;
    icon.addFile( ":/images/icon_16.png" );
    icon.addFile( ":/images/icon_32.png" );
    icon.addFile( ":/images/icon_64.png" );
    icon.addFile( ":/images/icon_128.png" );
    icon.addFile( ":/images/icon_256.png" );
    a.setWindowIcon( icon );

    a.setStyle("Fusion");
    MainWindow w;

    QString path;
    QStringList args = a.arguments();
    for( int i = 1; i < args.size(); i++ ) // arg 0 enthält Anwendungspfad
    {
        QString arg = args[ i ];
        if( arg[ 0 ] != '-' )
        {
            QFileInfo info( arg );
            const QByteArray suffix = info.suffix().toLower().toUtf8();

            if( suffix == "ebnf" )
            {
                path = arg;
            }
        }
    }

    if( !path.isEmpty() )
        w.open(path);

    return a.exec();
}
