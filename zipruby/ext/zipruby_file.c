#include <errno.h>

#include "zip.h"
#include "zipint.h"
#include "zipruby.h"
#include "zipruby_archive.h"
#include "zipruby_file.h"
#include "zipruby_stat.h"
#include "ruby.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static VALUE zipruby_file(VALUE klass);
static VALUE zipruby_file_alloc(VALUE klass);
static void zipruby_file_mark(struct zipruby_file *p);
static void zipruby_file_free(struct zipruby_file *p);
static VALUE zipruby_file_initialize(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_file_close(VALUE self);
static VALUE zipruby_file_read(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_file_stat(VALUE self);
static VALUE zipruby_file_get_comment(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_file_set_comment(VALUE self, VALUE comment);
static VALUE zipruby_file_delete(VALUE self);
static VALUE zipruby_file_rename(VALUE self, VALUE name);
static VALUE zipruby_file_unchange(VALUE self);
static VALUE zipruby_file_name(VALUE self);
static VALUE zipruby_file_index(VALUE self);
static VALUE zipruby_file_crc(VALUE self);
static VALUE zipruby_file_size(VALUE self);
static VALUE zipruby_file_mtime(VALUE self);
static VALUE zipruby_file_comp_size(VALUE self);
static VALUE zipruby_file_comp_method(VALUE self);
static VALUE zipruby_file_encryption_method(VALUE self);
static VALUE zipruby_file_is_directory(VALUE self);

extern VALUE Zip;
extern VALUE Archive;
VALUE File;
extern VALUE Stat;
extern VALUE Error;

void Init_zipruby_file() {
  File = rb_define_class_under(Zip, "File", rb_cObject);
  rb_define_alloc_func(File, zipruby_file_alloc);
  rb_define_method(File, "initialize", zipruby_file_initialize, -1);
  rb_define_method(File, "close", zipruby_file_close, 0);
  rb_define_method(File, "read", zipruby_file_read, -1);
  rb_define_method(File, "stat", zipruby_file_stat, 0);
  rb_define_method(File, "get_comment", zipruby_file_get_comment, -1);
  rb_define_method(File, "comment", zipruby_file_get_comment, -1);
  rb_define_method(File, "comment=", zipruby_file_set_comment, 1);
  rb_define_method(File, "delete", zipruby_file_delete, 0);
  rb_define_method(File, "rename", zipruby_file_rename, 1);
  rb_define_method(File, "unchange", zipruby_file_unchange, 1);
  rb_define_method(File, "revert", zipruby_file_unchange, 1);
  rb_define_method(File, "name", zipruby_file_name, 0);
  rb_define_method(File, "index", zipruby_file_index, 0);
  rb_define_method(File, "crc", zipruby_file_crc, 0);
  rb_define_method(File, "size", zipruby_file_size, 0);
  rb_define_method(File, "mtime", zipruby_file_mtime, 0);
  rb_define_method(File, "comp_size", zipruby_file_comp_size, 0);
  rb_define_method(File, "comp_method", zipruby_file_comp_method, 0);
  rb_define_method(File, "encryption_method", zipruby_file_encryption_method, 0);
  rb_define_method(File, "directory?", zipruby_file_is_directory, 0);
}

static VALUE zipruby_file_alloc(VALUE klass) {
  struct zipruby_file *p = ALLOC(struct zipruby_file);

  p->archive = NULL;
  p->file = NULL;
  p->sb = NULL;

  return Data_Wrap_Struct(klass, zipruby_file_mark, zipruby_file_free, p);
}

static void zipruby_file_mark(struct zipruby_file *p) {
  if (p->archive) { rb_gc_mark(p->v_archive); }
  if (p->sb) { rb_gc_mark(p->v_sb); }
}

static void zipruby_file_free(struct zipruby_file *p) {
  xfree(p);
}

/* */
static VALUE zipruby_file_initialize(int argc, VALUE *argv, VALUE self) {
  VALUE archive, index, flags, stat_flags;
  struct zipruby_archive *p_archive;
  struct zipruby_file *p_file;
  struct zipruby_stat *p_stat;
  struct zip_file *fzip;
  char *fname = NULL;
  int i_index = -1, i_flags = 0;

  rb_scan_args(argc, argv, "22", &archive,  &index, &flags, &stat_flags);

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

  if (fname) {
    fzip = zip_fopen(p_archive->archive, fname, i_flags);

    if (fzip == NULL) {
      rb_raise(Error, "Open file failed - %s: %s", fname, zip_strerror(p_archive->archive));
    }
  } else {
    fzip = zip_fopen_index(p_archive->archive, i_index, i_flags);

    if (fzip == NULL) {
      rb_raise(Error, "Open file failed at %d: %s", i_index, zip_strerror(p_archive->archive));
    }
  }

  Data_Get_Struct(self, struct zipruby_file, p_file);
  p_file->v_archive = archive;
  p_file->archive = p_archive->archive;
  p_file->file = fzip;
  p_file->v_sb = rb_funcall(Stat, rb_intern("new"), 3, archive, index, stat_flags);
  Data_Get_Struct(p_file->v_sb, struct zipruby_stat, p_stat);
  p_file->sb = p_stat->sb;

  return Qnil;
}

/* */
static VALUE zipruby_file_close(VALUE self) {
  struct zipruby_file *p_file;
  int error;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  if ((error = zip_fclose(p_file->file)) != 0) {
    char errstr[ERRSTR_BUFSIZE];
    zip_unchange(p_file->archive, p_file->sb->index);
    zip_error_to_str(errstr, ERRSTR_BUFSIZE, error, errno);
    rb_raise(Error, "Close file failed: %s", errstr);
  }

  p_file->archive = NULL;
  p_file->file = NULL;
  p_file->sb = NULL;

  return Qnil;
}

/* */
static VALUE zipruby_file_read(int argc, VALUE *argv, VALUE self) {
  VALUE size, retval = Qnil;
  struct zipruby_file *p_file;
  struct zip_stat sb;
  int block_given;
  size_t bytes_left;
  char buf[DATA_BUFSIZE];
  ssize_t n;

  rb_scan_args(argc, argv, "01", &size);
  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);
  zip_stat_init(&sb);

  if (p_file->archive->cdir->entry[0].bitflags & ZIP_GPBF_ENCRYPTED) {
    rb_raise(Error, "Read file failed: File encrypted");
  }

  if (zip_stat_index(p_file->archive, p_file->sb->index, 0, &sb)) {
    rb_raise(Error, "Read file failed: %s", zip_strerror(p_file->archive));
  }

  if (NIL_P(size)) {
    bytes_left = sb.size;
  } else {
    bytes_left = NUM2LONG(size);
  }

  if (bytes_left <= 0) {
    return Qnil;
  }

  block_given = rb_block_given_p();

  while ((n = zip_fread(p_file->file, buf, MIN(bytes_left, sizeof(buf)))) > 0) {
    if (block_given) {
      rb_yield(rb_str_new(buf, n));
    } else {
      if (NIL_P(retval)) {
        retval = rb_str_new(buf, n);
      } else {
        rb_str_buf_cat(retval, buf, n);
      }
    }

    bytes_left -= n;
  }

  if (n == -1) {
    rb_raise(Error, "Read file failed: %s", zip_file_strerror(p_file->file));
  }

  return retval;
}

/* */
static VALUE zipruby_file_stat(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  return p_file->v_sb;
}

/* */
static VALUE zipruby_file_get_comment(int argc, VALUE *argv, VALUE self) {
  VALUE flags;
  struct zipruby_file *p_file;
  const char *comment;
  int lenp, i_flags = 0;

  rb_scan_args(argc, argv, "01", &flags);

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  // XXX: How is the error checked?
  comment = zip_get_file_comment(p_file->archive, p_file->sb->index, &lenp, i_flags);

  return comment ? rb_str_new(comment, lenp) : Qnil;
}

/* */
static VALUE zipruby_file_set_comment(VALUE self, VALUE comment) {
  struct zipruby_file *p_file;
  char *s_comment = NULL;
  int len = 0;

  if (!NIL_P(comment)) {
    Check_Type(comment, T_STRING);
    s_comment = RSTRING_PTR(comment);
    len = RSTRING_LEN(comment);
  }

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  if (zip_set_file_comment(p_file->archive, p_file->sb->index, s_comment, len) == -1) {
    zip_unchange_all(p_file->archive);
    zip_unchange_archive(p_file->archive);
    rb_raise(Error, "Comment file failed - %s: %s", p_file->sb->name, zip_strerror(p_file->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_file_delete(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  if (zip_delete(p_file->archive, p_file->sb->index) == -1) {
    zip_unchange_all(p_file->archive);
    zip_unchange_archive(p_file->archive);
    rb_raise(Error, "Delete file failed - %s: %s", p_file->sb->name, zip_strerror(p_file->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_file_rename(VALUE self, VALUE name) {
  struct zipruby_file *p_file;

  Check_Type(name, T_STRING);
  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  if (zip_rename(p_file->archive, p_file->sb->index, RSTRING_PTR(name)) == -1) {
    zip_unchange_all(p_file->archive);
    zip_unchange_archive(p_file->archive);
    rb_raise(Error, "Rename file failed - %s: %s", p_file->sb->name, zip_strerror(p_file->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_file_unchange(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  if (zip_unchange(p_file->archive, p_file->sb->index) == -1) {
    rb_raise(Error, "Unchange file failed - %s: %s", p_file->sb->name, zip_strerror(p_file->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_file_name(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  return zipruby_stat_name(p_file->v_sb);
}

/* */
static VALUE zipruby_file_index(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  return zipruby_stat_index(p_file->v_sb);
}

/* */
static VALUE zipruby_file_crc(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  return zipruby_stat_crc(p_file->v_sb);
}

/* */
static VALUE zipruby_file_size(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  return zipruby_stat_size(p_file->v_sb);
}

/* */
static VALUE zipruby_file_mtime(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  return zipruby_stat_mtime(p_file->v_sb);
}

/* */
static VALUE zipruby_file_comp_size(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  return zipruby_stat_comp_size(p_file->v_sb);
}

/* */
static VALUE zipruby_file_comp_method(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  return zipruby_stat_comp_method(p_file->v_sb);
}

/* */
static VALUE zipruby_file_encryption_method(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  return zipruby_stat_encryption_method(p_file->v_sb);
}

/* */
static VALUE zipruby_file_is_directory(VALUE self) {
  struct zipruby_file *p_file;

  Data_Get_Struct(self, struct zipruby_file, p_file);
  Check_File(p_file);

  return zipruby_stat_is_directory(p_file->v_sb);
}
