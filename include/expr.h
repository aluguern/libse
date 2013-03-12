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
#include "core/op.h"
#include "visitor.h"

/// Symbolic execution namespace
namespace se {

/*! \file expr.h */

/// Built-in primitive types for which symbolic execution is supported

// Order of enum values determines the content of the internal types array.
enum Type : unsigned short {
  /// \see std::numeric_limits<bool>
  BOOL,
  /// \see std::numeric_limits<char>
  CHAR,
  /// \see std::numeric_limits<int>
  INT,
  /// \see std::numeric_limits<size_t>
  SIZE_T,

  POINTER
};

// Internal array which maps a Type to its corresponding string representation.
// Therefore, modifications to either Type or the array need to be coordinated.
static std::string types[] = { "bool", "char", "int" };

static std::string LPAR = "(";
static std::string RPAR = ")";
static std::string LSQPAR = "[";
static std::string RSQPAR = "]";
static std::string QUERY = "?";
static std::string COLON = ":";
static std::string COMMA = ",";
static std::string SPACE = " ";

// Internal array which maps an Operator to its corresponding string
// representation. Therefore, both data types need to be coordinated.
static std::string operators[] = { "!", "+", "-", "&&", "||", "==", "<" };

/// Expr subclass identifier

/// Expr subclasses which are not part of the library must register
/// with an enum value that is greater than or equal to \ref EXT_EXPR.
/// This can be facilitated by ext_expr_kind(unsigned short).
enum ExprKind : unsigned int {
  /// Identifier for AnyExpr
  ANY_EXPR,
  /// Identifier for ValueExpr
  VALUE_EXPR,
  /// Identifier for CastExpr
  CAST_EXPR,
  /// Identifier for UnaryExpr
  UNARY_EXPR,
  /// Identifier for IfThenElseExpr
  ITE_EXPR,
  /// Identifier for NaryExpr
  NARY_EXPR,
  /// Identifier for ArrayExpr
  ARRAY_EXPR,
  /// Identifier for SelectExpr
  SELECT_EXPR,
  /// Identifier for StoreExpr
  STORE_EXPR,

  /// Expr subclasses which are not part of the library must use this value.
  ///
  /// \see ext_expr_kind(unsigned short)
  EXT_EXPR = 256u,
};

/// Create an ExprKind identifier for an external Expr subclass

/// \see EXT_EXPR
constexpr ExprKind ext_expr_kind(unsigned short id) {
  // assumes IDs are sufficiently small such that there is no overflow
  return static_cast<ExprKind>(EXT_EXPR + id);
}

// Forward declaration for "Virtual Friend Function" idiom
class Expr;
typedef std::shared_ptr<Expr> SharedExpr;

/// Base class for a symbolic expression such as "y = x + 7"

/// Objects of this type correspond to C++ rvalues.
///
/// Expressions must form an acyclic directed graph (DAG) in which children
/// are ordered. This DAG can capture the syntactic structure of a symbolic
/// expression. With \ref SharedExpr "sharing of subexpressions", the space
/// requirement of a DAG can be linear in the length of the execution path
/// in the program under test which yields the symbolic expression.
///
/// The DAG can be \ref Visitor "traversed" in preorder or postorder. Since
/// each vertex could have multiple children, there is no well-defined inorder.
/// A subclass whose Expr::kind() member function returns a value less
/// than \ref EXT_EXPR is traversable with a unique Visitor member function.
/// Otherwise, Visitor::visit(const Expr&) is called. This is typically the
/// case for \ref Expr(const ExprKind) "library extensions".
///
/// \remark Subclasses must be immutable unless clearly documented otherwise
class Expr : public Walker {
private:
  const ExprKind m_kind;

  Expr(const Expr&);
  Expr& operator=(const Expr&);

protected:

  /// Subclasses which are not distributed as part of this library must supply
  /// `kind` >= @ref EXT_EXPR.
  ///
  /// \see ext_expr_kind(unsigned short)
  Expr(const ExprKind kind) : m_kind(kind) {}

public:

