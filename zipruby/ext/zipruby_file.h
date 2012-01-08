#ifndef __ZIPRUBY_FILE_H__
#define __ZIPRUBY_FILE_H__

#include "zip.h"
#include "ruby.h"

struct zipruby_file {
  VALUE v_archive;
  struct zip *archive;
  struct zip_file *file;
  VALUE v_sb;
  struct zip_stat *sb;
};

void Init_zipruby_file();

#define Check_File(p) do { \
  if ((p)->archive == NULL || (p)->file == NULL || (p)->sb == NULL) { \
    rb_raise(rb_eRuntimeError, "invalid Zip::File"); \
  } \
} while(0)

#endif
