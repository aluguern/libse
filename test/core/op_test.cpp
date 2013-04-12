#include "gtest/gtest.h"
#include "core/op.h"

using namespace se;

TEST(OpTest, OperatorsOrder) {
  static_assert(NOT + 1 == ADD, "");
  static_assert(ADD + 1 == SUB, "");
  static_assert(SUB + 1 == LAND, "");
  static_assert(LOR + 1 == EQL, "");
  static_assert(EQL + 1 == LSS, "");
}

TEST(OpTest, AttrFunctions) {
  static_assert(OperatorInfo<NOT>::s_op.is_unary(), "");

  static_assert(OperatorInfo<ADD>::s_op.is_commutative(), "");
  static_assert(OperatorInfo<ADD>::s_op.is_associative(), "");
  static_assert(OperatorInfo<ADD>::s_op.has_identity(), "");
  static_assert(!OperatorInfo<ADD>::s_op.is_unary(), "");

  static_assert(!OperatorInfo<SUB>::s_op.is_commutative(), "");
  static_assert(!OperatorInfo<SUB>::s_op.is_associative(), "");
  static_assert(OperatorInfo<SUB>::s_op.has_identity(), "");
  static_assert(!OperatorInfo<SUB>::s_op.is_unary(), "");

  static_assert(OperatorInfo<LAND>::s_op.is_commutative(), "");
  static_assert(OperatorInfo<LAND>::s_op.is_associative(), "");
  static_assert(OperatorInfo<LAND>::s_op.has_identity(), "");
  static_assert(!OperatorInfo<LAND>::s_op.is_unary(), "");

  static_assert(OperatorInfo<LOR>::s_op.is_commutative(), "");
  static_assert(OperatorInfo<LOR>::s_op.is_associative(), "");
  static_assert(OperatorInfo<LOR>::s_op.has_identity(), "");
  static_assert(!OperatorInfo<LOR>::s_op.is_unary(), "");

  static_assert(OperatorInfo<EQL>::s_op.is_commutative(), "");
  static_assert(OperatorInfo<EQL>::s_op.is_associative(), "");
  static_assert(!OperatorInfo<EQL>::s_op.has_identity(), "");
  static_assert(!OperatorInfo<EQL>::s_op.is_unary(), "");

  static_assert(!OperatorInfo<LSS>::s_op.is_commutative(), "");
  static_assert(!OperatorInfo<LSS>::s_op.is_associative(), "");
  static_assert(!OperatorInfo<LSS>::s_op.has_identity(), "");
  static_assert(!OperatorInfo<LSS>::s_op.is_unary(), "");
}

// See diagram in include/op.h
TEST(OpTest, OperatorEnumLayout) {
  static_assert(NOT == UNARY_BEGIN, "");
  static_assert(SUB == UNARY_END, "");
  static_assert(ADD == NARY_BEGIN, "");
  static_assert(LSS == NARY_END, "");
}