  /// Serialize expression to a human-readable format
  virtual std::ostream& write(std::ostream&) const = 0;

  WALK_DEF(void)
  WALK_DEF(z3::expr)

  /// Unique Expr subclass identifier

  /// \remark kind() can be safely used for downcast purposes
  ExprKind kind() const { return m_kind; }

  /// Two's complement signedness
  // TODO: Implement unsigned types
  bool is_signed() const { return true; }

  /// Serialize expression to a human-readable format

  // Uses "Virtual Friend Function" idiom.
  // See also http://www.parashift.com/c++-faq-lite/input-output.html#faq-15.11
  friend std::ostream& operator<<(std::ostream& out, const SharedExpr& expr);

  virtual ~Expr(){};
};

/// Vertex in the DAG which represents a symbolic expression

/// Vertices are shared in order to minimize the size of the DAG. Since this
/// sharing is implemented through C++11 smart pointers, their reference
/// counters can be used when computing the minimal static single assignment
/// form (SSA) of a DAG. This sharing necessitates that a SharedExpr is
/// never modified once it has been inserted into the DAG. Thus, if an
/// Expr subclass is mutable, a SharedExpr should only be constructed from
/// a copy of the expression.
///
/// \warning When symbolic expression is shared, it should never be modified
typedef std::shared_ptr<Expr> SharedExpr;

inline std::ostream& operator<<(std::ostream& out, const SharedExpr& expr) {
  // delegate the work to polymorphic member function
  expr->write(out);
  return out;
} 

/// Type-safe symbolic variable of an arbitrary value

/// Leaf in the DAG for an arbitrary value of type T. Since there is no
/// concrete value, the symbolic variable name is mandatory.
///
/// \remark T should be a primitive type
/// \remark Immutable
template<typename T>
class AnyExpr : public Expr {
private:
  const std::string m_identifier;

public:

  AnyExpr(const std::string& identifier) : Expr(ANY_EXPR), m_identifier(identifier) {}
  AnyExpr(const AnyExpr& other) : Expr(ANY_EXPR), m_identifier(other.m_identifier) {}

