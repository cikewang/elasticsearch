dnl $Id$
dnl config.m4 for extension elasticsearch

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(elasticsearch, for elasticsearch support,
dnl Make sure that the comment is aligned:
dnl [  --with-elasticsearch             Include elasticsearch support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(elasticsearch, whether to enable elasticsearch support,
Make sure that the comment is aligned:
[  --enable-elasticsearch           Enable elasticsearch support])

if test "$PHP_ELASTICSEARCH" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-elasticsearch -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/elasticsearch.h"  # you most likely want to change this
  dnl if test -r $PHP_ELASTICSEARCH/$SEARCH_FOR; then # path given as parameter
  dnl   ELASTICSEARCH_DIR=$PHP_ELASTICSEARCH
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for elasticsearch files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       ELASTICSEARCH_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$ELASTICSEARCH_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the elasticsearch distribution])
  dnl fi

  dnl # --with-elasticsearch -> add include path
  PHP_ADD_INCLUDE(/usr/include)
  PHP_ADD_LIBRARY_WITH_PATH(curl, /usr/$PHP_LIBDIR, ELASTICSEARCH_SHARED_LIBADD)

  dnl # --with-elasticsearch -> check for lib and symbol presence
  dnl LIBNAME=elasticsearch # you may want to change this
  dnl LIBSYMBOL=elasticsearch # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $ELASTICSEARCH_DIR/$PHP_LIBDIR, ELASTICSEARCH_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_ELASTICSEARCHLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong elasticsearch lib version or lib not found])
  dnl ],[
  dnl   -L$ELASTICSEARCH_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  PHP_SUBST(ELASTICSEARCH_SHARED_LIBADD)

  PHP_NEW_EXTENSION(elasticsearch, elasticsearch.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi


