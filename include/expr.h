// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef EXPR_H_
#define EXPR_H_

#include <memory>
#include <string>
#include <ostream>
#include <list>
#include <z3++.h>
#include "visitor.h"

namespace se {

// OperatorAttr is an unsigned bit vector whose value is a bitwise combination
// of the enum OperatorAttrBit values. These values describe the mathematical
// properties of an operator. Whenever possible, inspect a variable of this
// type with the appropriate getter functions (e.g. get_associative_attr)
typedef unsigned char OperatorAttr;

// OperatorAttrBit is an enumeration type that defines the bit mask semantics
// of an OperatorAttr variable. Use bitwise operators judiciously to combine
// enum values. Of course, unusual combinations should be avoided. For instance,
// it uncommon to define a non-associative but commutative operator (also known
// as magmas). If both LASSOC_ATTR and RASSOC_ATTR are set, then the operator
// must be associative: ((x ~ y) ~ z) = (x ~ (y ~ z)).
enum OperatorAttrBit : OperatorAttr {
  // Clear all other attributes via bitwise AND.
  CLEAR_ATTR = 0u,

  // Operator is left associative.
  LASSOC_ATTR = (1u << 2),

  // Operator is right associative.
  RASSOC_ATTR = (1u << 1),

  // Operator is commutative, i.e. (x ~ y) = (y ~ x).
  COMM_ATTR = (1u << 3)
};

// get_commutative_attr(const OperatorAttr) returns true if and only if
// the commutative bit in the given attribute bit vector is set.
inline bool get_commutative_attr(const OperatorAttr attr) {
  return attr & COMM_ATTR;
}

// get_associative_attr(const OperatorAttr) returns true if and only if
// the associative bit in the given attribute bit vector is set.
inline bool get_associative_attr(const OperatorAttr attr) {
  return (attr & (LASSOC_ATTR | RASSOC_ATTR)) == (LASSOC_ATTR | RASSOC_ATTR);
}

// Type enumerates built-in primitive types. The order of these types
// determines the order of elements in the internal types array.
enum Type : unsigned short {
  BOOL,
  CHAR,
  INT
};

// types is an internal array that maps a Type to its string corresponding
// string representation. Therefore, both data types need to be coordinated.
static std::string types[] = { "bool", "char", "int" };

// Operator enumerates the built-in C++ operations. These operators are ordered
// according to their arity. This can be used to create fast lookup tables that
// are indexed by the enum value. To facilitate this, there are extern constants
// that mark the first and last n-arity operator in the enum ordering for each
// positive number n. Since some nary operators are also unary (e.g. ADD), the
// enum value of these identifiers overlap. The last unary operator marks the
// beginning of operators that never accept less than two operands (e.g. LSS).
// The following schematic diagram illustrates this:
//
//    +--------------------------------------+ <-- 0 (unsigned short)
//    |               Future Use             |
//    +--------------------------------------+ <-- UNARY_BEGIN
//    |                                      |
//    |      Unary Operations (e.g. ADD)     |
//    |                                      |
//    +--------------------------------------+ <-- NARY_BEGIN
//    |                                      |
//    |  Unary & Nary Operations (e.g. ADD)  |
//    |                                      |
//    +--------------------------------------+ <-- UNARY_END
//    |                                      |  
//    |       Binary & Nary Operations       |
//    |           (e.g. LSS, ADD)            |
//    |                                      |
//    +--------------------------------------+ <- NARY_END
//    |                 ...                  |
//
// Note that all operators whose enum value is greater than NARY_BEGIN and
// less than UNARY_END should be associative unless otherwise specified through
// the operator's attributes.
//
// Maintainer notice: The order of all these operators determines the order of
// the elements in the internal operators string array.
enum Operator : unsigned short {
  NOT,

  ADD,

  LAND,
  LOR,

