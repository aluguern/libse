// Copyright 2013, Alex Horn. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef LIBSE_CONCURRENT_INSTR_H_
#define LIBSE_CONCURRENT_INSTR_H_

#include <memory>
#include <cassert>
#include <utility>
#include <forward_list>

#include "core/type.h"

#include "concurrent/event.h"

namespace z3 { class expr; }

namespace se {

class Z3;
class Z3ReadEncoder;

/// Non-copyable class that identifies a built-in memory read instruction

/// `T` refers to the type of the lvalue or rvalue being read. For example,
/// the right-hand side of the assignment `char y = xs[2]` consists of a
/// read instruction of the array pointer (i.e. `ReadInstr<char[]>`) followed
/// by another read instruction of its offset (i.e. `ReadInstr<size_t>`);
/// together, both read instructions must be coalesced into an object
/// whose type is `ReadInstr<char>` because variable `y` is of type `char`.
///
/// Any operands of a subclass of ReadInstr<T> must be only accessible
/// through a constant reference. If a subclass of ReadInstr<T> refers
/// to an event, then it must be of type ReadEvent<T>.
template<typename T>
class ReadInstr {
protected:
  ReadInstr() {}

public:
  ReadInstr(const ReadInstr& other) = delete;
  virtual ~ReadInstr() {}

  virtual void filter(std::forward_list<std::shared_ptr<Event>>&) const = 0;
  virtual z3::expr encode(const Z3ReadEncoder& encoder, Z3& helper) const = 0;

  virtual std::shared_ptr<ReadInstr<bool>> condition_ptr() const = 0;
};

#define DECL_READ_ENCODER_FN \
  z3::expr encode(const Z3ReadEncoder& encoder, Z3& helper) const;

template<typename T>
class LiteralReadInstr : public ReadInstr<T> {
private:
  const T m_literal;
  const std::shared_ptr<ReadInstr<bool>> m_condition;

protected:
  std::shared_ptr<ReadInstr<bool>> condition_ptr() const {
    return m_condition;
  }

public:
  LiteralReadInstr(const T& literal,
    const std::shared_ptr<ReadInstr<bool>>& condition = nullptr) :
    m_literal(literal), m_condition(condition) {}

  /// Zero literal
  LiteralReadInstr(const std::shared_ptr<ReadInstr<bool>>& condition = nullptr) :
    m_literal(0), m_condition(condition) {}

  LiteralReadInstr(const LiteralReadInstr& other) = delete;

  ~LiteralReadInstr() {}

  const T& literal() const { return m_literal; }

  void filter(std::forward_list<std::shared_ptr<Event>>&) const { /* skip */ }

  DECL_READ_ENCODER_FN
};

/// Array filled with identical literals
template<typename T, size_t N>
class LiteralReadInstr<T[N]> : public ReadInstr<T[N]> {
private:
  const T m_element_literal;
  const std::shared_ptr<ReadInstr<bool>> m_condition;

protected:
  std::shared_ptr<ReadInstr<bool>> condition_ptr() const {
    return m_condition;
  }

public:
  /// Initializes every array element to zero
  LiteralReadInstr(const std::shared_ptr<ReadInstr<bool>>& condition = nullptr) :
    m_element_literal(0), m_condition(condition) {}

  LiteralReadInstr(const LiteralReadInstr& other) = delete;

  ~LiteralReadInstr() {}

  const T element_literal() const { return m_element_literal; }

  void filter(std::forward_list<std::shared_ptr<Event>>&) const { /* skip */ }

  DECL_READ_ENCODER_FN
};

template<typename T>
class BasicReadInstr : public ReadInstr<T> {
private:
  std::shared_ptr<ReadEvent<T>> m_event_ptr;

protected:
  std::shared_ptr<ReadInstr<bool>> condition_ptr() const {
    return m_event_ptr->condition_ptr();
  }

public:
  /// Unique read event for shared variable access
  BasicReadInstr(std::unique_ptr<ReadEvent<T>> event_ptr) :
    m_event_ptr(std::move(event_ptr)) {}

  /// Shared read event for local variable access
  BasicReadInstr(std::shared_ptr<ReadEvent<T>> event_ptr) :
    m_event_ptr(event_ptr) {}

