# To support AC_CHECK_DEFINED
ACLOCAL_AMFLAGS = -I m4

# Since everything we compile requires the same headers,
# we define the global AM_CPPFLAGS. We also require the
# C++11 standard to be able to use std::shared_ptr.
AM_CPPFLAGS = -I$(srcdir)/include

AUTOMAKE_OPTIONS = foreign

# Always include gtest in distributions.
DIST_SUBDIRS = gtest

if HAVE_DOXYGEN

doc:
	$(DOXYGEN) $(top_srcdir)/doc/Doxyfile

endif

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
	time -p bench/if
	time -p bench/fib_005_safe
	time -p bench/fib_005_unsafe
	time -p bench/fib_006_safe
	time -p bench/fib_006_unsafe
	time -p bench/stateful01_safe
	time -p bench/stateful01_unsafe
	time -p bench/stack_007_slice_safe
	time -p bench/stack_007_slice_unsafe

.PHONY: bench doc

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

dist_doc_DATA = README.md

# Build rules for libraries.
lib_LTLIBRARIES = lib/libse.la

lib_libse_la_SOURCES = \
  src/core/op.cpp \
  src/concurrent/memory.cpp \
  src/concurrent/event.cpp \
  src/concurrent/encoder.cpp \
  src/concurrent/recorder.cpp \
  src/concurrent/relation.cpp \
  src/concurrent/thread.cpp \
  src/libse.cpp

pkginclude_HEADERS = \
  include/core/op.h \
  include/core/eval.h \
  include/core/type.h \
  include/concurrent/memory.h \
  include/concurrent/event.h \
  include/concurrent/instr.h \
  include/concurrent/block.h \
  include/concurrent/encoder.h \
  include/concurrent/recorder.h \
  include/concurrent/var.h \
  include/concurrent/relation.h \
  include/concurrent/thread.h \
  include/concurrent/mutex.h \
  include/concurrent.h \
  include/libse.h

# Build rules for functional and unit tests.
# Recall the Automake naming conventions:
#
# TESTS -- Programs run automatically by "make check"
# check_PROGRAMS -- Programs built by "make check" but not necessarily run

# See README in Z3 source code for installation instructions
tests_library_path = $(srcdir)/solvers/z3/build

TESTS_ENVIRONMENT = \
  LIBPATH=$(tests_library_path):$$LIBPATH \
  LD_LIBRARY_PATH=$(tests_library_path):$$LD_LIBRARY_PATH \
  DYLD_LIBRARY_PATH=$(tests_library_path):$$DYLD_LIBRARY_PATH \
  LIBRARY_PATH=$(tests_library_path):$$LIBRARY_PATH \
  SHLIB_PATH=$(tests_library_path):$$SHLIB_PATH \
  LD_LIBRARY64_PATH=$(tests_library_path):$$LD_LIBRARY64_PATH \
  LD_LIBRARYN32_PATH=$(tests_library_path):$$LD_LIBRARYN32_PATH \
  LD_LIBRARY_PATH_32=$(tests_library_path):$$LD_LIBRARY_PATH_32 \
  LD_LIBRARY_PATH_64=$(tests_library_path):$$LD_LIBRARY_PATH_64

TESTS = test/se
check_PROGRAMS = test/se

test_se_SOURCES = \
  test/core/op_test.cpp \
  test/core/eval_test.cpp \
  test/core/type_test.cpp \
  test/concurrent/memory_test.cpp \
  test/concurrent/event_test.cpp \
  test/concurrent/instr_test.cpp \
  test/concurrent/block_test.cpp \
  test/concurrent/encoder_test.cpp \
  test/concurrent/recorder_test.cpp \
  test/concurrent/var_test.cpp \
  test/concurrent/relation_test.cpp \
  test/concurrent/thread_test.cpp \
  test/concurrent/mutex_test.cpp \
  test/concurrent_test.cpp \
  test/concurrent/functional_test.cpp

test_se_CPPFLAGS = -std=c++0x -I$(srcdir)/include -I$(top_srcdir)/gtest/include
test_se_LDADD = $(top_builddir)/gtest/lib/libgtest_main.la lib/libse.la

# Benchmarks

bin_PROGRAMS = bench/if \
               bench/fib_005_safe \
               bench/fib_005_unsafe \
               bench/fib_006_safe \
               bench/fib_006_unsafe \
               bench/stateful01_safe \
               bench/stateful01_unsafe \
               bench/stack_007_slice_safe \
               bench/stack_007_slice_unsafe

bench_if_SOURCES = bench/if_bench.cpp
bench_if_CPPFLAGS = -std=c++0x -I$(srcdir)/include
bench_if_LDADD = lib/libse.la

bench_fib_005_safe_SOURCES = bench/fib_005_safe_bench.cpp
bench_fib_005_safe_CPPFLAGS = -std=c++0x -I$(srcdir)/include
bench_fib_005_safe_LDADD = lib/libse.la

bench_fib_005_unsafe_SOURCES = bench/fib_005_unsafe_bench.cpp
bench_fib_005_unsafe_CPPFLAGS = -std=c++0x -I$(srcdir)/include
bench_fib_005_unsafe_LDADD = lib/libse.la

bench_fib_006_safe_SOURCES = bench/fib_006_safe_bench.cpp
bench_fib_006_safe_CPPFLAGS = -std=c++0x -I$(srcdir)/include
bench_fib_006_safe_LDADD = lib/libse.la

bench_fib_006_unsafe_SOURCES = bench/fib_006_unsafe_bench.cpp
bench_fib_006_unsafe_CPPFLAGS = -std=c++0x -I$(srcdir)/include
bench_fib_006_unsafe_LDADD = lib/libse.la

bench_stateful01_safe_SOURCES = bench/stateful01_safe_bench.cpp
bench_stateful01_safe_CPPFLAGS = -std=c++0x -I$(srcdir)/include
bench_stateful01_safe_LDADD = lib/libse.la

bench_stateful01_unsafe_SOURCES = bench/stateful01_unsafe_bench.cpp
bench_stateful01_unsafe_CPPFLAGS = -std=c++0x -I$(srcdir)/include
bench_stateful01_unsafe_LDADD = lib/libse.la

bench_stack_007_slice_safe_SOURCES = bench/stack_007_slice_safe_bench.cpp
bench_stack_007_slice_safe_CPPFLAGS = -std=c++0x -I$(srcdir)/include
bench_stack_007_slice_safe_LDADD = lib/libse.la

bench_stack_007_slice_unsafe_SOURCES = bench/stack_007_slice_unsafe_bench.cpp
bench_stack_007_slice_unsafe_CPPFLAGS = -std=c++0x -I$(srcdir)/include
bench_stack_007_slice_unsafe_LDADD = lib/libse.la
