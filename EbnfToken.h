#ifndef EBNFTOKEN_H
#define EBNFTOKEN_H

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
#include <QHash>

struct EbnfToken
{
    class Sym
    {
    public:
        Sym(const Sym& rhs ):d_str(rhs.d_str){}
        Sym():d_str(0){}

        operator QByteArray() const { return toBa(); }
        QByteArray toBa() const;
        QString toStr() const;
        operator const char*() const { return c_str(); }
        const char* c_str() const;
        const char* data() const { return d_str; }
        int size() const;
        bool isEmpty() const;
        bool operator==(const Sym& rhs) const { return d_str == rhs.d_str; }
    private:
        friend class EbnfToken;
        const char* d_str;
    };

    enum TokenType { Invalid, Production, Assig, NonTerm, Keyword, Literal,
                     Bar, LPar, RPar, LBrack, RBrack, LBrace, RBrace, Predicate, Comment,
                     AddTo, // +=
                     PpDefine, PpUndef, PpIfdef, PpIfndef, PpElse, PpEndif, Eof };
    enum Handling { Normal,
                    Transparent, // *
                    Keep,        // !
                    Skip         // -
                    };

#ifdef _DEBUG
    TokenType d_type;
    Handling d_op;
#else
    uint d_type : 5;
    uint d_op : 2;
#endif
    uint d_len : 10;
    uint d_colNr : 15;
    quint32 d_lineNr;
    Sym d_val;
    EbnfToken(TokenType t = Invalid, quint32 line = 0,quint16 col = 0, quint16 len = 0, const QByteArray& val = QByteArray() ):
        d_type(t),d_lineNr(line),d_colNr(col),d_len(len),d_op(Normal){ d_val = getSym(val);}
    QByteArray toString(bool labeled = true) const;
    bool isValid() const { return d_type != Eof && d_type != Invalid; }
    bool isErr() const { return d_type == Invalid; }


    static Sym getSym( const QByteArray& );
    static void resetSymTbl();

    static bool isPpType( TokenType );

private:
    static QHash<QByteArray,Sym> s_symTbl;
};

inline uint qHash(const EbnfToken::Sym& r ) { return qHash(r.data()); }


#endif // EBNFTOKEN_H