  BasicReadInstr(const BasicReadInstr& other) = delete;

  ~BasicReadInstr() {}

  std::shared_ptr<ReadEvent<T>> event_ptr() const { return m_event_ptr; }

  void filter(std::forward_list<std::shared_ptr<Event>>& event_ptrs) const {
    event_ptrs.push_front(m_event_ptr);
  }

  DECL_READ_ENCODER_FN
};

template<Operator op, typename U>
class UnaryReadInstr : public ReadInstr<typename ReturnType<op, U>::result_type> {
private:
  const std::shared_ptr<ReadInstr<U>> m_operand_ptr;

  friend class Recorder;
  UnaryReadInstr(std::shared_ptr<ReadInstr<U>> operand_ptr) :
    m_operand_ptr(operand_ptr) {}

protected:
  std::shared_ptr<ReadInstr<bool>> condition_ptr() const {
    return m_operand_ptr->condition_ptr();
  }

public:
  UnaryReadInstr(std::unique_ptr<ReadInstr<U>> operand_ptr) :
    m_operand_ptr(std::move(operand_ptr)) {}

  UnaryReadInstr(const UnaryReadInstr& other) = delete;

  ~UnaryReadInstr() {}

  const ReadInstr<U>& operand_ref() const { return *m_operand_ptr; }

  void filter(std::forward_list<std::shared_ptr<Event>>& event_ptrs) const {
    operand_ref().filter(event_ptrs);
  }

  DECL_READ_ENCODER_FN
};

template<Operator op, typename U, typename V>
class BinaryReadInstr :
  public ReadInstr<typename ReturnType<op, U, V>::result_type> {

private:
  const std::unique_ptr<ReadInstr<U>> m_loperand_ptr;
  const std::unique_ptr<ReadInstr<V>> m_roperand_ptr;

protected:
  std::shared_ptr<ReadInstr<bool>> condition_ptr() const {
    return m_loperand_ptr->condition_ptr();
  }

public:
  BinaryReadInstr(std::unique_ptr<ReadInstr<U>> loperand_ptr,
    std::unique_ptr<ReadInstr<V>> roperand_ptr) :
    m_loperand_ptr(std::move(loperand_ptr)),
    m_roperand_ptr(std::move(roperand_ptr)) {

    assert(m_loperand_ptr->condition_ptr() == m_roperand_ptr->condition_ptr());
  }

  BinaryReadInstr(const BinaryReadInstr& other) = delete;

  ~BinaryReadInstr() {}

  const ReadInstr<U>& loperand_ref() const { return *m_loperand_ptr; }
  const ReadInstr<V>& roperand_ref() const { return *m_roperand_ptr; }

  void filter(std::forward_list<std::shared_ptr<Event>>& event_ptrs) const {
    roperand_ref().filter(event_ptrs);
    loperand_ref().filter(event_ptrs);
  }

  DECL_READ_ENCODER_FN
};

/// Commutative monoid read instruction

/// The operator `op` must have an identity element and be associative and
/// commutative, i.e. `op` is a commutative monoid. For example, the logical
/// operator \ref LAND satisfies these properties. These and other operators
/// allow us to flatten the usual tree data structure to a set of operands.
template<Operator op, typename T>
class NaryReadInstr : public ReadInstr<T> {
private:
  // Never empty
  const std::forward_list<std::shared_ptr<ReadInstr<T>>> m_operand_ptrs;
  const size_t m_size;

protected:
  std::shared_ptr<ReadInstr<bool>> condition_ptr() const {
    return m_operand_ptrs.front()->condition_ptr();
  }

public:
  /// \pre: There are at least two operands
  NaryReadInstr(std::forward_list<std::shared_ptr<ReadInstr<T>>>&& operand_ptrs,
    size_t size) : m_operand_ptrs(std::move(operand_ptrs)), m_size(size) {

    assert(!m_operand_ptrs.empty());
    assert(1 < size);
  }

  NaryReadInstr(const NaryReadInstr& other) = delete;

  ~NaryReadInstr() {}

  /// Number of operands
  size_t size() const { return m_size; }

  const std::forward_list<std::shared_ptr<ReadInstr<T>>>& operand_ptrs() const {
    return m_operand_ptrs;
  }

