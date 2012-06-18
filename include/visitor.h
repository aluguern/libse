// Copyright 2012, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef VISITOR_H_
#define VISITOR_H_

namespace se {

// Forward declarations of the kind of vertices the Visitor API must be able
// to traverse in the DAG. These must match the expression kinds in ExprKind.
template<typename T>
class AnyExpr;

template<typename T>
class ValueExpr;

class CastExpr;
class UnaryExpr;
class TernaryExpr;
class NaryExpr;

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

  TYPED_VISIT_DECL(bool)
  TYPED_VISIT_DECL(char)
  TYPED_VISIT_DECL(short int)
  TYPED_VISIT_DECL(int)

  virtual ReturnType visit(const CastExpr&) = 0;
  virtual ReturnType visit(const UnaryExpr&) = 0;
  virtual ReturnType visit(const TernaryExpr&) = 0;
  virtual ReturnType visit(const NaryExpr&) = 0;

  virtual ~Visitor() {}
};

}

#endif /* VISITOR_H_ */
