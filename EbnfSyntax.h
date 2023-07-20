#ifndef EBNF_SYNTAX_H
#define EBNF_SYNTAX_H

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

#include <QSharedData>
#include <QHash>
#include <QStringList>
#include <QSet>
#include <QVariant>
#include "EbnfToken.h"

class EbnfErrors;

namespace Ast
{
    struct Node;
    typedef QList<Node*> NodeList;
    typedef QList<const Node*> ConstNodeList;
    typedef QSet<const Node*> NodeSet;

    struct Symbol
    {
        EbnfToken d_tok;
        Symbol( const EbnfToken& tok = EbnfToken() ):d_tok(tok) {}
        virtual ~Symbol() {}
        virtual bool doIgnore() const { return false; }
        virtual bool isNullable() const { return false; }
        virtual bool isRepeatable() const { return false; }
        virtual bool isAnyReachable() const { return true; }
    };

    struct Definition : public Symbol
    {
        Node* d_node;
        QSet<Node*> d_usedBy; // TODO: könnte auch List sein!
        bool d_nullable;
        bool d_repeatable;
        bool d_directLeftRecursive;
        bool d_indirectLeftRecursive;
        bool d_notReachable;
        Definition(const EbnfToken& tok):Symbol(tok),d_node(0),d_nullable(false),d_repeatable(false),
            d_directLeftRecursive(false),d_indirectLeftRecursive(false),d_notReachable(false){}
        ~Definition();
        bool doIgnore() const;
        bool isNullable() const { return d_nullable; }
        bool isRepeatable() const { return d_repeatable; }
        void dump() const;
    };

    struct Node : public Symbol
    {
        enum Type { Terminal, Nonterminal, Sequence, Alternative, Predicate };
        enum Quantity { One, ZeroOrOne, ZeroOrMore };
        static const char* s_typeName[];
    #ifdef _DEBUG
        Type d_type;
        Quantity d_quant;
    #else
        quint8 d_type;
        quint8 d_quant;
    #endif
        bool d_leftRecursive;
        bool d_literal;
        NodeList d_subs; // owned
        NodeList d_pathToDef;
        Definition* d_owner;
        Definition* d_def; // resolved nonterminal
        Node* d_parent; // TODO: ev. unnötig; man kann damit bottom up über Sequence hinweg schauen
        Node(Type t, Definition* d, const EbnfToken& tok = EbnfToken(), bool lit = false):Symbol(tok),d_type(t),
            d_quant(One),d_owner(d),d_def(0),d_parent(0),d_leftRecursive(false),d_literal(lit){}
        Node(Type t, Node* parent, const EbnfToken& tok = EbnfToken()):Symbol(tok),d_type(t),
            d_quant(One),d_owner(parent->d_owner),d_def(0),d_parent(parent),d_leftRecursive(false){ parent->d_subs.append(this); }
        ~Node();
        bool doIgnore() const;
        bool isNullable() const;
        bool isRepeatable() const;
        bool isAnyReachable() const;
        const Node* getNext(int* index = 0) const;
        int getLlk() const; // 0..invalid
        int getLl() const; // 0..invalid
        QByteArray getLa() const;
        void dump(int level = 0) const;
        QString toString() const;
    };
    typedef QSet<Definition*> DefSet;

    struct NodeRef
    {
        const Node* d_node;
        NodeRef( const Node* node = 0 ):d_node(node){}
        bool operator==( const NodeRef& ) const;
        const Node* operator*() const { return d_node; }
    };
    typedef QSet<NodeRef> NodeRefSet;
    typedef QList<NodeRef> NodeRefList;

    struct NodeTree
    {
        const Node* d_node;
        quint8 d_k;
        QList<NodeTree*> d_leafs;
        NodeTree(quint8 k = 0, const Node* n=0):d_node(n),d_k(k){}
        ~NodeTree()
        {
            foreach( NodeTree* l, d_leafs )
                delete l;
        }
        void addLeaf(const Node* n)
        {
            foreach( NodeTree* l, d_leafs )
            {
                if( l->d_node == n )
                    return;
            }
            NodeTree* nt = new NodeTree(d_k+1, n);
            d_leafs.append(nt);
        }
    };

