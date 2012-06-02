#ifndef EXPR_H_
#define EXPR_H_

#include <memory>
#include <string>
#include <ostream>
#include <z3++.h>

namespace sp {

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
// positive number n. Since some operators are both unary and binary (e.g. ADD),
// the enum value of these identifiers overlap. The following schematic diagram
// illustrates this:
//
//    +---------------------------+ <-- 0 (unsigned short)
//    |                           |
//    |         Future Use        |
//    |                           |
//    +---------------------------+ <-- UNARY_BEGIN
//    |                           |
//    |      Unary Operations     |
//    |                           |
//    +---------------------------| <-- BINARY_BEGIN
//    | Unary & Binary Operations |
//    +---------------------------+ <-- UNARY_END
//    |                           |  
//    |     Binary Operations     |
//    |                           |
//    +---------------------------+ <-- BINARY_END
//    |            ...            |
//
// Maintainer notice: The order of all these operators determines the order of
// the elements in the internal operators string array.
enum Operator : unsigned short {
  NOT,
  ADD,
  LSS,
};

// UNARY_BEGIN marks the first unary operator. See also Operator enum.
extern const Operator UNARY_BEGIN;

// UNARY_END marks the last unary operator. See also Operator enum.
extern const Operator UNARY_END;

// BINARY_BEGIN marks the first binary operator. See also Operator enum.
extern const Operator BINARY_BEGIN;

// BINARY_END marks the last binary operator. See also Operator enum.
extern const Operator BINARY_END;

static std::string LPAR = "(";
static std::string RPAR = ")";
static std::string QUERY = "?";
static std::string COLON = ":";

// operators is an internal array that maps an Operator to its corresponding
// string representation. Therefore, both data types need to be coordinated.
static std::string operators[] = { "!", "+", "<" };

// The Visitor can traverse the following type of vertices in the DAG:
template<typename T>
class AnyExpr;

template<typename T>
class ValueExpr;

class UnaryExpr;
class BinaryExpr;
class TernaryExpr;
class CastExpr;

// Since virtual template functions are not allowed (because there is no bound
// on the number of possibilities for which the vtable would need to account),
// we declare the permissible return types as traits.
template<typename T> struct VisitorTraits;

#define VISITOR_TRAIT_DECL(type)\
template<>\
struct VisitorTraits<type> {\
  typedef type ReturnType;\
};

VISITOR_TRAIT_DECL(void)
VISITOR_TRAIT_DECL(z3::expr)

// Forward declaration to resolve circular dependency between
// Visitor::walk(const SharedExpr&) and Expr::walk(Visitor<T>* const).
class Expr;
typedef std::shared_ptr<Expr> SharedExpr;

// Visitor is an interface to traverse an acyclic directed graph (DAG) that
// represents the syntactic structure of an expression. The order in which a
// Visitor's visit member functions is called and the argument of that call is
// determined by the tree traversal algorithm.
template<typename T>
class Visitor {

#define TYPED_VISIT_DECL(type)\
  virtual ReturnType visit(const AnyExpr<type>&) = 0;\
  virtual ReturnType visit(const ValueExpr<type>&) = 0;

public:

  // ReturnType declares the type that each visit member function returns.
  typedef typename VisitorTraits<T>::ReturnType ReturnType;

  // walk(const SharedExpr&) starts the tree traversal at the specified root.
  T walk(const SharedExpr& expr);

  TYPED_VISIT_DECL(bool)
  TYPED_VISIT_DECL(char)
  TYPED_VISIT_DECL(short int)
  TYPED_VISIT_DECL(int)

  virtual ReturnType visit(const UnaryExpr&) = 0;
  virtual ReturnType visit(const BinaryExpr&) = 0;
  virtual ReturnType visit(const TernaryExpr&) = 0;
  virtual ReturnType visit(const CastExpr&) = 0;

