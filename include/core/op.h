// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef OP_H_
#define OP_H_

namespace se {

/// Built-in C++ operators for which symbolic execution is supported

/// Operators are ordered according to their arity. This ordering can be used
/// to create fast runtime lookup tables. To facilitate this, there are extern
/// constants that mark the first and last n-arity operator in this enum type.
/// Since some nary operators are also unary (e.g. \ref ADD), these delimiter
/// values can overlap. The last unary operator marks the beginning of operators
/// that never accept less than two operands (e.g. \ref LSS). The following
/// schematic diagram illustrates this:
///
///     + - - - - - - - - - - - - - - - - - - - + <- 0 (unsigned short)
///     |                Future Use             |
///     + - - - - - - - - - - - - - - - - - - - + <- UNARY_BEGIN
///     |                                       |
///     |    Unary Operators (e.g. NOT, ADD)    |
///     |                                       |
///     + - - - - - - - - - - - - - - - - - - - + <- NARY_BEGIN
///     |                                       |
///     |   Unary & Nary Operators (e.g. ADD)   |
///     |                                       |
///     + - - - - - - - - - - - - - - - - - - - + <- UNARY_END
///     |                                       |  
///     |         Binary & Nary Operators       |
///     |            (e.g. LSS, ADD)            |
///     |                                       |
///     + - - - - - - - - - - - - - - - - - - - + <- NARY_END
///     |                  ...                  |
///
///
/// Note that all operators whose enum value is greater than \ref NARY_BEGIN
/// and less than \ref UNARY_END should be associative unless otherwise
/// specified through the \ref OperatorAttr "operator's attributes".

// Maintainer notice: The order of these operators determines the order of
// the elements in the internal operators string array.
//
// TODO: Account for signed LSS etc.
enum Opcode : unsigned short {
  /// \verbatim !\endverbatim
  NOT,

  /// \verbatim +\endverbatim
  ADD,
  /// \verbatim -\endverbatim
  SUB,

  /// \verbatim &&\endverbatim
  LAND,
  /// \verbatim ||\endverbatim
  LOR,

  /// \verbatim =\endverbatim
  EQL,
  /// \verbatim <\endverbatim
  LSS,
};

/// First unary operator

/// \ref UNARY_BEGIN is always less than or equal to \ref UNARY_END.
/// \see Opcode
constexpr Opcode UNARY_BEGIN = NOT;

/// Last unary operator

/// \ref UNARY_END is always less than or equal \ref NARY_END.
/// \see Opcode
constexpr Opcode UNARY_END = SUB;

/// First nary operator

/// \ref NARY_BEGIN is always less than or equal to \ref NARY_END.
/// \see Opcode 
constexpr Opcode NARY_BEGIN = ADD;

/// Last binary operator

/// The range \ref UNARY_END ... \ref NARY_END gives all the operators which
/// require at least two operands.
/// \see Opcode
constexpr Opcode NARY_END = LSS;

/// Bitmask to describe the mathematical properties of an operator
typedef unsigned char OperatorAttr;

/// Bit vector to define and inspect an operator's mathematical properties

/// Use bitwise operators judiciously to combine mathematical properties into
/// a bitmask of type \ref OperatorAttr. Unusual value combinations should be
/// avoided. For example, it is uncommon for an operator to be non-associative
/// but commutative (such an operator is also known as a magma).
///
/// \remark if both \ref LASSOC_ATTR and \ref RASSOC_ATTR are set, then the
///         operator must be associative, i.e. ((x ~ y) ~ z) = (x ~ (y ~ z))
enum OperatorAttrBit : OperatorAttr {
  /// Clear all other attributes via bitwise AND
  CLEAR_ATTR = 0u,

  /// Operator is left associative, i.e. x ~ y ~ z = (x ~ y) ~ z
  LASSOC_ATTR = (1u << 2),

  /// Operator is right associative, i.e. x ~ y ~ z = x ~ (y ~ z)
  RASSOC_ATTR = (1u << 1),

  /// Operator is commutative, i.e. (x ~ y) = (y ~ x)
  COMM_ATTR = (1u << 3),

  /// Operator has a unique identity element e, i.e. x ~ e = e ~ x = x
  HAS_ID_ELEMENT_ATTR = (1u << 4),

  /// Operator is always unary, e.g. NOT
  UNARY_ATTR = (1u << 5)
};

/// Compile-time information about an operator
class Operator {
private:
  const Opcode m_opcode;
  const OperatorAttr m_attr;

  template<Opcode opcode> friend class OperatorInfo;
  constexpr Operator(Opcode opcode, OperatorAttr attr) :
    m_opcode(opcode), m_attr(attr) {}

public:
  constexpr Opcode opcode() const { return m_opcode; }

  constexpr bool is_commutative() const {
    return m_attr & COMM_ATTR;
  }

  constexpr bool is_associative() const {
    return (m_attr & (LASSOC_ATTR | RASSOC_ATTR)) == (LASSOC_ATTR | RASSOC_ATTR);
  }

  constexpr bool has_identity() const {
    return (m_attr & HAS_ID_ELEMENT_ATTR) == HAS_ID_ELEMENT_ATTR;
  }

  constexpr bool is_unary() const {
    return (m_attr & UNARY_ATTR) == UNARY_ATTR;
  }

  constexpr bool is_commutative_monoid() const {
    return is_commutative() && is_associative() && has_identity();
  }
};
/// Static lookup function to find an operator's mathematical properties

/// Example:
///
///      OperatorInfo<ADD>::s_op.is_associative()
///
/// \see Opcode 
template<Opcode opcode> class OperatorInfo {};

/// Define an operator's mathematical properties with a bit vector
#define OPERATOR_INFO_DEF(opcode, attr)\
  template<>\
  struct OperatorInfo<opcode> {\
    static constexpr Operator s_op = Operator(opcode, attr);\
  };

// TODO: Consider using another bit mask for floats etc.
OPERATOR_INFO_DEF(NOT,  UNARY_ATTR | CLEAR_ATTR)
OPERATOR_INFO_DEF(ADD,  LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR | HAS_ID_ELEMENT_ATTR)
OPERATOR_INFO_DEF(SUB,  LASSOC_ATTR | HAS_ID_ELEMENT_ATTR)
OPERATOR_INFO_DEF(LAND, LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR | HAS_ID_ELEMENT_ATTR)
OPERATOR_INFO_DEF(LOR,  LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR | HAS_ID_ELEMENT_ATTR)
OPERATOR_INFO_DEF(EQL,  LASSOC_ATTR | RASSOC_ATTR | COMM_ATTR)
OPERATOR_INFO_DEF(LSS,  CLEAR_ATTR)

}

#endif /* OP_H_ */