    // NOTE: weil alle Symbole denselben String verwenden auf Adresse als Hash wechseln
    inline uint qHash(const NodeRef& r ) { return r.d_node ? qHash(r.d_node->d_tok.d_val) : 0; }
}

class EbnfSyntax : public QSharedData
{
public:
    typedef QList<qint32> IfDefOutList; // Jeder Eintrag ist die Zeile der Änderung. Start bei On.
    typedef QSet<EbnfToken::Sym> Defines;
    typedef QSet<EbnfToken::Sym> Keywords;

    struct IssueData
    {
        enum Type { None, AmbigAlt, AmbigOpt, BadPred, LeftRec, DetailItem } d_type;
        const Ast::Node* d_ref;
        const Ast::Node* d_other;
        Ast::ConstNodeList d_list;
        IssueData(Type t = None, const Ast::Node* r = 0, const Ast::Node* oth = 0, const Ast::ConstNodeList& l = Ast::ConstNodeList()):
            d_type(t), d_ref(r),d_other(oth),d_list(l){}
    };

    explicit EbnfSyntax(EbnfErrors* errs = 0);
    ~EbnfSyntax();

    void clear();
    EbnfErrors* getErrs() const { return d_errs; }

    typedef QHash<EbnfToken::Sym,Ast::Definition*> Definitions;
    typedef QList<Ast::Definition*> OrderedDefs;
    typedef QList<EbnfToken::Sym> SymList;

    bool addDef( Ast::Definition* ); // transfer ownership
    bool addPragma(const EbnfToken& name, Ast::Node* ); // transfer ownership
    const Definitions& getDefs() const { return d_defs; }
    const Ast::Definition* getDef(const EbnfToken::Sym& name ) const;
    const OrderedDefs& getOrderedDefs() const { return d_order; }
    const Definitions& getPragmas() const { return d_pragmas; }
    SymList getPragma(const QByteArray& name ) const;
    EbnfToken::Sym getPragmaFirst(const QByteArray& name ) const;
    void addIdol(qint32);
    const IfDefOutList& getIdol() const { return d_idol; }
    void setDefines( const Defines& d ) { d_defines = d; }
    const Defines& getDefines() const { return d_defines; }
    void setKeywords( const Keywords& kw ) { d_kw = kw; }
    const Keywords& getKeywords() const { return d_kw; }

    bool finishSyntax();

    const Ast::Symbol* findSymbolBySourcePos( quint32 line, quint16 col , bool nonTermOnly = true ) const;
    Ast::ConstNodeList getBackRefs( const Ast::Symbol* ) const;
    static const Ast::Node* firstVisibleElementOf( const Ast::Node* );
    static const Ast::Node* firstPredicateOf( const Ast::Node* );

    static QString pretty( const Ast::NodeRefSet& );
    static QString pretty( const Ast::NodeSet& );
    static Ast::NodeRefSet nodeToRefSet( const Ast::NodeSet& );
    static Ast::NodeSet collectNodes( const Ast::NodeRefSet& pattern, const Ast::NodeSet& set );

    void dump() const;
protected:
    bool resolveAllSymbols();
    void calculateNullable();
    void checkReachability();
    bool resolveAllSymbols( Ast::Node *node );
    const Ast::Symbol* findSymbolBySourcePosImp( const Ast::Node*, quint32 line, quint16 col, bool nonTermOnly ) const;
    void calcLeftRecursion();
    void markLeftRecursion( Ast::Definition*,Ast::Node* node, Ast::NodeList& );
    void checkPragmas();
    Ast::NodeRefSet calcStartsWithNtSet( Ast::Node* node );
    void checkPredicates();
    void checkPredicates(Ast::Node* node);

private:
    Q_DISABLE_COPY(EbnfSyntax)
    EbnfErrors* d_errs;
    Definitions d_defs;
    OrderedDefs d_order;
    Definitions d_pragmas;
    Defines d_defines;
    typedef QHash<EbnfToken::Sym,Ast::ConstNodeList> BackRefs;
    BackRefs d_backRefs;
    IfDefOutList d_idol;
    Keywords d_kw;
    bool d_finished;
};

typedef QExplicitlySharedDataPointer<EbnfSyntax> EbnfSyntaxRef;


Q_DECLARE_METATYPE(EbnfSyntax::IssueData)

#endif // EBNF_SYNTAX_H
