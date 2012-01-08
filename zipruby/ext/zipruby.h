#ifndef __ZIPRUBY_H__
#define __ZIPRUBY_H__

#include "ruby.h"

#ifndef RSTRING_PTR
#define RSTRING_PTR(s) (RSTRING(s)->ptr)
#endif
#ifndef RSTRING_LEN
#define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

#define Check_IO(x) do { \
  const char *classname = rb_class2name(CLASS_OF(x)); \
  if (rb_obj_is_kind_of((x), rb_cIO)) { \
    rb_io_binmode(x); \
  } else if (strcmp(classname, "StringIO") != 0) { \
    rb_raise(rb_eTypeError, "wrong argument type %s (expected IO or StringIO)", classname); \
  } \
} while(0)

#define TIME2LONG(v) NUM2LONG(rb_funcall((v), rb_intern("tv_sec"), 0))

#define VERSION "0.3.6"
#define ERRSTR_BUFSIZE 256
#define DATA_BUFSIZE 8192

void Init_zipruby();

#endif
