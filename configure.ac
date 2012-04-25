AC_INIT([pvr], [0.0.1], [mignov@gmail.com])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror])

AC_PROG_CC
AM_PROG_CC_C_O
AC_CHECK_LIB([virt], [virConnectOpen], [], AC_MSG_ERROR([You need libvirt to compile this application]))
AC_CHECK_LIB([xml2], [xmlParseFile], [], AC_MSG_ERROR([You need libxml2 to compile this application]))
AC_CHECK_LIB([blkid], [blkid_get_devx], [AM_CONDITIONAL([H_BLKID], [true])], [AM_CONDITIONAL([H_BLKID], [false])])
AC_CHECK_LIB([gtk-vnc-1.0], [vnc_display_new], [AM_CONDITIONAL([H_GTKVNC], [true])], [AM_CONDITIONAL([H_GTKVNC], [false])])
AC_CHECK_TOOL([MKDIR], [mkdir])
AC_CHECK_TOOL([ECHO], [echo])
AC_CHECK_TOOL([RM], [rm])
AC_CHECK_TOOL([CP], [cp])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
