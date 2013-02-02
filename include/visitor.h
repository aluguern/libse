// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef VISITOR_H_
#define VISITOR_H_

namespace se {

/*! \file visitor.h */
  
// Forward declarations of the kind of vertices the Visitor API must be able
// to traverse in the DAG. These must match the expression kinds in ExprKind.
class Expr;

template<typename T>
class AnyExpr;

template<typename T>
class ValueExpr;

class CastExpr;
class UnaryExpr;
class IfThenElseExpr;
class NaryExpr;

/// \interface Visitor
/// \brief Traversal algorithm for an acyclic directed graph (DAG)

/// Visitor can traverse a DAG whose vertices are Expr subclasses. Recall that
/// this DAG represents the syntactic structure of a symbolic expression. The
/// order of the traversal is implemention-specific.
///
/// Let `E` be an Expr subclass such that \ref Expr::kind() "E::kind()" <
/// \ref EXT_EXPR. Then, `E` is traversable by `Visitor<T>::visit(const E&)`.
/// Otherwise, `E` is visited by visit(const Expr&). This eases the
/// incremental extension with new Expr subclasses.
///
/// Note that alternative designs are discussed in the object-oriented
/// programming literature in the context of the "Expression Problem".
///
/// \see Walker
template<typename T>
class Visitor {

#define TYPED_VISIT_DECL(type)\
  virtual T visit(const AnyExpr<type>&) = 0;\
  virtual T visit(const ValueExpr<type>&) = 0;

public:

  /// Handle expression where Expr::kind() >= \ref EXT_EXPR.
  virtual T visit(const Expr& other) = 0;

  TYPED_VISIT_DECL(bool)
  TYPED_VISIT_DECL(char)
  TYPED_VISIT_DECL(short int)
  TYPED_VISIT_DECL(int)

  virtual T visit(const CastExpr&) = 0;
  virtual T visit(const UnaryExpr&) = 0;
  virtual T visit(const IfThenElseExpr&) = 0;
  virtual T visit(const NaryExpr&) = 0;

  virtual ~Visitor() {}
};

/// \interface Walker
/// \brief Acceptor in Visitor pattern

/// Must be implemented by every Expr subclass to support polymorphic DAG
/// traversals with the Visitor interface. To facilitate this, the Expr class
/// inherits Walker. Since it declares only pure virtual member functions with
/// the \ref WALK_DECL macro, each Expr subclass should use \ref WALK_DEF
/// inside its class to define the required walk member functions.
///
/// Example:
///
///     class UnaryExpr : public Expr {
///       public:
///       ...
///       WALK_DEF(T1)
///       WALK_DEF(T2)
///       ...
///       WALK_DEF(TN)
///     }
///
/// where T1 ... TN are types for which there exists a Visitor<T> template
/// class.
class Walker {

public:

/// Declare a pure virtual visitor dispatcher member function
#define WALK_DECL(type)\
  virtual type walk(Visitor<type>* const) const = 0;

/// Define a visitor dispatcher member function
#define WALK_DEF(type)\
  type walk(Visitor<type>* const v_ptr) const { return v_ptr->visit(*this); }

  WALK_DECL(void)
  WALK_DECL(z3::expr)

};

}

#endif /* VISITOR_H_ */
