// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_TYPE_H_
#define LIBSE_TYPE_H_

#include <utility>
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

}

#endif
