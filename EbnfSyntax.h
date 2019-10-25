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

class EbnfSyntax : public QSharedData
{
public:
    struct Node;
    typedef QList<Node*> NodeList;
    typedef QList<const Node*> ConstNodeList;
    typedef QSet<const EbnfSyntax::Node*> NodeSet;
    typedef QList<qint32> IfDefOutList; // Jeder Eintrag ist die Zeile der Änderung. Start bei On.
    typedef QSet<EbnfToken::Sym> Defines;
    typedef QSet<EbnfToken::Sym> Keywords;

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
        ~Definition() { if( d_node ) delete d_node; }
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
        NodeList d_subs; // owned
        NodeList d_pathToDef;
        Definition* d_owner;
        Definition* d_def; // resolved nonterminal
        Node* d_parent; // TODO: ev. unnötig; man kann damit bottom up über Sequence hinweg schauen
        Node(Type t, Definition* d, const EbnfToken& tok = EbnfToken()):Symbol(tok),d_type(t),
            d_quant(One),d_owner(d),d_def(0),d_parent(0),d_leftRecursive(false){}
        Node(Type t, Node* parent, const EbnfToken& tok = EbnfToken()):Symbol(tok),d_type(t),
            d_quant(One),d_owner(parent->d_owner),d_def(0),d_parent(parent),d_leftRecursive(false){ parent->d_subs.append(this); }
        ~Node();
        bool doIgnore() const;
        bool isNullable() const;
        bool isRepeatable() const;
        bool isAnyReachable() const;
        const Node* getNext(int* index = 0) const;
        int getLlk() const; // 0..invalid
        QByteArray getLa() const;
        void dump(int level = 0) const;
        QString toString() const;
    };
    typedef QSet<Definition*> DefSet;

    struct NodeRef
    {
        const EbnfSyntax::Node* d_node;
        NodeRef( const EbnfSyntax::Node* node = 0 ):d_node(node){}
        bool operator==( const NodeRef& ) const;
        const EbnfSyntax::Node* operator*() const { return d_node; }
    };
    typedef QSet<NodeRef> NodeRefSet;
    typedef QList<NodeRef> NodeRefList;

    struct IssueData
    {
        enum Type { None, AmbigAlt, AmbigOpt, BadPred, LeftRec, DetailItem } d_type;
        const Node* d_ref;
        const Node* d_other;
        ConstNodeList d_list;
        IssueData(Type t = None, const Node* r = 0, const Node* oth = 0, const ConstNodeList& l = ConstNodeList()):
            d_type(t), d_ref(r),d_other(oth),d_list(l){}
    };

    explicit EbnfSyntax(EbnfErrors* errs = 0);
    ~EbnfSyntax();

    void clear();
    EbnfErrors* getErrs() const { return d_errs; }

    typedef QHash<EbnfToken::Sym,Definition*> Definitions;
    typedef QList<Definition*> OrderedDefs;
    typedef QList<EbnfToken::Sym> SymList;

    bool addDef( Definition* ); // transfer ownership
    bool addPragma(const EbnfToken& name, Node* ); // transfer ownership
    const Definitions& getDefs() const { return d_defs; }
    const Definition* getDef(const EbnfToken::Sym& name ) const;
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

    const Symbol* findSymbolBySourcePos( quint32 line, quint16 col , bool nonTermOnly = true ) const;
    ConstNodeList getBackRefs( const Symbol* ) const;
    static const Node* firstVisibleElementOf( const Node* );
    static const Node* firstPredicateOf( const Node* );

    static QString pretty( const NodeRefSet& );
    static QString pretty( const NodeSet& );
    static NodeRefSet nodeToRefSet( const NodeSet& );
    static NodeSet collectNodes( const NodeRefSet& pattern, const NodeSet& set );

    void dump() const;
protected:
    bool resolveAllSymbols();
    void calculateNullable();
    void checkReachability();
    bool resolveAllSymbols( Node *node );
    const Symbol* findSymbolBySourcePosImp( const Node*, quint32 line, quint16 col, bool nonTermOnly ) const;
    void calcLeftRecursion();
    void markLeftRecursion( Definition*,Node* node, NodeList& );
    void checkPragmas();
    NodeRefSet calcStartsWithNtSet( Node* node );
    void checkPredicates();
    void checkPredicates(Node* node);

private:
    Q_DISABLE_COPY(EbnfSyntax)
    EbnfErrors* d_errs;
    Definitions d_defs;
    OrderedDefs d_order;
    Definitions d_pragmas;
    Defines d_defines;
    typedef QHash<EbnfToken::Sym,ConstNodeList> BackRefs;
    BackRefs d_backRefs;
    IfDefOutList d_idol;
    Keywords d_kw;
    bool d_finished;
};

typedef QExplicitlySharedDataPointer<EbnfSyntax> EbnfSyntaxRef;
//inline uint qHash(const EbnfSyntax::NodeRef& r ) { return r.d_node ? qHash(r.d_node->d_tok.d_val) : 0; }
// NOTE: weil alle Symbole denselben String verwenden auf Adresse als Hash wechseln
inline uint qHash(const EbnfSyntax::NodeRef& r ) { return r.d_node ? qHash(r.d_node->d_tok.d_val) : 0; }

Q_DECLARE_METATYPE(EbnfSyntax::IssueData)

#endif // EBNF_SYNTAX_H
