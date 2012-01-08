#ifndef __ZIPRUBY_ZIP_SOURCE_IO_H__
#define __ZIPRUBY_ZIP_SOURCE_IO_H__

#include "zip.h"
#include "ruby.h"

struct read_io {
  VALUE io;
  long mtime;
};

struct zip_source *zip_source_io(struct zip *za, struct read_io *z);

#endif
