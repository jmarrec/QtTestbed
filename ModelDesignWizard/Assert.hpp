#ifndef UTILITIES_CORE_ASSERT_HPP
#define UTILITIES_CORE_ASSERT_HPP

#define OS_ASSERT(expr) BOOST_ASSERT(expr)
#define BOOST_DISABLE_ASSERTS
#include <boost/assert.hpp>
#include <boost/regex.hpp>

#endif  // UTILITIES_CORE_ASSERT_HPP
