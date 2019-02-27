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

#include "EbnfErrors.h"
#include <QtDebug>

EbnfErrors::EbnfErrors(QObject *parent) : QObject(parent),d_reportToConsole(false),d_errCounter(0)
{
    d_eventLatency.setSingleShot(true);
    connect(&d_eventLatency, SIGNAL(timeout()), this, SIGNAL(sigChanged()));
}

void EbnfErrors::error(EbnfErrors::Source src, int line, int col, const QString& msg, const QVariant& data)
{
    bool inserted = true;
    if( true ) // d_record )
    {
        Entry e;
        e.d_col = col;
        e.d_line = line;
        e.d_msg = msg;
        e.d_source = src;
        e.d_isErr = true;
        e.d_data = data;
        const int count = d_errs.size();
        d_errs.insert(e);
        inserted = count != d_errs.size();
        if( inserted )
        {
            notify();
            d_errCounter++;
        }
    }
    if( d_reportToConsole && inserted )
    {
        qCritical() << line << ":" << col << ": error:" << msg;
    }
}

void EbnfErrors::warning(EbnfErrors::Source src, int line, int col, const QString& msg, const QVariant& data)
{
    bool inserted = true;
    if( true ) // d_showWarnings )
    {
        if( true ) // d_record )
        {
            Entry e;
            e.d_col = col;
            e.d_line = line;
            e.d_msg = msg;
            e.d_source = src;
            e.d_isErr = false;
            e.d_data = data;
            const int count = d_errs.size();
            d_errs.insert(e);
            inserted = count != d_errs.size();
            if( inserted )
                notify();
        }
        if( d_reportToConsole && inserted )
            qWarning() << line << ":" << col << ": warning:" << msg;
    }
}

void EbnfErrors::clear()
{
    d_errs.clear();
    d_errCounter = 0;
    notify();
}

void EbnfErrors::notify()
{
    d_eventLatency.start(400);
}