  virtual ~Visitor() {}
};

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
// one of the following three classes: ValueExpr, UnaryExpr and BinaryExpr.
// By construction, ValueExpr is always a leaf in the DAG. In contrast,
// both UnaryExpr and BinaryExpr are always interior vertices in the DAG.
// This DAG could be traversed in preorder or postorder. Since each vertex
// in the DAG can have multiple children, there is no well-defined inorder.
class Expr {
private:
  Expr(const Expr&);
  Expr& operator=(const Expr&);

protected:
  Expr() {}

public:
  virtual std::ostream& write(std::ostream&) const = 0;

// Declare an internal virtual walk member function that dispatches a
// constant reference to the current polymorphic object by calling the
// visit member function on the supplied Visitor<type> object. The type
// argument must be one of the traits declared with VISITOR_TRAIT_DECL.
// These member functions are only public because friends are not shared
// through inheritance.
//
// Use Visitor::walk(const SharedExpr&) as the public tree traversal API.
#define WALK_DECL(type)\
  virtual typename VisitorTraits<type>::ReturnType\
    walk(Visitor<typename VisitorTraits<type>::ReturnType>* const) const = 0;

// Define member function that has been declared with WALK_DECL.
#define WALK_DEF(type)\
  typename VisitorTraits<type>::ReturnType\
    walk(Visitor<typename VisitorTraits<type>::ReturnType>* const v_ptr) const { return v_ptr->visit(*this); }

  WALK_DECL(void)
  WALK_DECL(z3::expr)

  virtual ~Expr(){};
};

// SharedExpr is a C++11 shared pointer to a vertex in the DAG.
typedef std::shared_ptr<Expr> SharedExpr;

template<typename T>
inline T Visitor<T>::walk(const SharedExpr& expr) { return expr->walk(this); }

// AnyExpr<T> represents a symbolic variable with an arbitrary value of type T.
// The specified template parameter should be a primitive type. Since there is
// no concrete value, the symbolic variable name is mandatory.
template<typename T>
class AnyExpr : public Expr {
private:
  const std::string name;

public:

  AnyExpr(const std::string& name) : name(name) {};
  AnyExpr(const AnyExpr& other) : name(other.name) {};

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
  ValueExpr(const T& value) : value(value), name("") {};
  ValueExpr(const ValueExpr& other) : value(other.value), name(other.name) {};

  ValueExpr(const T& value, const std::string& name) :
    value(value), name(name) {};

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
  CastExpr(const SharedExpr& expr, const Type type) : expr(expr), type(type) {}
  CastExpr(const CastExpr& other) : expr(other.expr), type(other.type) {}

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
  UnaryExpr(const SharedExpr& expr, const Operator op) : expr(expr), op(op) {};
  UnaryExpr(const UnaryExpr& other) : expr(other.expr), op(other.op) {}

  GET_SHARED_EXPR(expr)
  SET_SHARED_EXPR(expr)

  // Returns an enum value for the unary operator of this expression.
  Operator get_op() const { return op; }

  std::ostream& write(std::ostream&) const;

  WALK_DEF(void)
  WALK_DEF(z3::expr)
};

// BinaryExpr is a vertex in the DAG with two (ordered) children. It stores a
// symbolic expression of the form "x op y" where op is an operation and x
// and y are both symbolic expressions.
class BinaryExpr : public Expr {
private:
  // left operand
  SharedExpr x_expr;

  // right operand
  SharedExpr y_expr;
  const Operator op;

public:
  BinaryExpr(const SharedExpr& x_expr, const SharedExpr& y_expr,
    const Operator op) : x_expr(x_expr), y_expr(y_expr), op(op) {};

  BinaryExpr(const BinaryExpr& other) : x_expr(other.x_expr),
    y_expr(other.y_expr), op(other.op) {}

  GET_SHARED_EXPR(x_expr)
  GET_SHARED_EXPR(y_expr)

  SET_SHARED_EXPR(x_expr)
  SET_SHARED_EXPR(y_expr)

  // Returns an enum value for the binary operator of this expression.
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
    const SharedExpr& else_expr) : cond_expr(cond_expr), then_expr(then_expr),
                                   else_expr(else_expr) {};

  TernaryExpr(const TernaryExpr& other) : cond_expr(other.cond_expr),
    then_expr(other.then_expr), else_expr(other.else_expr) {}

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
