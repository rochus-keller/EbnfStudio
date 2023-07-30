#ifndef CPPGEN_H
#define CPPGEN_H

/*
* Copyright 2023 Rochus Keller <mailto:me@rochus-keller.ch>
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
#include "EbnfSyntax.h"

class QTextStream;
class FirstFollowSet;

class CppGen
{
public:
    CppGen();
    bool generate(const QString& ebnfPath, EbnfSyntax*, FirstFollowSet*);
protected:
    void writeNode(QTextStream& out, Ast::Node* node, int level);
    void handlePredicate(QTextStream& out, const Ast::Node* pred);
    QList<const Ast::Node*> findFirstsOf(Ast::Node*, bool checkFollowSet = false) const;
    void writeCond( QTextStream& out, bool loop, const QList<const Ast::Node*>& firsts );
private:
    FirstFollowSet* d_tbl;
    EbnfSyntax* d_syn;
    bool d_pseudoKeywords;
    bool d_genSynTree;
};

#endif // CPPGEN_H
