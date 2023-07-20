#ifndef GENUTIL_H
#define GENUTIL_H

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
#include <QSet>

class GenUtils
{
public:
    typedef QHash<QString,QString> TokMap;
    static TokMap s_tokMap;
    static QString escapeDollars(QString name );
    static bool containsAlnum( const QString& str );
    static bool looksLikeKeyword( const QString& str );
    static QString symToString(const QString& sym );
    static QString charToString(QChar );
    static QStringList orderedTokenList(const QSet<QString>& tokens, bool applySymToString = true );
private:
    GenUtils();
};

#endif // GENUTIL_H
