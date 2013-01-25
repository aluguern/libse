#include "gtest/gtest.h"
#include "op.h"

using namespace se;

TEST(OpTest, OperatorsOrder) {
  EXPECT_EQ(NOT + 1, ADD);
  EXPECT_EQ(ADD + 1, LAND);
  EXPECT_EQ(LOR + 1, EQL);
  EXPECT_EQ(EQL + 1, LSS);
}

TEST(OpTest, AddAttr) {
  OperatorAttr expected_attr = LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR | HAS_ID_ELEMENT_ATTR;
  OperatorAttr actual_attr = OperatorInfo<ADD>::attr;
  EXPECT_EQ(expected_attr, actual_attr);
}

TEST(OpTest, LssAttr) {
  OperatorAttr expected_attr = CLEAR_ATTR;
  OperatorAttr actual_attr = OperatorInfo<LSS>::attr;
  EXPECT_EQ(expected_attr, actual_attr);
}

TEST(OpTest, AttrFunctions) {
  EXPECT_TRUE(OperatorInfo<NOT>::is_unary());

  EXPECT_TRUE(OperatorInfo<ADD>::is_commutative());
  EXPECT_TRUE(OperatorInfo<ADD>::is_associative());
  EXPECT_TRUE(OperatorInfo<ADD>::has_identity());
  EXPECT_FALSE(OperatorInfo<ADD>::is_unary());

  EXPECT_TRUE(OperatorInfo<LAND>::is_commutative());
  EXPECT_TRUE(OperatorInfo<LAND>::is_associative());
  EXPECT_TRUE(OperatorInfo<LAND>::has_identity());
  EXPECT_FALSE(OperatorInfo<LAND>::is_unary());

  EXPECT_TRUE(OperatorInfo<LOR>::is_commutative());
  EXPECT_TRUE(OperatorInfo<LOR>::is_associative());
  EXPECT_TRUE(OperatorInfo<LOR>::has_identity());
  EXPECT_FALSE(OperatorInfo<LOR>::is_unary());

  EXPECT_TRUE(OperatorInfo<EQL>::is_commutative());
  EXPECT_TRUE(OperatorInfo<EQL>::is_associative());
  EXPECT_FALSE(OperatorInfo<EQL>::has_identity());
  EXPECT_FALSE(OperatorInfo<EQL>::is_unary());

  EXPECT_FALSE(OperatorInfo<LSS>::is_commutative());
  EXPECT_FALSE(OperatorInfo<LSS>::is_associative());
  EXPECT_FALSE(OperatorInfo<LSS>::has_identity());
  EXPECT_FALSE(OperatorInfo<LSS>::is_unary());

  EXPECT_TRUE(get_commutative_attr(COMM_ATTR));
  EXPECT_FALSE(get_associative_attr(COMM_ATTR));
  EXPECT_FALSE(get_identity_attr(COMM_ATTR));
  EXPECT_FALSE(get_unary_attr(COMM_ATTR));

  EXPECT_FALSE(get_commutative_attr(LASSOC_ATTR));
  EXPECT_FALSE(get_associative_attr(LASSOC_ATTR));
  EXPECT_FALSE(get_identity_attr(LASSOC_ATTR));
  EXPECT_FALSE(get_unary_attr(LASSOC_ATTR));

  EXPECT_FALSE(get_commutative_attr(RASSOC_ATTR));
  EXPECT_FALSE(get_associative_attr(RASSOC_ATTR));
  EXPECT_FALSE(get_identity_attr(RASSOC_ATTR));
  EXPECT_FALSE(get_unary_attr(RASSOC_ATTR));

  EXPECT_FALSE(get_commutative_attr(LASSOC_ATTR | RASSOC_ATTR));
  EXPECT_TRUE(get_associative_attr(LASSOC_ATTR | RASSOC_ATTR));
  EXPECT_FALSE(get_identity_attr(LASSOC_ATTR | RASSOC_ATTR));
  EXPECT_FALSE(get_unary_attr(LASSOC_ATTR | RASSOC_ATTR));

  EXPECT_FALSE(get_commutative_attr(UNARY_ATTR));
  EXPECT_FALSE(get_associative_attr(UNARY_ATTR));
  EXPECT_FALSE(get_identity_attr(UNARY_ATTR));
  EXPECT_TRUE(get_unary_attr(UNARY_ATTR));

  EXPECT_FALSE(get_commutative_attr(CLEAR_ATTR));
  EXPECT_FALSE(get_associative_attr(CLEAR_ATTR));
  EXPECT_FALSE(get_identity_attr(CLEAR_ATTR));
  EXPECT_FALSE(get_unary_attr(CLEAR_ATTR));
}

// See diagram in include/op.h
TEST(OpTest, OperatorEnumLayout) {
  EXPECT_EQ(NOT, UNARY_BEGIN);
  EXPECT_EQ(ADD, UNARY_END);
  EXPECT_EQ(ADD, NARY_BEGIN);
  EXPECT_EQ(LSS, NARY_END);
}

