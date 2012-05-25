#ifndef EXPR_H_
#define EXPR_H_

#include <memory>
#include <string>
#include <ostream>

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

// Operator enumerates the built-in C++ operations. The order of these
// operations determines the order of elements in the operators array.
enum Operator : unsigned short {
  ADD,
  LSS,
  NOT
};

static std::string LPAR = "(";
static std::string RPAR = ")";
static std::string QUERY = "?";
static std::string COLON = ":";

// operators is an internal array that maps an Operator to its corresponding
// string representation. Therefore, both data types need to be coordinated.
static std::string operators[] = { "+", "<", "!" };

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
class Expr {
private:
  Expr(const Expr&);
  Expr& operator=(const Expr&);

protected:
  Expr() {}
  virtual ~Expr(){};

public:
  virtual std::ostream& write(std::ostream&) const = 0;
};

// SharedExpr is a C++11 shared pointer to a vertex in the DAG.
typedef std::shared_ptr<Expr> SharedExpr;

// AnyExpr<T> represents a symbolic variable with an arbitrary value of type T.
// The specified template parameter should be a primitive type. Since there is
// no concrete value, the symbolic variable name is mandatory.
template<typename T>
class AnyExpr : public Expr {
private:
  const std::string name;

public:

  AnyExpr(const std::string& name) : name(name) {};
  ~AnyExpr() {};

  std::ostream& write(std::ostream&) const;
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

  ValueExpr(const T& value, const std::string& name) :
    value(value), name(name) {};
  ~ValueExpr() {};

  T get_value() const { return value; }

  std::ostream& write(std::ostream&) const;
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
  ~UnaryExpr() {};

  std::ostream& write(std::ostream&) const;

  GET_SHARED_EXPR(expr)
  SET_SHARED_EXPR(expr)

  // Returns an enum value for the unary operator of this expression.
  Operator get_op() const { return op; }

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
  ~BinaryExpr() {};

  GET_SHARED_EXPR(x_expr)
  GET_SHARED_EXPR(y_expr)

  SET_SHARED_EXPR(x_expr)
  SET_SHARED_EXPR(y_expr)

  // Returns an enum value for the binary operator of this expression.
  Operator get_op() const { return op; }

  std::ostream& write(std::ostream&) const;

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
  ~TernaryExpr() {};

  GET_SHARED_EXPR(cond_expr)
  GET_SHARED_EXPR(then_expr)
  GET_SHARED_EXPR(else_expr)

  SET_SHARED_EXPR(cond_expr)
  SET_SHARED_EXPR(then_expr)
  SET_SHARED_EXPR(else_expr)

  std::ostream& write(std::ostream&) const;
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

// CastExpr is an annotation expression that identifies type casts. These type
// casts give bit precision semantics of a symbolic expression.
class CastExpr : public Expr {
private:
  SharedExpr expr;
  const Type type;

public:
  CastExpr(const SharedExpr& expr, const Type type) :
    expr(expr), type(type) {}

  GET_SHARED_EXPR(expr)
  SET_SHARED_EXPR(expr)

  // Returns an enum value for the type of the cast expression.
  Type get_type() const { return type; }

  std::ostream& write(std::ostream&) const;
};

}

#endif /* EXPR_H_ */
