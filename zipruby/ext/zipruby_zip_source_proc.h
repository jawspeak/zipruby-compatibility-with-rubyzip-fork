#ifndef __ZIPRUBY_ZIP_SOURCE_PROC_H__
#define __ZIPRUBY_ZIP_SOURCE_PROC_H__

#include "zip.h"
#include "ruby.h"

struct read_proc {
  VALUE proc;
  long mtime;
};

struct zip_source *zip_source_proc(struct zip *za, struct read_proc *z);

#endif
