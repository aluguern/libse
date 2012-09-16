# Provide various options to initialize the Autoconf and configure processes.
AC_PREREQ([2.59])

AC_INIT([libse], [0.2], [alex.horn@cs.ox.ac.uk])

AC_CONFIG_SRCDIR([src/expr.cpp])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([build-aux/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([gtest])

# Search for SMT and SAT solver libraries
CXXFLAGS="$CXXFLAGS -Isolvers/z3/include"
LDFLAGS="$LDFLAGS -Lsolvers/z3/lib"

AC_SEARCH_LIBS([Z3_mk_config], [z3], , AC_MSG_ERROR([Unable to find Z3 theorem prover]))

AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN";
   then AC_MSG_WARN([Doxygen not found - API documentation cannot be generated.])
fi

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([doc/Doxyfile])])

AM_INIT_AUTOMAKE([-Wall -Werror dist-zip foreign subdir-objects])

AC_PROG_CXX
AC_LANG([C++])
AC_PROG_LIBTOOL

# Output the generated files. No further autoconf macros may be used.
AC_OUTPUT