  EQL,
  LSS,
};

// UNARY_BEGIN marks the first unary operator. See also Operator enum.
// UNARY_BEGIN is always less than or equal to UNARY_END.
extern const Operator UNARY_BEGIN;

// UNARY_END marks the last unary operator. See also Operator enum.
// UNARY_END is always less than or equal NARY_END.
extern const Operator UNARY_END;

// NARY_BEGIN marks the first nary operator. See also Operator enum.
// NARY_BEGIN is always less than or equal to NARY_END.
extern const Operator NARY_BEGIN;

// NARY_END marks the last binary operator. See also Operator enum.
// The range UNARY_END ... NARY_END gives all the operators that
// require at least two operands.
extern const Operator NARY_END;

// OperatorTraits is a lookup function that maps operators to their attributes.
// Since template specializations are used, this lookup occurs at compile-time.
template<Operator op>
class OperatorTraits {};

// OPERATOR_TRAITS_DEF is a macro whose second argument defines the mathematical
// properties of the operator given as the first argument.
#define OPERATOR_TRAITS_DEF(op, attribute_bv)\
template<>\
class OperatorTraits<op> {\
  public:\
    static const OperatorAttr attr = (attribute_bv);\
};\

// TODO: Consider using another bit mask for floats etc.
OPERATOR_TRAITS_DEF(NOT,  CLEAR_ATTR)
OPERATOR_TRAITS_DEF(ADD,  LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR)
OPERATOR_TRAITS_DEF(LAND, LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR)
OPERATOR_TRAITS_DEF(LOR,  LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR)
OPERATOR_TRAITS_DEF(EQL,  LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR)
OPERATOR_TRAITS_DEF(LSS,  CLEAR_ATTR)

static std::string LPAR = "(";
static std::string RPAR = ")";
static std::string QUERY = "?";
static std::string COLON = ":";

// operators is an internal array that maps an Operator to its corresponding
// string representation. Therefore, both data types need to be coordinated.
static std::string operators[] = { "!", "+", "&&", "||", "==", "<" };

// ExprKind is an enumeration type that identifies an Expr subclass.
enum ExprKind : unsigned int {
  ANY_EXPR,
  VALUE_EXPR,
  CAST_EXPR,
  UNARY_EXPR,
  TERNARY_EXPR,
  NARY_EXPR,

  // Expr subclasses that are not part of the library must register
  // with an ExprKind enum value that is greater than EXT_EXPR.
  //
  // See also ext_expr_kind()
  EXT_EXPR = 256u,
};

// ext_expr_kind(unsigned short) should be only used by library
// extensions that wish to create an Expr subclass whose kind
// must be greater than EXT_EXPR. The id value must be positive.
ExprKind ext_expr_kind(unsigned short id);

// Forward declaration for "Virtual Friend Function" idiom
class Expr;
typedef std::shared_ptr<Expr> SharedExpr;

// Expr is the base class for a symbolic expression such as "y = x + 7".
// Once a symbolic expression object has been instantiated, it can never
// be copied or modified. An Expr object corresponds to an rvalue in C++.
//
// Expressions form an acyclic directed graph (DAG) in which children are
// ordered. The space requirement for each DAG is guaranteed to be linear
// in the length of the execution path that generated the symbolic
// expression in the program under test.
//
// The syntactic structure of a symbolic expression can be captured by
// one of the following three classes: ValueExpr, UnaryExpr and NaryExpr.
// By construction, ValueExpr is always a leaf in the DAG. In contrast,
// both UnaryExpr and NaryExpr are always interior vertices in the DAG.
// This DAG could be traversed in preorder or postorder. Since each vertex
// in the DAG can have multiple children, there is no well-defined inorder.
//
// Subclasses whose get_kind() member function returns a value less than
// EXT_EXPR must be traversable with a unique member functions of the
// Visitor<T> class. Visitor<T>::visit(const Expr&) accounts for the case
// when get_kind() returns a value is greater than or equal to EXT_EXPR.
// This can be achieved by calling ext_expr_kind(unsigned short).
class Expr : public Walker {
private:
  const ExprKind kind;

  Expr(const Expr&);
  Expr& operator=(const Expr&);

protected:

  // Subclasses that are not part of this library must use a
  // kind enum value that is greater than EXT_EXPR.
  Expr(const ExprKind kind) : kind(kind) {}

public:

  // write(std::ostream&) serializes the expression to a human-readable format.
  virtual std::ostream& write(std::ostream&) const = 0;

  WALK_DEF(void)
  WALK_DEF(z3::expr)