  void filter(std::forward_list<std::shared_ptr<Event>>& event_ptrs) const {
    for (const std::shared_ptr<ReadInstr<T>>& instr_ptr : m_operand_ptrs) {
      instr_ptr->filter(event_ptrs);
    }
  }

  DECL_READ_ENCODER_FN
};

/// Load memory of type `T` at an offset of type `U`
template<typename T, typename U> class DerefReadInstr;

/// Select an element in a fixed-size array
template<typename T, typename U, size_t N>
class DerefReadInstr<T[N], U> : public ReadInstr<T> {
private:
  const std::unique_ptr<ReadInstr<T[N]>> m_array_ptr;
  const std::unique_ptr<ReadInstr<U>> m_index_ptr;

protected:
  std::shared_ptr<ReadInstr<bool>> condition_ptr() const {
    return m_array_ptr->condition_ptr();
  }

public:
  DerefReadInstr(std::unique_ptr<ReadInstr<T[N]>> array_ptr,
    std::unique_ptr<ReadInstr<U>> index_ptr) :
    m_array_ptr(std::move(array_ptr)), m_index_ptr(std::move(index_ptr)) {}

  const ReadInstr<T[N]>& memory_ref() const { return *m_array_ptr; }
  const ReadInstr<U>& offset_ref() const { return *m_index_ptr; }

  void filter(std::forward_list<std::shared_ptr<Event>>& event_ptrs) const {
    memory_ref().filter(event_ptrs);
    offset_ref().filter(event_ptrs);
  }

  DECL_READ_ENCODER_FN
};

template<typename ...T> struct ReadInstrResult;

template<typename T>
struct ReadInstrResult<LiteralReadInstr<T>> { typedef T Type; };

template<typename T>
struct ReadInstrResult<BasicReadInstr<T>> { typedef T Type; };

template<Operator op, typename T>
struct ReadInstrResult<UnaryReadInstr<op, T>> {
  typedef typename ReturnType<op, T>::result_type Type;
};

template<Operator op, typename T, typename U>
struct ReadInstrResult<BinaryReadInstr<op, T, U>> {
  typedef typename ReturnType<op, T, U>::result_type Type;
};

template<typename T, typename U, size_t N>
struct ReadInstrResult<DerefReadInstr<T[N], U>> { typedef T Type; };

/// Optional control flow over the exact type of a read instruction

/// \remark Can only deallocate ReadInstrSwitch<Subclass> through Subclass
template<typename Subclass, typename U>
class ReadInstrSwitch {
private:
  template<typename> struct template_true_type : std::true_type{};

  template<class T, class Arg0, class Arg1>
  static auto check_case_instr(Arg0&& arg0, Arg1&& arg1, int)
    -> template_true_type<decltype(std::declval<T>().case_instr(arg0, arg1))>;

  template<class T, class Arg0, class Arg1>
  static std::false_type check_case_instr(Arg0&&, Arg1&&, long);

  // For gcc earlier than 4.7
  template<class T> struct wrap { typedef T type; };

  template<class T, class Arg0, class Arg1>
  struct has_case_instr :
    wrap<decltype(check_case_instr<T>(std::declval<Arg0>(), std::declval<Arg1>(), 0))>::type {};

protected:
  ~ReadInstrSwitch() {}

  /// Subclass is allowed to reimplement this non-virtual member function

  /// \remark Default implementation does nothing
  template<typename T, class = typename std::enable_if<
    /* if */ std::is_base_of<ReadInstr<typename ReadInstrResult<T>::Type>, T>::value,
    /* then */ T>::type>
  void case_instr(const T& instr, U& update) const {}

public:
  /// Statically dispatches to Subclass if allowed

  /// \warning Subclass must not reimplement this non-virtual member function
  template<typename T, class = typename std::enable_if<
    /* if */ std::is_base_of<ReadInstr<typename ReadInstrResult<T>::Type>, T>::value,
    /* then */ T>::type>
  void switch_instr(const T& instr, U& update) const {
    typedef typename std::conditional<
      /* if */ has_case_instr<Subclass, T, U>::value,
      /* then */ const Subclass*,
      /* else */ const ReadInstrSwitch*>
      ::type Case;

    static_cast<Case>(this)->case_instr(instr, update);
  }
};

}

#endif
