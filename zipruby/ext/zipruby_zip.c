#include <zlib.h>

#include "ruby.h"
#include "zip.h"
#include "zipruby.h"
#include "zipruby_zip.h"

VALUE Zip;

void Init_zipruby_zip() {
  Zip = rb_define_module("ZipRuby");
  rb_define_const(Zip, "VERSION", rb_str_new2(VERSION));

  rb_define_const(Zip, "CREATE",    INT2NUM(ZIP_CREATE));
  rb_define_const(Zip, "EXCL",      INT2NUM(ZIP_EXCL));
  rb_define_const(Zip, "CHECKCONS", INT2NUM(ZIP_CHECKCONS));
  rb_define_const(Zip, "TRUNC",     INT2NUM(ZIP_TRUNC));

  rb_define_const(Zip, "FL_NOCASE",     INT2NUM(ZIP_FL_NOCASE));
  rb_define_const(Zip, "FL_NODIR",      INT2NUM(ZIP_FL_NODIR));
  rb_define_const(Zip, "FL_COMPRESSED", INT2NUM(ZIP_FL_COMPRESSED));
  rb_define_const(Zip, "FL_UNCHANGED",  INT2NUM(ZIP_FL_UNCHANGED));

  rb_define_const(Zip, "CM_DEFAULT"   ,     INT2NUM(ZIP_CM_DEFAULT));
  rb_define_const(Zip, "CM_STORE",          INT2NUM(ZIP_CM_STORE));
  rb_define_const(Zip, "CM_SHRINK",         INT2NUM(ZIP_CM_SHRINK));
  rb_define_const(Zip, "CM_REDUCE_1",       INT2NUM(ZIP_CM_REDUCE_1));
  rb_define_const(Zip, "CM_REDUCE_2",       INT2NUM(ZIP_CM_REDUCE_2));
  rb_define_const(Zip, "CM_REDUCE_3",       INT2NUM(ZIP_CM_REDUCE_3));
  rb_define_const(Zip, "CM_REDUCE_4",       INT2NUM(ZIP_CM_REDUCE_4));
  rb_define_const(Zip, "CM_IMPLODE",        INT2NUM(ZIP_CM_IMPLODE));
  rb_define_const(Zip, "CM_DEFLATE",        INT2NUM(ZIP_CM_DEFLATE));
  rb_define_const(Zip, "CM_DEFLATE64",      INT2NUM(ZIP_CM_DEFLATE64));
  rb_define_const(Zip, "CM_PKWARE_IMPLODE", INT2NUM(ZIP_CM_PKWARE_IMPLODE));
  rb_define_const(Zip, "CM_BZIP2",          INT2NUM(ZIP_CM_BZIP2));

  rb_define_const(Zip, "EM_NONE",        INT2NUM(ZIP_EM_NONE));
  rb_define_const(Zip, "EM_TRAD_PKWARE", INT2NUM(ZIP_EM_TRAD_PKWARE));
  // XXX: Strong Encryption Header not parsed yet

  rb_define_const(Zip, "NO_COMPRESSION",      INT2NUM(Z_NO_COMPRESSION));
  rb_define_const(Zip, "BEST_SPEED",          INT2NUM(Z_BEST_SPEED));
  rb_define_const(Zip, "BEST_COMPRESSION",    INT2NUM(Z_BEST_COMPRESSION));
  rb_define_const(Zip, "DEFAULT_COMPRESSION", INT2NUM(Z_DEFAULT_COMPRESSION));
}
