#ifndef _FirstFollowSet_H
#define _FirstFollowSet_H

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
#include "EbnfSyntax.h"

class FirstFollowSet : public QObject
{
public:
    typedef QHash<const Ast::Node*, Ast::NodeSet> Lookup;

    explicit FirstFollowSet(QObject *parent = 0);

    void setSyntax( EbnfSyntax* );
    void setIncludeNts(bool);
    EbnfSyntax* getSyntax() const { return d_syn.data(); }
    void clear();

    Ast::NodeSet getFirstNodeSet(const Ast::Node* , bool cache = true) const;
    Ast::NodeRefSet getFirstSet(const Ast::Node* , bool cache = true) const;
    Ast::NodeRefSet getFirstSet( const Ast::Definition* ) const;
    Ast::NodeSet getFollowNodeSet( const Ast::Node*) const;
    Ast::NodeRefSet getFollowSet( const Ast::Definition*) const;
    Ast::NodeRefSet getFollowSet( const Ast::Node*) const;
protected:
    Ast::NodeSet calculateFirstSet( const Ast::Node* ) const;
    void calculateFirstSets();
    void calculateFollowSets();
    bool calculateFollowSet2(const Ast::Node* , bool addRepetitions = true);
    bool calculateFollowSet( const Ast::Definition* );
private:
    friend class EbnfAnalyzer;
    Lookup d_first;
    Lookup d_follow;
    EbnfSyntaxRef d_syn;
    bool d_includeNts;
};

#endif // _FirstFollowSet_H
