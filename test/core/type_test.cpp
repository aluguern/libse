#include "gtest/gtest.h"
#include "core/type.h"

using namespace se;

#define STATIC_EXPECT_TRUE(assertion) static_assert((assertion), "")

TEST(TypeTest, ReturnType) {
  STATIC_EXPECT_TRUE((std::is_same<unsigned long, typename ReturnType<ADD,
    unsigned int, unsigned long>::result_type>::value));

  STATIC_EXPECT_TRUE((std::is_same<bool, typename ReturnType<LSS,
    int, int>::result_type>::value));

  STATIC_EXPECT_TRUE((std::is_same<bool, typename ReturnType<NOT,
    unsigned int>::result_type>::value));
}

namespace se {

template<typename T> class Foo {};
template<typename T> struct UnwrapType<Foo<T>> { typedef T base; };

}

TEST(TypeTest, UnwrapType) {
  STATIC_EXPECT_TRUE((std::is_same<long,
    typename UnwrapType<Foo<long>>::base>::value));

  STATIC_EXPECT_TRUE((std::is_same<long,
    typename UnwrapType<long>::base>::value));

  STATIC_EXPECT_TRUE((std::is_same<long*,
    typename UnwrapType<long*>::base>::value));

  STATIC_EXPECT_TRUE((std::is_same<bool,
    typename UnwrapType<bool>::base>::value));
}

