#ifndef SYNTREEGEN_H
#define SYNTREEGEN_H

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

#include <QString>
#include <QPair>

class EbnfSyntax;

class SynTreeGen
{
public:
    typedef QList< QPair<QByteArray,QByteArray> > TokenNameValueList; // Value empty -> Title
    static TokenNameValueList generateTokenList(EbnfSyntax* , int* startOfSpecial = 0);
    static bool generateTree( const QString& ebnfPath, EbnfSyntax*, bool includeNt = true );
    static bool generateTt(const QString& ebnfPath, EbnfSyntax*, bool includeLex = true, bool includeNt = false );
private:
    SynTreeGen();
};

#endif // SYNTREEGEN_H
