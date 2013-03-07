// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_TYPE_H_
#define LIBSE_TYPE_H_

#include <limits>
#include <utility>
#include <cassert>
#include <type_traits>

#include "core/op.h"
#include "core/eval.h"

namespace se {

template<Operator op, typename ...T> struct ReturnType;

template<Operator op, typename T>
struct ReturnType<op, T> {
  typedef decltype(Eval<op>::template const_eval<T>(std::declval<T>())) result_type;
};

template<Operator op, typename T, typename U>
struct ReturnType<op, T, U> {
  typedef decltype(Eval<op>::template const_eval<T, U>(std::declval<T>(), std::declval<U>())) result_type;
};

struct __EmptyUnwrapType {};
template<typename T> struct __UnwrapArithmeticType {
  typedef typename std::enable_if<std::is_arithmetic<T>::value, T>::type base;
};

template<typename T>
struct UnwrapType : std::conditional<
  /* if */ std::is_arithmetic<T>::value,
  /* then */ __UnwrapArithmeticType<T>,
  /* else */ __EmptyUnwrapType>
              ::type {};

template<typename T>
struct UnwrapType<T*> {
  typedef typename UnwrapType<T>::base* base;
};

template<typename T>
struct UnwrapType<T* const> {
  typedef typename UnwrapType<T>::base* const base;
};

template<typename T>
struct UnwrapType<T* volatile> {
  typedef typename UnwrapType<T>::base* volatile base;
};

template<typename T>
struct UnwrapType<T* const volatile> {
  typedef typename UnwrapType<T>::base* const volatile base;
};

/// Runtime type information
class Type {
private:
  const size_t m_bv_size;
  const bool m_is_signed;
  const bool m_is_pointer;
  const Type* const m_pointer_type;

  template<typename T>
  friend struct TypeInfo;

  constexpr Type(
    size_t bv_size, bool is_signed,
    bool is_pointer, const Type* const pointer_type) :
    m_bv_size(bv_size), m_is_signed(is_signed),
    m_is_pointer(is_pointer), m_pointer_type(pointer_type) {}

public:
  size_t bv_size() const { return m_bv_size; }
  bool is_signed() const { return m_is_signed; }
  bool is_pointer() const { return m_is_pointer; }

  /// \pre is_pointer() is true
  const Type& pointer_type() const {
    assert(m_is_pointer); return *m_pointer_type;
  }
};

/// Lookup table of statically allocated Type objects
template<typename T> struct TypeInfo { static Type s_type; };
template<typename T> constexpr size_t bv_size() { return 8 * sizeof(T); }

template<typename T>
Type TypeInfo<T>::s_type(bv_size<T>(),
                         std::is_signed<T>::value,
                         std::is_pointer<T>::value,

                         /* undefined memory address if T is not a pointer type */
                         &TypeInfo<typename std::remove_pointer<T>::type>::s_type);

}

#endif
