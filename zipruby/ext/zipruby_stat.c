#include <string.h>

#include "zip.h"
#include "zipruby.h"
#include "zipruby_archive.h"
#include "zipruby_stat.h"
#include "ruby.h"

static VALUE zipruby_stat_alloc(VALUE klass);
static void zipruby_stat_free(struct zipruby_stat *p);
static VALUE zipruby_stat_initialize(int argc, VALUE *argv, VALUE self);

extern VALUE Zip;
extern VALUE Archive;
VALUE Stat;
extern VALUE Error;

void Init_zipruby_stat() {
  Stat = rb_define_class_under(Zip, "Stat", rb_cObject);
  rb_define_alloc_func(Stat, zipruby_stat_alloc);
  rb_define_method(Stat, "initialize", zipruby_stat_initialize, -1);
  rb_define_method(Stat, "name", zipruby_stat_name, 0);
  rb_define_method(Stat, "index", zipruby_stat_index, 0);
  rb_define_method(Stat, "crc", zipruby_stat_crc, 0);
  rb_define_method(Stat, "size", zipruby_stat_size, 0);
  rb_define_method(Stat, "mtime", zipruby_stat_mtime, 0);
  rb_define_method(Stat, "comp_size", zipruby_stat_comp_size, 0);
  rb_define_method(Stat, "comp_method", zipruby_stat_comp_method, 0);
  rb_define_method(Stat, "encryption_method", zipruby_stat_encryption_method, 0);
  rb_define_method(Stat, "directory?", zipruby_stat_is_directory, 0);
}

static VALUE zipruby_stat_alloc(VALUE klass) {
  struct zipruby_stat *p = ALLOC(struct zipruby_stat);

  p->sb = ALLOC(struct zip_stat);
  zip_stat_init(p->sb);

  return Data_Wrap_Struct(klass, 0, zipruby_stat_free, p);
}

static void zipruby_stat_free(struct zipruby_stat *p) {
  xfree(p->sb);
  xfree(p);
}

/* */
static VALUE zipruby_stat_initialize(int argc, VALUE *argv, VALUE self) {
  VALUE archive, index, flags;
  struct zipruby_archive *p_archive;
  struct zipruby_stat *p_stat;
  char *fname = NULL;
  int i_index = -1, i_flags = 0;

  rb_scan_args(argc, argv, "21", &archive, &index, &flags);

  if (!rb_obj_is_instance_of(archive, Archive)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Zip::Archive)", rb_class2name(CLASS_OF(archive)));
  }

  switch (TYPE(index)) {
  case T_STRING: fname = RSTRING_PTR(index); break;
  case T_FIXNUM: i_index = NUM2INT(index); break;
  default:
    rb_raise(rb_eTypeError, "wrong argument type %s (expected String or Fixnum)", rb_class2name(CLASS_OF(index)));
  }

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Data_Get_Struct(archive, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);
  Data_Get_Struct(self, struct zipruby_stat, p_stat);

  if (fname) {
    if (zip_stat(p_archive->archive, fname, i_flags, p_stat->sb) != 0) {
      rb_raise(Error, "Obtain file status failed - %s: %s", fname, zip_strerror(p_archive->archive));
    }
  } else {
    if (zip_stat_index(p_archive->archive, i_index, i_flags, p_stat->sb) != 0) {
      rb_raise(Error, "Obtain file status failed at %d: %s", i_index, zip_strerror(p_archive->archive));
    }
  }

  return Qnil;
}

/* */
VALUE zipruby_stat_name(VALUE self) {
  struct zipruby_stat *p_stat;

  Data_Get_Struct(self, struct zipruby_stat, p_stat);

  return p_stat->sb->name ? rb_str_new2(p_stat->sb->name) : Qnil;
}

/* */
VALUE zipruby_stat_index(VALUE self) {
  struct zipruby_stat *p_stat;

  Data_Get_Struct(self, struct zipruby_stat, p_stat);

  return INT2NUM(p_stat->sb->index);
}

/* */
VALUE zipruby_stat_crc(VALUE self) {
  struct zipruby_stat *p_stat;

  Data_Get_Struct(self, struct zipruby_stat, p_stat);

  return UINT2NUM(p_stat->sb->crc);
}

/* */
VALUE zipruby_stat_size(VALUE self) {
  struct zipruby_stat *p_stat;

  Data_Get_Struct(self, struct zipruby_stat, p_stat);

  return LONG2NUM(p_stat->sb->size);
}

/* */
VALUE zipruby_stat_mtime(VALUE self) {
  struct zipruby_stat *p_stat;

  Data_Get_Struct(self, struct zipruby_stat, p_stat);

  return rb_funcall(rb_cTime, rb_intern("at"), 1,  LONG2NUM((long) p_stat->sb->mtime));
}

/* */
VALUE zipruby_stat_comp_size(VALUE self) {
  struct zipruby_stat *p_stat;

  Data_Get_Struct(self, struct zipruby_stat, p_stat);

  return LONG2NUM(p_stat->sb->comp_size);
}

/* */
VALUE zipruby_stat_comp_method(VALUE self) {
  struct zipruby_stat *p_stat;

  Data_Get_Struct(self, struct zipruby_stat, p_stat);

  return INT2NUM(p_stat->sb->comp_method);
}

/* */
VALUE zipruby_stat_encryption_method(VALUE self) {
  struct zipruby_stat *p_stat;

  Data_Get_Struct(self, struct zipruby_stat, p_stat);

  return INT2NUM(p_stat->sb->encryption_method);
}

/* */
VALUE zipruby_stat_is_directory(VALUE self) {
  struct zipruby_stat *p_stat;
  const char *name;
  size_t name_len;
  off_t size;

  Data_Get_Struct(self, struct zipruby_stat, p_stat);
  name = p_stat->sb->name;
  size = p_stat->sb->size;

  if (!name || size != 0) {
    return Qfalse;
  }

  name_len = strlen(name);

  if (name_len > 0 && name[name_len - 1] == '/') {
    return Qtrue;
  } else {
    return Qfalse;
  }
}
