#ifndef EBNFERRORS_H
#define EBNFERRORS_H

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

#include <QObject>
#include <QSet>
#include <QTimer>
#include <QVariant>

class EbnfErrors : public QObject
{
    Q_OBJECT
public:
    enum Source { Syntax, Semantics, Analysis };
    struct Entry
    {
        quint32 d_line;
        quint16 d_col;
        quint8 d_source;
        bool d_isErr;
        QString d_msg;
        QVariant d_data;
        Entry():d_source(0),d_isErr(true){}
        bool operator==( const Entry& rhs )const
        {
            return d_line == rhs.d_line && d_col == rhs.d_col && d_msg == rhs.d_msg &&
                    d_isErr == rhs.d_isErr && d_source == rhs.d_source;
        }
    };
    typedef QSet<Entry> EntryList;

    explicit EbnfErrors(QObject *parent = 0);

    void error( Source, int line, int col, const QString& msg, const QVariant& = QVariant() );
    void warning( Source, int line, int col, const QString& msg, const QVariant& = QVariant() );
    void clear();

    const EntryList& getErrors() const { return d_errs; }
    void resetErrCount() { d_errCounter = 0; }
    quint16 getErrCount() const { return d_errCounter; }

signals:
    void sigChanged();

protected:
    void notify();

private:
    QTimer d_eventLatency;
    EntryList d_errs;
    quint16 d_errCounter;
    bool d_reportToConsole;
};

inline uint qHash(const EbnfErrors::Entry & e, uint seed = 0) {
    return ::qHash(e.d_msg,seed) ^ ::qHash(e.d_line,seed) ^ ::qHash(e.d_col,seed);
}

#endif // EBNFERRORS_H