  // get_kind() uniquely identifies the implementation of this Expr object.
  // Use the public static "kind" member field of the desired Expr subclass
  // that needs to be identified for downcast purposes.
  ExprKind get_kind() const { return kind; }

  // Uses "Virtual Friend Function" idiom.
  // See also http://www.parashift.com/c++-faq-lite/input-output.html#faq-15.11
  friend std::ostream& operator<<(std::ostream& out, const SharedExpr& expr);

  virtual ~Expr(){};
};

// SharedExpr is a C++11 shared pointer to a vertex in the DAG.
typedef std::shared_ptr<Expr> SharedExpr;

inline std::ostream& operator<<(std::ostream& out, const SharedExpr& expr) {
  // delegate the work to polymorphic member function
  expr->write(out);
  return out;
} 

// AnyExpr<T> represents a symbolic variable with an arbitrary value of type T.
// The specified template parameter should be a primitive type. Since there is
// no concrete value, the symbolic variable name is mandatory.
template<typename T>
class AnyExpr : public Expr {
private:
  const std::string name;

public:

  AnyExpr(const std::string& name) : Expr(ANY_EXPR), name(name) {}
  AnyExpr(const AnyExpr& other) : Expr(ANY_EXPR), name(other.name) {}

  const std::string& get_name() const { return name; }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

// Macro to inline a member function that gets a shared expression in the field.
#define GET_SHARED_EXPR(field) \
  const SharedExpr& get_##field() const { return field; }

// Macro to inline a member function that sets a shared expression in the field.
#define SET_SHARED_EXPR(field) \
  void set_##field(const SharedExpr& field) { this->field = field; };

// ValueExpr<T> represents a symbolic expression with a value whose type is T.
// T should be a primitive type. ValueExpr<T> objects are useful for a
// combination of concrete and symbolic (aka concolic) execution.
template<typename T>
class ValueExpr : public Expr {
private:
  const T value;
  const std::string name;

public:

  ValueExpr(const T& value) : Expr(VALUE_EXPR), value(value), name("") {}
  ValueExpr(const ValueExpr& other) : Expr(VALUE_EXPR), value(other.value), name(other.name) {}

  ValueExpr(const T& value, const std::string& name) :
    Expr(VALUE_EXPR), value(value), name(name) {}

  T get_value() const { return value; }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

// CastExpr is an annotation expression that identifies type casts. These type
// casts give bit precision semantics of a symbolic expression.
class CastExpr : public Expr {
private:
  SharedExpr expr;
  const Type type;

public:

  CastExpr(const Type type, const SharedExpr& expr) :
    Expr(CAST_EXPR), type(type), expr(expr) {}

  CastExpr(const CastExpr& other) :
    Expr(CAST_EXPR), type(other.type), expr(other.expr) {}

  GET_SHARED_EXPR(expr)
  SET_SHARED_EXPR(expr)

  // Returns an enum value for the type of the cast expression.
  Type get_type() const { return type; }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

// UnaryExpr is a vertex in the DAG with one child. It stores a symbolic
// expression of the form "op x" where op is an operation and x another
// symbolic expression.
class UnaryExpr : public Expr {
private:
  SharedExpr expr;
  const Operator op;

public:

  UnaryExpr(const Operator op, const SharedExpr& expr) :
    Expr(UNARY_EXPR), op(op), expr(expr) {}

  UnaryExpr(const UnaryExpr& other) :
    Expr(UNARY_EXPR), op(other.op), expr(other.expr) {}

  GET_SHARED_EXPR(expr)
  SET_SHARED_EXPR(expr)

  // Returns an enum value for the unary operator of this expression.
  Operator get_op() const { return op; }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

// TernaryExpr is a vertex in the DAG with three (ordered) children. It stores
// a symbolic expression of the form "x ? y : z" where x, y and z are symbolic
// expressions.
class TernaryExpr : public Expr {
private:
  SharedExpr cond_expr;
  SharedExpr then_expr;
  SharedExpr else_expr;

public:

  TernaryExpr(const SharedExpr& cond_expr, const SharedExpr& then_expr,
      const SharedExpr& else_expr) : Expr(TERNARY_EXPR),
    cond_expr(cond_expr), then_expr(then_expr), else_expr(else_expr) {}