  /// Symbolic variable identifier
  const std::string& identifier() const { return m_identifier; }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

/// Define an inline getter function which returns the given field
#define GET_SHARED_EXPR(field) \
  const SharedExpr& field() const { return m_##field; }

/// Define an inline setter function for the given field
#define SET_SHARED_EXPR(field) \
  void set_##field(const SharedExpr& field) { m_##field = field; };

/// Type-safe concrete value with an optional symbolic value

/// Leaf in the DAG for a concrete and/or an arbitrary value of type T. The
/// combination of a concrete and symbolic value is the basis for single-path
/// (i.e. DART-style) symbolic execution. This is also known as concolic
/// execution.
///
/// \remark T should be a primitive type
/// \remark Immutable
template<typename T>
class ValueExpr : public Expr {
private:
  const T m_value;
  const std::string m_identifier;

public:

  ValueExpr(const T& value) : Expr(VALUE_EXPR), m_value(value),
    m_identifier("") {}

  ValueExpr(const ValueExpr& other) : Expr(VALUE_EXPR), m_value(other.m_value),
    m_identifier(other.m_identifier) {}

  ValueExpr(const T& value, const std::string& identifier) :
    Expr(VALUE_EXPR), m_value(value), m_identifier(identifier) {}

  /// Concrete value
  T value() const { return m_value; }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

/// Type cast of another symbolic expression

/// Interior vertex in the DAG for a type cast operation such as `(char)(i)`
/// where `i` could be an `int`.
///
/// \remark Immutable
class CastExpr : public Expr {
private:
  SharedExpr m_operand;
  const Type m_type;

public:

  CastExpr(const Type type, const SharedExpr& operand) :
    Expr(CAST_EXPR), m_type(type), m_operand(operand) {}

  CastExpr(const CastExpr& other) :
    Expr(CAST_EXPR), m_type(other.m_type), m_operand(other.m_operand) {}

  GET_SHARED_EXPR(operand)
  SET_SHARED_EXPR(operand)

  /// Type cast which coerces the operand
  Type type() const { return m_type; }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

/// Operator with one operand

/// Interior vertex in the DAG with one child to create a symbolic expression
/// of the form "op x" where op is an \ref Operator "operator" and x is another
/// shared expression.
class UnaryExpr : public Expr {
private:
  SharedExpr m_operand;
  const Operator m_op;

public:

  UnaryExpr(const Operator op, const SharedExpr& operand) :
    Expr(UNARY_EXPR), m_op(op), m_operand(operand) {}

  UnaryExpr(const UnaryExpr& other) :
    Expr(UNARY_EXPR), m_op(other.m_op), m_operand(other.m_operand) {}

  GET_SHARED_EXPR(operand)
  SET_SHARED_EXPR(operand)

  /// Unary operator
  Operator op() const { return m_op; }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

/// Ternary expression of the form "x ? y : z"

/// Interior vertex in the DAG with three (ordered) children which form a
/// symbolic expression of the form "x ? y : z" where x, y and z are shared
/// expressions such that "x" has a Boolean type and the type of both "y" and
/// "z" are identical.
///
/// \warning Operands are not type checked
class IfThenElseExpr : public Expr {
private:
  SharedExpr m_cond_expr;
  SharedExpr m_then_expr;
  SharedExpr m_else_expr;

public:

  IfThenElseExpr(const SharedExpr& cond_expr, const SharedExpr& then_expr,
      const SharedExpr& else_expr) : Expr(ITE_EXPR),
    m_cond_expr(cond_expr), m_then_expr(then_expr), m_else_expr(else_expr) {}

  IfThenElseExpr(const IfThenElseExpr& other) : Expr(ITE_EXPR),
    m_cond_expr(other.m_cond_expr), m_then_expr(other.m_then_expr),
    m_else_expr(other.m_else_expr) {}

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

/// Operator with several operands

/// Interior vertex in the DAG with several (ordered) children
///
/// If NaryExpr::is_associative(), these children correspond to operands of an
/// associative operator such as integer addition. Note that not every kind of
/// \ref ADD operator is necessarily associative. For example, IEEE 754
/// floating-point addition is not associative. However, all forms of machine
/// addition tend to be commutative.
///
/// An example of a non-commutative operator is subtraction. A non-associative
/// but commutative operator is called a magma. Even though the operator might
/// be commutative, its operands are ordered according to the order in which
/// they have been \ref prepend_operand(const SharedExpr&) "prepended" or
/// \ref append_operand(const SharedExpr&) "appended".
///
/// It is possible to have NaryExpr objects with fewer than two operands.
/// These nary expressions are said to be \ref is_partial() "partial".
/// Partial nary expression play an important role in \ref Value::aggregate()
/// "constant propagation". Due to \ref SharedExpr "sharing of subexpressions",
/// any expression simplifications should occur before inserting the expression
/// into the DAG.
///
/// \warning Mutable!
class NaryExpr : public Expr {
private:
  std::list<SharedExpr> m_operands;

  const Operator m_op;
  const OperatorAttr m_attr;

public:

  /// Partial nary expression

  /// \warning Either one of the modifiers has to be called at least twice
  NaryExpr(const Operator op, const OperatorAttr attr) : Expr(NARY_EXPR),
    m_operands(), m_op(op), m_attr(attr) {}

  /// Partial nary expression

  /// \warning Either one of the modifiers has to be called at least once.
  NaryExpr(const Operator op, const OperatorAttr attr,
      const SharedExpr& operand) :
    Expr(NARY_EXPR), m_operands(), m_op(op), m_attr(attr) {

    append_operand(operand);
  }

  /// Binary expression

  /// \remark Additional operands may be added later
  NaryExpr(const Operator op, const OperatorAttr attr,
      const SharedExpr& x_operand, const SharedExpr& y_operand) :
    Expr(NARY_EXPR), m_operands(), m_op(op), m_attr(attr) {

    append_operand(x_operand);
    append_operand(y_operand);
  }

  /// Copy operands, operator and its attributes
  NaryExpr(const NaryExpr& other) : Expr(NARY_EXPR),
      m_operands(other.m_operands), m_op(other.m_op), m_attr(other.m_attr) {}

  /// Are there fewer than two operands?
  bool is_partial() const { return m_operands.size() < 2; }

  /// Ordered children of the vertex in the DAG

  /// The order of the operands is according to the order in which they have
  /// been \ref prepend_operand(const SharedExpr&) "prepended" or
  /// \ref append_operand(const SharedExpr&) "appended" even if if the operator
  /// is \ref is_associative() "associative".
  const std::list<SharedExpr> operands() const { return m_operands; }

  /// Add rightmost operand
  void append_operand(const SharedExpr& expr) { m_operands.push_back(expr); }

  /// Add leftmost operand
  void prepend_operand(const SharedExpr& expr) { m_operands.push_front(expr); }

  /// Nary operator
  Operator op() const { return m_op; }

  /// Mathematical properties of the nary operator

  /// \see is_commutative()
  /// \see is_associative()
  OperatorAttr attr() const { return m_attr; }

  /// Is (x op y) = (y op x) where op = op()?
  bool is_commutative() const { return get_commutative_attr(m_attr); }

  /// Is ((x op y) op z) = (x op (y op z)) where op = op()?
  bool is_associative() const { return get_associative_attr(m_attr); }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

class ArrayExpr : public Expr {
private:
  const Type m_range_type;
  const size_t m_size;
  const std::string m_identifier;

public:
  ArrayExpr(Type range_type, size_t size, const std::string& identifier) :
    Expr(ARRAY_EXPR), m_range_type(range_type), m_size(size),
    m_identifier(identifier) {}

  ArrayExpr(const ArrayExpr& other) : Expr(ARRAY_EXPR),
    m_range_type(other.m_range_type), m_size(other.m_size),
    m_identifier(other.m_identifier) {}

  const std::string& identifier() const { return m_identifier; }
  size_t size() const { return m_size; }
  Type range_type() const { return m_range_type; }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

class SelectExpr : public Expr {
private:
  const SharedExpr m_array_expr;
  const SharedExpr m_index_expr;

public:
  SelectExpr(const SharedExpr& array_expr, const SharedExpr& index_expr) :
    Expr(SELECT_EXPR), m_array_expr(array_expr), m_index_expr(index_expr) {}

  SelectExpr(const SelectExpr& other) : Expr(SELECT_EXPR),
    m_array_expr(other.m_array_expr), m_index_expr(other.m_index_expr) {}

  GET_SHARED_EXPR(array_expr)
  GET_SHARED_EXPR(index_expr)

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

class StoreExpr : public Expr {
private:
  const SharedExpr m_array_expr;
  const SharedExpr m_index_expr;
  const SharedExpr m_elem_expr;

public:
  StoreExpr(const SharedExpr& array_expr, const SharedExpr& index_expr,
    const SharedExpr& elem_expr) : Expr(STORE_EXPR), m_array_expr(array_expr),
    m_index_expr(index_expr), m_elem_expr(elem_expr) {}

  StoreExpr(const StoreExpr& other) : Expr(STORE_EXPR),
    m_array_expr(other.m_array_expr), m_index_expr(other.m_index_expr),
    m_elem_expr(other.m_elem_expr) {}

  GET_SHARED_EXPR(array_expr)
  GET_SHARED_EXPR(index_expr)
  GET_SHARED_EXPR(elem_expr)

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

template<typename T>
std::ostream& AnyExpr<T>::write(std::ostream& out) const {
  return out << "[" << m_identifier << "]";
}

template<typename T>
std::ostream& ValueExpr<T>::write(std::ostream& out) const {
  if(m_identifier.empty()) {
    return out << m_value;
  }

  return out << "[" << m_identifier << ":" << m_value << "]";
}

}

#endif /* EXPR_H_ */
