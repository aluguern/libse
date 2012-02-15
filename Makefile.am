# Since everything we compile requires the same headers,
# we define the global AM_CPPFLAGS. We also require the
# C++11 standard to be able to use std::shared_ptr.
AM_CPPFLAGS = -std=c++0x -I$(srcdir)/include

AUTOMAKE_OPTIONS = foreign

# Always include gtest in distributions.
DIST_SUBDIRS = gtest

# The following local target definition is copied from the Protobuf project:
#   Build gtest before we build our tests. We don't add gtest to SUBDIRS
#   because then "make check" would also build and run all of gtest's own tests,
#   which takes a lot of time and is generally not useful to us. Also, we don't
#   want "make install" to recurse into gtest since we don't want to overwrite
#   the installed version of gtest if there is one.
check-local:
	@echo "Making lib/libgtest.a lib/libgtest_main.a in gtest"
	@cd gtest && $(MAKE) $(AM_MAKEFLAGS) lib/libgtest.la lib/libgtest_main.la

# We need to create a new custom target to run the tests because Autoconf does
# not reliably add check-local before the check target.
test: check-local check

bench: all
	git rev-parse HEAD >> bench/LOG
	time -p -o bench/LOG -a bench/loop

.PHONY: bench

# The following local target definition is copied from the Protobuf project:
#   We would like to clean gtest when "make clean" is invoked. But we have to
#   be careful because clean-local is also invoked during "make distclean", but
#   "make distclean" already recurses into gtest because it's listed among the
#   DIST_SUBDIRS. distclean will delete gtest/Makefile, so if we then try to
#   cd to the directory again and "make clean" it will fail. So, check that the
#   Makefile exists before recursing.
clean-local:
	@if test -e gtest/Makefile; then \
	  echo "Making clean in gtest"; \
	  cd gtest && $(MAKE) $(AM_MAKEFLAGS) clean; \
	fi

dist_doc_DATA = README

# Build rules for libraries.
lib_LTLIBRARIES = lib/libhegel.la

lib_libhegel_la_SOURCES = \
  src/expr.cpp \
  src/reflect.cpp \
  src/var.cpp \
  src/symtracer.cpp

pkginclude_HEADERS = \
  include/reflect.h \
  include/expr.h \
  include/var.h \
  include/overload.h \
  include/symtracer.h

# Bulid rules for functional and unit tests.
# Recall the Automake naming conventions:
#
# TESTS -- Programs run automatically by "make check"
# check_PROGRAMS -- Programs built by "make check" but not necessarily run

TESTS = test/hegel
check_PROGRAMS = test/hegel

test_hegel_SOURCES = \
  test/reflect_test.cpp \
  test/expr_test.cpp \
  test/var_test.cpp \
  test/symtracer_test.cpp \
  test/functional_test.cpp

test_hegel_CPPFLAGS = -std=c++0x -I$(srcdir)/include -I$(top_srcdir)/gtest/include
test_hegel_LDADD = $(top_builddir)/gtest/lib/libgtest_main.la lib/libhegel.la

# Benchmarks

bin_PROGRAMS = bench/loop
bench_loop_SOURCES = bench/loop_bench.cpp
bench_loop_CPPFLAGS = -std=c++0x -I$(srcdir)/include
bench_loop_LDADD = lib/libhegel.la