  TernaryExpr(const TernaryExpr& other) : Expr(TERNARY_EXPR),
    cond_expr(other.cond_expr), then_expr(other.then_expr),
    else_expr(other.else_expr) {}

  GET_SHARED_EXPR(cond_expr)
  GET_SHARED_EXPR(then_expr)
  GET_SHARED_EXPR(else_expr)

  SET_SHARED_EXPR(cond_expr)
  SET_SHARED_EXPR(then_expr)
  SET_SHARED_EXPR(else_expr)

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

// NaryExpr is a vertex in the DAG with at least two or more (ordered) children.
// If NaryExpr::is_associative() returns true, these children correspond to
// operands of an associative operator such as integer addition. Note that not
// every kind of ADD operator is necessarily associative. For example, IEEE 754
// floating-point addition is not associative. However, all forms of machine
// addition tend to be commutative. An example of a non-commutative operator is
// subtraction. A non-associative but commutative operator is called a magma.
// Even though the operator might be commutative, its operands are ordered
// according to the order in which they have been prepended or appended.
//
// It is possible to have NaryExpr objects which have not got yet at least
// two operands. These nary expressions are said to be partial. Partial
// nary expressions play an important role in DAG rewriting.
class NaryExpr : public Expr {
private:
  std::list<SharedExpr> exprs;

  const Operator op;
  const OperatorAttr attr;

public:

  // Constructor for a partial nary expression with the specified operator
  // attributes. After this object has been instantiated, either one of the
  // modifiers has to be called at least twice.
  NaryExpr(const Operator op, const OperatorAttr attr) : Expr(NARY_EXPR),
    exprs(), op(op), attr(attr) {}

  // Constructor for a partial nary nary expression with the specified operator
  // attributes. After this object has been instantiated, either one of the
  // modifiers has to be called at least once.
  NaryExpr(const Operator op, const OperatorAttr attr, const SharedExpr& expr) :
    Expr(NARY_EXPR), exprs(), op(op), attr(attr) {

    append_expr(expr);
  }

  // Constructor for a binary expression with the specified associativity.
  NaryExpr(const Operator op, const OperatorAttr attr, const SharedExpr& x_expr,
      const SharedExpr& y_expr) :
    Expr(NARY_EXPR), exprs(), op(op), attr(attr) {

    append_expr(x_expr);
    append_expr(y_expr);
  }

  NaryExpr(const NaryExpr& other) : Expr(NARY_EXPR), exprs(other.exprs),
      op(other.op), attr(other.attr) {}

  // is_partial() returns true if and only if there are fewer than
  // two operands.
  bool is_partial() const { return exprs.size() < 2; }

  // get_exprs() returns the operands of this nary associative operator.
  // The order of these is according to the order in which the operands
  // have been prepended or appended to this nary expression.
  const std::list<SharedExpr> get_exprs() const { return exprs; }

  // append_expr(const SharedExpr&) adds the supplied expression as the
  // rightmost operand in this nary expression.
  void append_expr(const SharedExpr& expr) { exprs.push_back(expr); }

  // prepend_expr(const SharedExpr&) adds the supplied expression as the
  // leftmost operand in this nary expression.
  void prepend_expr(const SharedExpr& expr) { exprs.push_front(expr); }

  // Returns an enum value for the nary associative operator of this expression.
  Operator get_op() const { return op; }

  // Returns a bit vector that describes the mathematical properties of the
  // operator which is applied to the operands of this nary expression.
  OperatorAttr get_attr() const { return attr; }

  // is_commutative() returns true iff (x op y) = (y op x).
  bool is_commutative() const { return get_commutative_attr(attr); }

  // is_associative() returns true iff ((x op y) op z) = (x op (y op z)).
  bool is_associative() const { return get_associative_attr(attr); }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

template<typename T>
std::ostream& AnyExpr<T>::write(std::ostream& out) const {
  return out << "[" << name << "]";
}

template<typename T>
std::ostream& ValueExpr<T>::write(std::ostream& out) const {
  if(name.empty()) {
    return out << value;
  }

  return out << "[" << name << ":" << value << "]";
}

}

#endif /* EXPR_H_ */
