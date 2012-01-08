#ifndef __ZIPRUBY_ARCHIVE_H__
#define __ZIPRUBY_ARCHIVE_H__

#include "zip.h"
#include "ruby.h"

struct zipruby_archive {
  struct zip *archive;
  VALUE path;
  int flags;
  char *tmpfilnam;
  VALUE buffer;
  VALUE sources;
};

void Init_zipruby_archive();

#define Check_Archive(p) do { \
  if ((p)->archive == NULL || NIL_P((p)->path)) { \
    rb_raise(rb_eRuntimeError, "invalid Zip::Archive"); \
  } \
} while(0)

#endif
