#ifndef __ZIPRUBY_STAT_H__
#define __ZIPRUBY_STAT_H__

#include "zip.h"
#include "ruby.h"

struct zipruby_stat {
  struct zip_stat *sb;
};

void Init_zipruby_stat();

VALUE zipruby_stat_name(VALUE self);
VALUE zipruby_stat_index(VALUE self);
VALUE zipruby_stat_crc(VALUE self);
VALUE zipruby_stat_size(VALUE self);
VALUE zipruby_stat_mtime(VALUE self);
VALUE zipruby_stat_comp_size(VALUE self);
VALUE zipruby_stat_comp_method(VALUE self);
VALUE zipruby_stat_encryption_method(VALUE self);
VALUE zipruby_stat_is_directory(VALUE self);

#endif
