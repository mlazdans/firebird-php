PHP_ARG_WITH([firebird],
  [for Firebird support],
  [AS_HELP_STRING([[--with-firebird[=DIR]]],
    [Include Firebird support. DIR is the Firebird base install directory
    [/opt/firebird]])])

if test "$PHP_FIREBIRD" != "no"; then

  AC_PATH_PROG(FB_CONFIG, fb_config, no)

  if test -x "$FB_CONFIG" && test "$PHP_FIREBIRD" = "yes"; then
    AC_MSG_CHECKING(for libfbconfig)
    FB_CFLAGS=`$FB_CONFIG --cflags`
    FB_LIBDIR=`$FB_CONFIG --libs`
    FB_VERSION=`$FB_CONFIG --version`
    AC_MSG_RESULT(version $FB_VERSION)
    PHP_EVAL_LIBLINE($FB_LIBDIR, FIREBIRD_SHARED_LIBADD)
    PHP_EVAL_INCLINE($FB_CFLAGS)

  else
    if test "$PHP_FIREBIRD" = "yes"; then
      FIREBIRD_INCDIR=/opt/firebird/include
      FIREBIRD_LIBDIR=/opt/firebird/lib
    else
      FIREBIRD_INCDIR=$PHP_FIREBIRD/include
      FIREBIRD_LIBDIR=$PHP_FIREBIRD/$PHP_LIBDIR
    fi

    PHP_CHECK_LIBRARY(fbclient, isc_detach_database,
    [
      FIREBIRD_LIBNAME=fbclient
    ], [
      PHP_CHECK_LIBRARY(gds, isc_detach_database,
      [
        FIREBIRD_LIBNAME=gds
      ], [
        PHP_CHECK_LIBRARY(ib_util, isc_detach_database,
        [
          FIREBIRD_LIBNAME=ib_util
        ], [
          AC_MSG_ERROR([libfbclient, libgds or libib_util not found! Check config.log for more information.])
        ], [
          -L$FIREBIRD_LIBDIR
        ])
      ], [
        -L$FIREBIRD_LIBDIR
      ])
    ], [
      -L$FIREBIRD_LIBDIR
    ])

    PHP_ADD_LIBRARY_WITH_PATH($FIREBIRD_LIBNAME, $FIREBIRD_LIBDIR, FIREBIRD_SHARED_LIBADD)
    PHP_ADD_INCLUDE($FIREBIRD_INCDIR)
  fi

  AC_DEFINE(HAVE_FIREBIRD,1,[ ])
  PHP_NEW_EXTENSION(firebird, [firebird.c fbp_database.c database.c fbp_transaction.c transaction.c fbp_statement.c statement.c fbp_blob.c blob.c error.c args.c var_info.c event.c fbp_service.c service.c tbuilder.c], $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1,[cxx])
  PHP_SUBST(FIREBIRD_SHARED_LIBADD)

  PHP_REQUIRE_CXX()
  PHP_CXX_COMPILE_STDCXX([11], [mandatory], [PHP_FIREBIRD_STDCXX])

  PHP_FIREBIRD_CXX_SOURCES="pdo_firebird_utils.cpp"

  AS_VAR_IF([ext_shared], [no],
    [PHP_ADD_SOURCES([$ext_dir],
      [$PHP_FIREBIRD_CXX_SOURCES],
      [$PHP_FIREBIRD_STDCXX])],
    [PHP_ADD_SOURCES_X([$ext_dir],
      [$PHP_FIREBIRD_CXX_SOURCES],
      [$PHP_FIREBIRD_STDCXX],
      [shared_objects_firebird],
      [yes])])

fi
