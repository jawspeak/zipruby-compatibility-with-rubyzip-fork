#include <errno.h>
#include <zlib.h>

#include "zip.h"
#include "zipint.h"
#include "zipruby.h"
#include "zipruby_archive.h"
#include "zipruby_zip_source_proc.h"
#include "zipruby_zip_source_io.h"
#include "tmpfile.h"
#include "ruby.h"
#ifndef RUBY_VM
#include "rubyio.h"
#endif

static VALUE zipruby_archive_alloc(VALUE klass);
static void zipruby_archive_mark(struct zipruby_archive *p);
static void zipruby_archive_free(struct zipruby_archive *p);
static VALUE zipruby_archive_s_open(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_s_open_buffer(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_s_decrypt(VALUE self, VALUE path, VALUE password);
static VALUE zipruby_archive_s_encrypt(VALUE self, VALUE path, VALUE password);
static VALUE zipruby_archive_close(VALUE self);
static VALUE zipruby_archive_num_files(VALUE self);
static VALUE zipruby_archive_get_name(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_fopen(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_get_stat(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_add_buffer(VALUE self, VALUE name, VALUE source);
static VALUE zipruby_archive_add_file(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_add_io(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_add_function(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_replace_buffer(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_replace_file(int argc, VALUE* argv, VALUE self);
static VALUE zipruby_archive_replace_io(int argc, VALUE* argv, VALUE self);
static VALUE zipruby_archive_replace_function(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_add_or_replace_buffer(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_add_or_replace_file(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_add_or_replace_io(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_add_or_replace_function(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_update(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_get_comment(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_set_comment(VALUE self, VALUE comment);
static VALUE zipruby_archive_locate_name(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_get_fcomment(int argc, VALUE *argv, VALUE self);
static VALUE zipruby_archive_set_fcomment(VALUE self, VALUE index, VALUE comment);
static VALUE zipruby_archive_fdelete(VALUE self, VALUE index);
static VALUE zipruby_archive_frename(VALUE self, VALUE index, VALUE name);
static VALUE zipruby_archive_funchange(VALUE self, VALUE index);
static VALUE zipruby_archive_funchange_all(VALUE self);
static VALUE zipruby_archive_unchange(VALUE self);
static VALUE zipruby_archive_revert(VALUE self);
static VALUE zipruby_archive_each(VALUE self);
static VALUE zipruby_archive_commit(VALUE self);
static VALUE zipruby_archive_is_open(VALUE self);
static VALUE zipruby_archive_decrypt(VALUE self, VALUE password);
static VALUE zipruby_archive_encrypt(VALUE self, VALUE password);
static VALUE zipruby_archive_read(VALUE self);
static VALUE zipruby_archive_add_dir(VALUE self, VALUE name);

extern VALUE Zip;
VALUE Archive;
extern VALUE File;
extern VALUE Stat;
extern VALUE Error;

void Init_zipruby_archive() {
  Archive = rb_define_class_under(Zip, "Archive", rb_cObject);
  rb_define_alloc_func(Archive, zipruby_archive_alloc);
  rb_include_module(Archive, rb_mEnumerable);
  rb_define_singleton_method(Archive, "open", zipruby_archive_s_open, -1);
  rb_define_singleton_method(Archive, "open_buffer", zipruby_archive_s_open_buffer, -1);
  rb_define_singleton_method(Archive, "decrypt", zipruby_archive_s_decrypt, 2);
  rb_define_singleton_method(Archive, "encrypt", zipruby_archive_s_encrypt, 2);
  rb_define_method(Archive, "close", zipruby_archive_close, 0);
  rb_define_method(Archive, "num_files", zipruby_archive_num_files, 0);
  rb_define_method(Archive, "get_name", zipruby_archive_get_name, -1);
  rb_define_method(Archive, "fopen", zipruby_archive_fopen, -1);
  rb_define_method(Archive, "get_stat", zipruby_archive_get_stat, -1);
  rb_define_method(Archive, "add_buffer", zipruby_archive_add_buffer, 2);
  rb_define_method(Archive, "add_file", zipruby_archive_add_file, -1);
  rb_define_method(Archive, "add_io", zipruby_archive_add_io, -1);
  rb_define_method(Archive, "add", zipruby_archive_add_function, -1);
  rb_define_method(Archive, "replace_buffer", zipruby_archive_replace_buffer, -1);
  rb_define_method(Archive, "replace_file", zipruby_archive_replace_file, -1);
  rb_define_method(Archive, "replace_io", zipruby_archive_replace_io, -1);
  rb_define_method(Archive, "replace", zipruby_archive_replace_function, -1);
  rb_define_method(Archive, "add_or_replace_buffer", zipruby_archive_add_or_replace_buffer, -1);
  rb_define_method(Archive, "add_or_replace_file", zipruby_archive_add_or_replace_file, -1);
  rb_define_method(Archive, "add_or_replace_io", zipruby_archive_add_or_replace_io, -1);
  rb_define_method(Archive, "add_or_replace", zipruby_archive_add_or_replace_function, -1);
  rb_define_method(Archive, "update", zipruby_archive_update, -1);
  rb_define_method(Archive, "<<", zipruby_archive_add_io, -1);
  rb_define_method(Archive, "get_comment", zipruby_archive_get_comment, -1);
  rb_define_method(Archive, "comment", zipruby_archive_get_comment, -1);
  rb_define_method(Archive, "comment=", zipruby_archive_set_comment, 1);
  rb_define_method(Archive, "locate_name", zipruby_archive_locate_name, -1);
  rb_define_method(Archive, "get_fcomment", zipruby_archive_get_fcomment, -1);
  rb_define_method(Archive, "set_fcomment", zipruby_archive_set_fcomment, 2);
  rb_define_method(Archive, "fdelete", zipruby_archive_fdelete, 1);
  rb_define_method(Archive, "frename", zipruby_archive_frename, 2);
  rb_define_method(Archive, "funchange", zipruby_archive_funchange, 1);
  rb_define_method(Archive, "funchange_all", zipruby_archive_funchange_all, 0);
  rb_define_method(Archive, "unchange", zipruby_archive_unchange, 0);
  rb_define_method(Archive, "frevert", zipruby_archive_unchange, 1);
  rb_define_method(Archive, "revert", zipruby_archive_revert, 0);
  rb_define_method(Archive, "each", zipruby_archive_each, 0);
  rb_define_method(Archive, "commit", zipruby_archive_commit, 0);
  rb_define_method(Archive, "open?", zipruby_archive_is_open, 0);
  rb_define_method(Archive, "decrypt", zipruby_archive_decrypt, 1);
  rb_define_method(Archive, "encrypt", zipruby_archive_encrypt, 1);
  rb_define_method(Archive, "read", zipruby_archive_read, 0);
  rb_define_method(Archive, "add_dir", zipruby_archive_add_dir, 1);
}

static VALUE zipruby_archive_alloc(VALUE klass) {
  struct zipruby_archive *p = ALLOC(struct zipruby_archive);

  p->archive = NULL;
  p->path = Qnil;
  p->flags = 0;
  p->tmpfilnam = NULL;
  p->buffer = Qnil;
  p->sources = Qnil;

  return Data_Wrap_Struct(klass, zipruby_archive_mark, zipruby_archive_free, p);
}

static void zipruby_archive_mark(struct zipruby_archive *p) {
  rb_gc_mark(p->path);
  rb_gc_mark(p->buffer);
  rb_gc_mark(p->sources);
}

static void zipruby_archive_free(struct zipruby_archive *p) {
  if (p->tmpfilnam) {
    zipruby_rmtmp(p->tmpfilnam);
    free(p->tmpfilnam);
  }

  xfree(p);
}

/* */
static VALUE zipruby_archive_s_open(int argc, VALUE *argv, VALUE self) {
  VALUE path, flags, comp_level;
  VALUE archive;
  struct zipruby_archive *p_archive;
  int i_flags = 0;
  int errorp;
  int i_comp_level = Z_BEST_COMPRESSION;

  rb_scan_args(argc, argv, "12", &path, &flags, &comp_level);
  Check_Type(path, T_STRING);

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  if (!NIL_P(comp_level)) {
    i_comp_level = NUM2INT(comp_level);

    if (i_comp_level != Z_DEFAULT_COMPRESSION && i_comp_level != Z_NO_COMPRESSION && (i_comp_level < Z_BEST_SPEED || Z_BEST_COMPRESSION < i_comp_level)) {
      rb_raise(rb_eArgError, "Wrong compression level %d", i_comp_level);
    }
  }

  archive = rb_funcall(Archive, rb_intern("new"), 0);
  Data_Get_Struct(archive, struct zipruby_archive, p_archive);

  if ((p_archive->archive = zip_open(RSTRING_PTR(path), i_flags, &errorp)) == NULL) {
    char errstr[ERRSTR_BUFSIZE];
    zip_error_to_str(errstr, ERRSTR_BUFSIZE, errorp, errno);
    rb_raise(Error, "Open archive failed - %s: %s", RSTRING_PTR(path), errstr);
  }

  p_archive->archive->comp_level = i_comp_level;
  p_archive->path = path;
  p_archive->flags = i_flags;
  p_archive->sources = rb_ary_new();

  if (rb_block_given_p()) {
    VALUE retval;
    int status;

    retval = rb_protect(rb_yield, archive, &status);
    zipruby_archive_close(archive);

    if (status != 0) {
      rb_jump_tag(status);
    }

    return retval;
  } else {
    return archive;
  }
}

/* */
static VALUE zipruby_archive_s_open_buffer(int argc, VALUE *argv, VALUE self) {
  VALUE buffer, flags, comp_level;
  VALUE archive;
  struct zipruby_archive *p_archive;
  void *data = NULL;
  int len = 0, i_flags = 0;
  int errorp;
  int i_comp_level = Z_BEST_COMPRESSION;
  int buffer_is_temporary = 0;

  rb_scan_args(argc, argv, "03", &buffer, &flags, &comp_level);

  if (FIXNUM_P(buffer) && NIL_P(comp_level)) {
    comp_level = flags;
    flags = buffer;
    buffer = Qnil;
  }

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  if (!NIL_P(comp_level)) {
    i_comp_level = NUM2INT(comp_level);

    if (i_comp_level != Z_DEFAULT_COMPRESSION && i_comp_level != Z_NO_COMPRESSION && (i_comp_level < Z_BEST_SPEED || Z_BEST_COMPRESSION < i_comp_level)) {
      rb_raise(rb_eArgError, "Wrong compression level %d", i_comp_level);
    }
  }

  if (i_flags & ZIP_CREATE) {
    if (!NIL_P(buffer)) {
      Check_Type(buffer, T_STRING);
    } else {
      buffer = rb_str_new("", 0);
      buffer_is_temporary = 1;
    }

    i_flags = (i_flags | ZIP_TRUNC);
  } else if (TYPE(buffer) == T_STRING) {
    data = RSTRING_PTR(buffer);
    len = RSTRING_LEN(buffer);
  } else if (rb_obj_is_instance_of(buffer, rb_cProc)) {
    data = (void *) buffer;
    len = -1;
  } else {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected String or Proc)", rb_class2name(CLASS_OF(buffer)));
  }

  archive = rb_funcall(Archive, rb_intern("new"), 0);
  Data_Get_Struct(archive, struct zipruby_archive, p_archive);

  if ((p_archive->tmpfilnam = zipruby_tmpnam(data, len)) == NULL) {
    rb_raise(Error, "Open archive failed: Failed to create temporary file");
  }

  if ((p_archive->archive = zip_open(p_archive->tmpfilnam, i_flags, &errorp)) == NULL) {
    char errstr[ERRSTR_BUFSIZE];
    zip_error_to_str(errstr, ERRSTR_BUFSIZE, errorp, errno);
    rb_raise(Error, "Open archive failed: %s", errstr);
  }

  p_archive->archive->comp_level = i_comp_level;
  p_archive->path = rb_str_new2(p_archive->tmpfilnam);
  p_archive->flags = i_flags;
  p_archive->buffer = buffer;
  p_archive->sources = rb_ary_new();

  if (rb_block_given_p()) {
    VALUE retval;
    int status;

    retval = rb_protect(rb_yield, archive, &status);
    zipruby_archive_close(archive);

    if (status != 0) {
      rb_jump_tag(status);
    }

    return buffer_is_temporary ? buffer : retval;
  } else {
    return archive;
  }
}

/* */
static VALUE zipruby_archive_s_decrypt(VALUE self, VALUE path, VALUE password) {
  int res;
  int errorp, wrongpwd;
  long pwdlen;

  Check_Type(path, T_STRING);
  Check_Type(password, T_STRING);
  pwdlen = RSTRING_LEN(password);

  if (pwdlen < 1) {
    rb_raise(Error, "Decrypt archive failed - %s: Password is empty", RSTRING_PTR(path));
  } else if (pwdlen > 0xff) {
    rb_raise(Error, "Decrypt archive failed - %s: Password is too long", RSTRING_PTR(path));
  }

  res = zip_decrypt(RSTRING_PTR(path), RSTRING_PTR(password), pwdlen, &errorp, &wrongpwd);

  if (res == -1) {
    if (wrongpwd) {
      rb_raise(Error, "Decrypt archive failed - %s: Wrong password", RSTRING_PTR(path));
    } else {
      char errstr[ERRSTR_BUFSIZE];
      zip_error_to_str(errstr, ERRSTR_BUFSIZE, errorp, errno);
      rb_raise(Error, "Decrypt archive failed - %s: %s", RSTRING_PTR(path), errstr);
    }
  }

  return (res > 0) ? Qtrue : Qfalse;
}

/* */
static VALUE zipruby_archive_s_encrypt(VALUE self, VALUE path, VALUE password) {
  int res;
  int errorp;
  long pwdlen;

  Check_Type(path, T_STRING);
  Check_Type(password, T_STRING);
  pwdlen = RSTRING_LEN(password);

  if (pwdlen < 1) {
    rb_raise(Error, "Encrypt archive failed - %s: Password is empty", RSTRING_PTR(path));
  } else if (pwdlen > 0xff) {
    rb_raise(Error, "Encrypt archive failed - %s: Password is too long", RSTRING_PTR(path));
  }

  res = zip_encrypt(RSTRING_PTR(path), RSTRING_PTR(password), pwdlen, &errorp);

  if (res == -1) {
    char errstr[ERRSTR_BUFSIZE];
    zip_error_to_str(errstr, ERRSTR_BUFSIZE, errorp, errno);
    rb_raise(Error, "Encrypt archive failed - %s: %s", RSTRING_PTR(path), errstr);
  }

  return (res > 0) ? Qtrue : Qfalse;
}

/* */
static VALUE zipruby_archive_close(VALUE self) {
  struct zipruby_archive *p_archive;
  int changed, survivors;

  if (!zipruby_archive_is_open(self)) {
    return Qfalse;
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  changed = _zip_changed(p_archive->archive, &survivors);

  if (zip_close(p_archive->archive) == -1) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Close archive failed: %s", zip_strerror(p_archive->archive));
  }

  if (!NIL_P(p_archive->sources)){
    rb_ary_clear(p_archive->sources);
  }

  if (!NIL_P(p_archive->buffer) && changed) {
    rb_funcall(p_archive->buffer, rb_intern("replace"), 1, rb_funcall(self, rb_intern("read"), 0));
  }

  zipruby_rmtmp(p_archive->tmpfilnam);
  p_archive->archive = NULL;
  p_archive->flags = 0;

  return Qtrue;
}

/* */
static VALUE zipruby_archive_num_files(VALUE self) {
  struct zipruby_archive *p_archive;
  int num_files;

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);
  num_files = zip_get_num_files(p_archive->archive);

  return INT2NUM(num_files);
}

/* */
static VALUE zipruby_archive_get_name(int argc, VALUE *argv, VALUE self) {
  VALUE index, flags;
  struct zipruby_archive *p_archive;
  int i_flags = 0;
  const char *name;

  rb_scan_args(argc, argv, "11", &index, &flags);
  Check_Type(index, T_FIXNUM);

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if ((name = zip_get_name(p_archive->archive, NUM2INT(index), i_flags)) == NULL) {
    rb_raise(Error, "Get name failed at %d: %s", index, zip_strerror(p_archive->archive));
  }

  return (name != NULL) ? rb_str_new2(name) : Qnil;
}

/* */
static VALUE zipruby_archive_fopen(int argc, VALUE *argv, VALUE self) {
  VALUE index, flags, stat_flags, file;

  rb_scan_args(argc, argv, "12", &index, &flags, &stat_flags);
  file = rb_funcall(File, rb_intern("new"), 4, self, index, flags, stat_flags);

  if (rb_block_given_p()) {
    VALUE retval;
    int status;

    retval = rb_protect(rb_yield, file, &status);
    rb_funcall(file, rb_intern("close"), 0);

    if (status != 0) {
      rb_jump_tag(status);
    }

    return retval;
  } else {
    return file;
  }
}

/* */
static VALUE zipruby_archive_get_stat(int argc, VALUE *argv, VALUE self) {
  VALUE index, flags;

  rb_scan_args(argc, argv, "11", &index, &flags);

  return rb_funcall(Stat, rb_intern("new"), 3, self, index, flags);
}

/* */
static VALUE zipruby_archive_add_buffer(VALUE self, VALUE name, VALUE source) {
  struct zipruby_archive *p_archive;
  struct zip_source *zsource;
  char *data;
  size_t len;

  Check_Type(name, T_STRING);
  Check_Type(source, T_STRING);
  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  len = RSTRING_LEN(source);

  if ((data = malloc(len)) == NULL) {
    rb_raise(rb_eRuntimeError, "Add file failed: Cannot allocate memory");
  }

  memset(data, 0, len);
  memcpy(data, RSTRING_PTR(source), len);

  if ((zsource = zip_source_buffer(p_archive->archive, data, len, 1)) == NULL) {
    free(data);
    rb_raise(Error, "Add file failed - %s: %s", RSTRING_PTR(name), zip_strerror(p_archive->archive));
  }

  if (zip_add(p_archive->archive, RSTRING_PTR(name), zsource) == -1) {
    zip_source_free(zsource);
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Add file failed - %s: %s", RSTRING_PTR(name), zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_replace_buffer(int argc, VALUE *argv, VALUE self) {
  struct zipruby_archive *p_archive;
  struct zip_source *zsource;
  VALUE index, source, flags;
  int i_index, i_flags = 0;
  char *data;
  size_t len;

  rb_scan_args(argc, argv, "21", &index, &source, &flags);

  if (TYPE(index) != T_STRING && !FIXNUM_P(index)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Fixnum or String)", rb_class2name(CLASS_OF(index)));
  }

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Check_Type(source, T_STRING);
  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (FIXNUM_P(index)) {
    i_index = NUM2INT(index);
  } else if ((i_index = zip_name_locate(p_archive->archive, RSTRING_PTR(index), i_flags)) == -1) {
    rb_raise(Error, "Replace file failed - %s: Archive does not contain a file", RSTRING_PTR(index));
  }

  len = RSTRING_LEN(source);

  if ((data = malloc(len)) == NULL) {
    rb_raise(rb_eRuntimeError, "Replace file failed: Cannot allocate memory");
  }

  memcpy(data, RSTRING_PTR(source), len);

  if ((zsource = zip_source_buffer(p_archive->archive, data, len, 1)) == NULL) {
    free(data);
    rb_raise(Error, "Replace file failed at %d: %s", i_index, zip_strerror(p_archive->archive));
  }

  if (zip_replace(p_archive->archive, i_index, zsource) == -1) {
    zip_source_free(zsource);
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Replace file failed at %d: %s", i_index, zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_add_or_replace_buffer(int argc, VALUE *argv, VALUE self) {
  struct zipruby_archive *p_archive;
  VALUE name, source, flags;
  int index, i_flags = 0;

  rb_scan_args(argc, argv, "21", &name, &source, &flags);

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Check_Type(name, T_STRING);
  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  index = zip_name_locate(p_archive->archive, RSTRING_PTR(name), i_flags);

  if (index >= 0) {
    VALUE _args[] = { INT2NUM(index), source };
    return zipruby_archive_replace_buffer(2, _args, self);
  } else {
    return zipruby_archive_add_buffer(self, name, source);
  }
}

/* */
static VALUE zipruby_archive_add_file(int argc, VALUE *argv, VALUE self) {
  VALUE name, fname;
  struct zipruby_archive *p_archive;
  struct zip_source *zsource;

  rb_scan_args(argc, argv, "11", &name, &fname);

  if (NIL_P(fname)) {
    fname = name;
    name = Qnil;
  }

  Check_Type(fname, T_STRING);

  if (NIL_P(name)) {
    name = rb_funcall(rb_cFile, rb_intern("basename"), 1, fname);
  }

  Check_Type(name, T_STRING);
  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if ((zsource = zip_source_file(p_archive->archive, RSTRING_PTR(fname), 0, -1)) == NULL) {
    rb_raise(Error, "Add file failed - %s: %s", RSTRING_PTR(name), zip_strerror(p_archive->archive));
  }

  if (zip_add(p_archive->archive, RSTRING_PTR(name), zsource) == -1) {
    zip_source_free(zsource);
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Add file failed - %s: %s", RSTRING_PTR(name), zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_replace_file(int argc, VALUE* argv, VALUE self) {
  struct zipruby_archive *p_archive;
  struct zip_source *zsource;
  VALUE index, fname, flags;
  int i_index, i_flags = 0;

  rb_scan_args(argc, argv, "21", &index, &fname, &flags);

  if (TYPE(index) != T_STRING && !FIXNUM_P(index)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Fixnum or String)", rb_class2name(CLASS_OF(index)));
  }

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Check_Type(fname, T_STRING);
  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (FIXNUM_P(index)) {
    i_index = NUM2INT(index);
  } else if ((i_index = zip_name_locate(p_archive->archive, RSTRING_PTR(index), i_flags)) == -1) {
    rb_raise(Error, "Replace file failed - %s: Archive does not contain a file", RSTRING_PTR(index));
  }

  if ((zsource = zip_source_file(p_archive->archive, RSTRING_PTR(fname), 0, -1)) == NULL) {
    rb_raise(Error, "Replace file failed at %d: %s", i_index, zip_strerror(p_archive->archive));
  }

  if (zip_replace(p_archive->archive, i_index, zsource) == -1) {
    zip_source_free(zsource);
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Replace file failed at %d: %s", i_index, zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_add_or_replace_file(int argc, VALUE *argv, VALUE self) {
  VALUE name, fname, flags;
  struct zipruby_archive *p_archive;
  int index, i_flags = 0;

  rb_scan_args(argc, argv, "12", &name, &fname, &flags);

  if (NIL_P(flags) && FIXNUM_P(fname)) {
    flags = fname;
    fname = Qnil;
  }

  if (NIL_P(fname)) {
    fname = name;
    name = Qnil;
  }

  Check_Type(fname, T_STRING);

  if (NIL_P(name)) {
    name = rb_funcall(rb_cFile, rb_intern("basename"), 1, fname);
  }

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Check_Type(name, T_STRING);
  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  index = zip_name_locate(p_archive->archive, RSTRING_PTR(name), i_flags);

  if (index >= 0) {
    VALUE _args[] = { INT2NUM(index), fname };
    return zipruby_archive_replace_file(2, _args, self);
  } else {
    VALUE _args[] = { name, fname };
    return zipruby_archive_add_file(2, _args, self);
  }
}

/* */
static VALUE zipruby_archive_add_io(int argc, VALUE *argv, VALUE self) {
  VALUE name, file, mtime;
  struct zipruby_archive *p_archive;
  struct zip_source *zsource;
  struct read_io *z;

  rb_scan_args(argc, argv, "11", &name, &file);

  if (NIL_P(file)) {
    file = name;
    name = Qnil;
  }

  Check_IO(file);

  if (NIL_P(name)) {
    if (rb_obj_is_kind_of(file, rb_cFile)) {
      name = rb_funcall(rb_cFile, rb_intern("basename"), 1, rb_funcall(file, rb_intern("path"), 0));
    } else {
      rb_raise(rb_eRuntimeError, "Add io failed - %s: Entry name is not given", RSTRING(rb_inspect(file)));
    }
  }

  if (rb_obj_is_kind_of(file, rb_cFile)) {
    mtime = rb_funcall(file, rb_intern("mtime"), 0);
  } else {
    mtime = rb_funcall(rb_cTime, rb_intern("now"), 0);
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive); 
  Check_Archive(p_archive);

  if ((z = malloc(sizeof(struct read_io))) == NULL) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(rb_eRuntimeError, "Add io failed - %s: Cannot allocate memory", RSTRING(rb_inspect(file)));
  }

  z->io = file;
  rb_ary_push(p_archive->sources, file);
  z->mtime = TIME2LONG(mtime);

  if ((zsource = zip_source_io(p_archive->archive, z)) == NULL) {
    free(z);
    rb_raise(Error, "Add io failed - %s: %s", RSTRING(rb_inspect(file)), zip_strerror(p_archive->archive));
  }

  if (zip_add(p_archive->archive, RSTRING_PTR(name), zsource) == -1) {
    zip_source_free(zsource);
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Add io failed - %s: %s", RSTRING_PTR(name), zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_replace_io(int argc, VALUE *argv, VALUE self) {
  VALUE file, index, flags, mtime;
  struct zipruby_archive *p_archive;
  struct zip_source *zsource;
  struct read_io *z;
  int i_index, i_flags = 0;

  rb_scan_args(argc, argv, "21", &index, &file, &flags);

  if (TYPE(index) != T_STRING && !FIXNUM_P(index)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Fixnum or String)", rb_class2name(CLASS_OF(index)));
  }

  Check_IO(file);

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  if (rb_obj_is_kind_of(file, rb_cFile)) {
    mtime = rb_funcall(file, rb_intern("mtime"), 0);
  } else {
    mtime = rb_funcall(rb_cTime, rb_intern("now"), 0);
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (FIXNUM_P(index)) {
    i_index = NUM2INT(index);
  } else if ((i_index = zip_name_locate(p_archive->archive, RSTRING_PTR(index), i_flags)) == -1) {
    rb_raise(Error, "Replace io failed - %s: Archive does not contain a file", RSTRING_PTR(index));
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive); 
  Check_Archive(p_archive);

  if ((z = malloc(sizeof(struct read_io))) == NULL) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(rb_eRuntimeError, "Replace io failed at %d - %s: Cannot allocate memory", i_index, RSTRING(rb_inspect(file)));
  }

  z->io = file;
  rb_ary_push(p_archive->sources, file);
  z->mtime = TIME2LONG(mtime);

  if ((zsource = zip_source_io(p_archive->archive, z)) == NULL) {
    free(z);
    rb_raise(Error, "Replace io failed at %d - %s: %s", i_index, RSTRING(rb_inspect(file)), zip_strerror(p_archive->archive));
  }

  if (zip_replace(p_archive->archive, i_index, zsource) == -1) {
    zip_source_free(zsource);
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Replace io failed at %d - %s: %s", i_index, RSTRING(rb_inspect(file)), zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_add_or_replace_io(int argc, VALUE *argv, VALUE self) {
  VALUE name, io, flags;
  struct zipruby_archive *p_archive;
  int index, i_flags = 0;

  rb_scan_args(argc, argv, "21", &name, &io, &flags);
  Check_IO(io);

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Check_Type(name, T_STRING);
  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  index = zip_name_locate(p_archive->archive, RSTRING_PTR(name), i_flags);

  if (index >= 0) {
    VALUE _args[] = {INT2NUM(index), io, flags};
    return zipruby_archive_replace_io(2, _args, self);
  } else {
    VALUE _args[2] = { name, io };
    return zipruby_archive_add_io(2, _args, self);
  }
}

/* */
static VALUE zipruby_archive_add_function(int argc, VALUE *argv, VALUE self) {
  VALUE name, mtime;
  struct zipruby_archive *p_archive;
  struct zip_source *zsource;
  struct read_proc *z;

  rb_scan_args(argc, argv, "11", &name, &mtime);
  rb_need_block();
  Check_Type(name, T_STRING);

  if (NIL_P(mtime)) {
    mtime = rb_funcall(rb_cTime, rb_intern("now"), 0);
  } else if (!rb_obj_is_instance_of(mtime, rb_cTime)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Time)", rb_class2name(CLASS_OF(mtime)));
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive); 
  Check_Archive(p_archive);

  if ((z = malloc(sizeof(struct read_proc))) == NULL) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(rb_eRuntimeError, "Add failed - %s: Cannot allocate memory", RSTRING_PTR(name));
  }

  z->proc = rb_block_proc();
  rb_ary_push(p_archive->sources, z->proc);
  z->mtime = TIME2LONG(mtime);

  if ((zsource = zip_source_proc(p_archive->archive, z)) == NULL) {
    free(z);
    rb_raise(Error, "Add failed - %s: %s", RSTRING_PTR(name), zip_strerror(p_archive->archive));
  }

  if (zip_add(p_archive->archive, RSTRING_PTR(name), zsource) == -1) {
    zip_source_free(zsource);
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Add file failed - %s: %s", RSTRING_PTR(name), zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_replace_function(int argc, VALUE *argv, VALUE self) {
  VALUE index, flags, mtime;
  struct zipruby_archive *p_archive;
  struct zip_source *zsource;
  struct read_proc *z;
  int i_index, i_flags = 0;

  rb_scan_args(argc, argv, "12", &index, &mtime, &flags);
  rb_need_block();

  if (TYPE(index) != T_STRING && !FIXNUM_P(index)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Fixnum or String)", rb_class2name(CLASS_OF(index)));
  }

  if (NIL_P(mtime)) {
    mtime = rb_funcall(rb_cTime, rb_intern("now"), 0);
  } else if (!rb_obj_is_instance_of(mtime, rb_cTime)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Time)", rb_class2name(CLASS_OF(mtime)));
  }

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive); 
  Check_Archive(p_archive);

  if (FIXNUM_P(index)) {
    i_index = NUM2INT(index);
  } else if ((i_index = zip_name_locate(p_archive->archive, RSTRING_PTR(index), i_flags)) == -1) {
    rb_raise(Error, "Replace file failed - %s: Archive does not contain a file", RSTRING_PTR(index));
  }

  if ((z = malloc(sizeof(struct read_proc))) == NULL) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(rb_eRuntimeError, "Replace failed at %d: Cannot allocate memory", i_index);
  }

  z->proc = rb_block_proc();
  rb_ary_push(p_archive->sources, z->proc);
  z->mtime = TIME2LONG(mtime);

  if ((zsource = zip_source_proc(p_archive->archive, z)) == NULL) {
    free(z);
    rb_raise(Error, "Replace failed at %d: %s", i_index, zip_strerror(p_archive->archive));
  }

  if (zip_replace(p_archive->archive, i_index, zsource) == -1) {
    zip_source_free(zsource);
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Replace failed at %d: %s", i_index, zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_add_or_replace_function(int argc, VALUE *argv, VALUE self) {
  VALUE name, mtime, flags;
  struct zipruby_archive *p_archive;
  int index, i_flags = 0;

  rb_scan_args(argc, argv, "12", &name, &mtime, &flags);

  if (NIL_P(flags) && FIXNUM_P(mtime)) {
    flags = mtime;
    mtime = Qnil;
  }

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Check_Type(name, T_STRING);
  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  index = zip_name_locate(p_archive->archive, RSTRING_PTR(name), i_flags);

  if (index >= 0) {
    VALUE _args[] = { INT2NUM(index), mtime };
    return zipruby_archive_replace_function(2, _args, self);
  } else {
    VALUE _args[] = { name, mtime };
    return zipruby_archive_add_function(2, _args, self);
  }
}

/* */
static VALUE zipruby_archive_update(int argc, VALUE *argv, VALUE self) {
  struct zipruby_archive *p_archive, *p_srcarchive;
  VALUE srcarchive, flags;
  int i, num_files, i_flags = 0;

  rb_scan_args(argc, argv, "11", &srcarchive, &flags);

  if (!rb_obj_is_instance_of(srcarchive, Archive)) {
    rb_raise(rb_eTypeError, "wrong argument type %s (expected Zip::Archive)", rb_class2name(CLASS_OF(srcarchive)));
  }

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);
  Data_Get_Struct(srcarchive, struct zipruby_archive, p_srcarchive);
  Check_Archive(p_srcarchive);

  num_files = zip_get_num_files(p_srcarchive->archive);

  for (i = 0; i < num_files; i++) {
    struct zip_source *zsource;
    struct zip_file *fzip;
    struct zip_stat sb;
    char *buf;
    const char *name;
    int index, error;

    zip_stat_init(&sb);

    if (zip_stat_index(p_srcarchive->archive, i, 0, &sb)) {
      zip_unchange_all(p_archive->archive);
      zip_unchange_archive(p_archive->archive);
      rb_raise(Error, "Update archive failed: %s", zip_strerror(p_srcarchive->archive));
    }

    if ((buf = malloc(sb.size)) == NULL) {
      zip_unchange_all(p_archive->archive);
      zip_unchange_archive(p_archive->archive);
      rb_raise(rb_eRuntimeError, "Update archive failed: Cannot allocate memory");
    }

    fzip = zip_fopen_index(p_srcarchive->archive, i, 0);

    if (fzip == NULL) {
      free(buf);
      zip_unchange_all(p_archive->archive);
      zip_unchange_archive(p_archive->archive);
      rb_raise(Error, "Update archive failed: %s", zip_strerror(p_srcarchive->archive));
    }

    if (zip_fread(fzip, buf, sb.size) == -1) {
      free(buf);
      zip_fclose(fzip);
      zip_unchange_all(p_archive->archive);
      zip_unchange_archive(p_archive->archive);
      rb_raise(Error, "Update archive failed: %s", zip_file_strerror(fzip));
    }

    if ((error = zip_fclose(fzip)) != 0) {
      char errstr[ERRSTR_BUFSIZE];
      free(buf);
      zip_unchange_all(p_archive->archive);
      zip_unchange_archive(p_archive->archive);
      zip_error_to_str(errstr, ERRSTR_BUFSIZE, error, errno);
      rb_raise(Error, "Update archive failed: %s", errstr);
    }

    if ((zsource = zip_source_buffer(p_archive->archive, buf, sb.size, 1)) == NULL) {
      free(buf);
      zip_unchange_all(p_archive->archive);
      zip_unchange_archive(p_archive->archive);
      rb_raise(Error, "Update archive failed: %s", zip_strerror(p_archive->archive));
    }

    if ((name = zip_get_name(p_srcarchive->archive, i, 0)) == NULL) {
      zip_source_free(zsource);
      zip_unchange_all(p_archive->archive);
      zip_unchange_archive(p_archive->archive);
      rb_raise(Error, "Update archive failed: %s", zip_strerror(p_srcarchive->archive));
    }

    index = zip_name_locate(p_archive->archive, name, i_flags);

    if (index >= 0) {
      if (zip_replace(p_archive->archive, i, zsource) == -1) {
        zip_source_free(zsource);
        zip_unchange_all(p_archive->archive);
        zip_unchange_archive(p_archive->archive);
        rb_raise(Error, "Update archive failed: %s", zip_strerror(p_archive->archive));
      }
    } else {
      if (zip_add(p_archive->archive, name, zsource) == -1) {
        zip_source_free(zsource);
        zip_unchange_all(p_archive->archive);
        zip_unchange_archive(p_archive->archive);
        rb_raise(Error, "Update archive failed: %s", zip_strerror(p_archive->archive));
      }
    }
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_get_comment(int argc, VALUE *argv, VALUE self) {
  VALUE flags;
  struct zipruby_archive *p_archive;
  const char *comment;
  int lenp, i_flags = 0;

  rb_scan_args(argc, argv, "01", &flags);

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  // XXX: How is the error checked?
  comment = zip_get_archive_comment(p_archive->archive, &lenp, i_flags);

  return comment ? rb_str_new(comment, lenp) : Qnil;
}

/* */
static VALUE zipruby_archive_set_comment(VALUE self, VALUE comment) {
  struct zipruby_archive *p_archive;
  const char *s_comment = NULL;
  int len = 0;

  if (!NIL_P(comment)) {
    Check_Type(comment, T_STRING);
    s_comment = RSTRING_PTR(comment);
    len = RSTRING_LEN(comment);
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (zip_set_archive_comment(p_archive->archive, s_comment, len) == -1) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Comment archived failed: %s", zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_locate_name(int argc, VALUE *argv, VALUE self) {
  VALUE fname, flags;
  struct zipruby_archive *p_archive;
  int i_flags = 0;

  rb_scan_args(argc, argv, "11", &fname, &flags);
  Check_Type(fname, T_STRING);

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  return INT2NUM(zip_name_locate(p_archive->archive, RSTRING_PTR(fname), i_flags));
}

/* */
static VALUE zipruby_archive_get_fcomment(int argc, VALUE *argv, VALUE self) {
  VALUE index, flags;
  struct zipruby_archive *p_archive;
  const char *comment;
  int lenp, i_flags = 0;

  rb_scan_args(argc, argv, "11", &index, &flags);

  if (!NIL_P(flags)) {
    i_flags = NUM2INT(flags);
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  // XXX: How is the error checked?
  comment = zip_get_file_comment(p_archive->archive, NUM2INT(index), &lenp, i_flags);

  return comment ? rb_str_new(comment, lenp) : Qnil;
}

/* */
static VALUE zipruby_archive_set_fcomment(VALUE self, VALUE index, VALUE comment) {
  struct zipruby_archive *p_archive;
  char *s_comment = NULL;
  int len = 0;

  if (!NIL_P(comment)) {
    Check_Type(comment, T_STRING);
    s_comment = RSTRING_PTR(comment);
    len = RSTRING_LEN(comment);
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (zip_set_file_comment(p_archive->archive, NUM2INT(index), s_comment, len) == -1) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Comment file failed at %d: %s", NUM2INT(index), zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_fdelete(VALUE self, VALUE index) {
  struct zipruby_archive *p_archive;

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (zip_delete(p_archive->archive, NUM2INT(index)) == -1) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Delete file failed at %d: %s", NUM2INT(index), zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_frename(VALUE self, VALUE index, VALUE name) {
  struct zipruby_archive *p_archive;

  Check_Type(name, T_STRING);
  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (zip_rename(p_archive->archive, NUM2INT(index), RSTRING_PTR(name)) == -1) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Rename file failed at %d: %s", NUM2INT(index), zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_funchange(VALUE self, VALUE index) {
  struct zipruby_archive *p_archive;

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (zip_unchange(p_archive->archive, NUM2INT(index)) == -1) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Unchange file failed at %d: %s", NUM2INT(index), zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_funchange_all(VALUE self) {
  struct zipruby_archive *p_archive;

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (zip_unchange_all(p_archive->archive) == -1) {
    rb_raise(Error, "Unchange all file failed: %s", zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_unchange(VALUE self) {
  struct zipruby_archive *p_archive;

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (zip_unchange_archive(p_archive->archive) == -1) {
    rb_raise(Error, "Unchange archive failed: %s", zip_strerror(p_archive->archive));
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_revert(VALUE self) {
  zipruby_archive_funchange_all(self);
  zipruby_archive_unchange(self);

  return Qnil;
}

/* */
static VALUE zipruby_archive_each(VALUE self) {
  struct zipruby_archive *p_archive;
  int i, num_files;

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);
  num_files = zip_get_num_files(p_archive->archive);

  for (i = 0; i < num_files; i++) {
    VALUE file;
    int status;

    file = rb_funcall(File, rb_intern("new"), 2, self, INT2NUM(i));
    rb_protect(rb_yield, file, &status);
    rb_funcall(file, rb_intern("close"), 0);

    if (status != 0) {
      rb_jump_tag(status);
    }
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_commit(VALUE self) {
  struct zipruby_archive *p_archive;
  int changed, survivors;
  int errorp;

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  changed = _zip_changed(p_archive->archive, &survivors);

  if (zip_close(p_archive->archive) == -1) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Commit archive failed: %s", zip_strerror(p_archive->archive));
  }

  if (!NIL_P(p_archive->sources)){
    rb_ary_clear(p_archive->sources);
  }

  if (!NIL_P(p_archive->buffer) && changed) {
    rb_funcall(p_archive->buffer, rb_intern("replace"), 1, rb_funcall(self, rb_intern("read"), 0));
  }

  p_archive->archive = NULL;
  p_archive->flags = (p_archive->flags & ~(ZIP_CREATE | ZIP_EXCL));

  if ((p_archive->archive = zip_open(RSTRING_PTR(p_archive->path), p_archive->flags, &errorp)) == NULL) {
    char errstr[ERRSTR_BUFSIZE];
    zip_error_to_str(errstr, ERRSTR_BUFSIZE, errorp, errno);
    rb_raise(Error, "Commit archive failed: %s", errstr);
  }

  return Qnil;
}

/* */
static VALUE zipruby_archive_is_open(VALUE self) {
  struct zipruby_archive *p_archive;

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  return (p_archive->archive != NULL) ? Qtrue : Qfalse;
}

/* */
static VALUE zipruby_archive_decrypt(VALUE self, VALUE password) {
  VALUE retval;
  struct zipruby_archive *p_archive;
  long pwdlen;
  int changed, survivors;
  int errorp;

  Check_Type(password, T_STRING);
  pwdlen = RSTRING_LEN(password);

  if (pwdlen < 1) {
    rb_raise(Error, "Decrypt archive failed: Password is empty");
  } else if (pwdlen > 0xff) {
    rb_raise(Error, "Decrypt archive failed: Password is too long");
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  changed = _zip_changed(p_archive->archive, &survivors);

  if (zip_close(p_archive->archive) == -1) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Decrypt archive failed: %s", zip_strerror(p_archive->archive));
  }

  if (!NIL_P(p_archive->buffer) && changed) {
    rb_funcall(p_archive->buffer, rb_intern("replace"), 1, rb_funcall(self, rb_intern("read"), 0));
  }

  p_archive->archive = NULL;
  p_archive->flags = (p_archive->flags & ~(ZIP_CREATE | ZIP_EXCL));

  retval = zipruby_archive_s_decrypt(Archive, p_archive->path, password);

  if ((p_archive->archive = zip_open(RSTRING_PTR(p_archive->path), p_archive->flags, &errorp)) == NULL) {
    char errstr[ERRSTR_BUFSIZE];
    zip_error_to_str(errstr, ERRSTR_BUFSIZE, errorp, errno);
    rb_raise(Error, "Decrypt archive failed: %s", errstr);
  }

  return retval;
}

/* */
static VALUE zipruby_archive_encrypt(VALUE self, VALUE password) {
  VALUE retval;
  struct zipruby_archive *p_archive;
  long pwdlen;
  int changed, survivors;
  int errorp;

  Check_Type(password, T_STRING);
  pwdlen = RSTRING_LEN(password);

  if (pwdlen < 1) {
    rb_raise(Error, "Encrypt archive failed: Password is empty");
  } else if (pwdlen > 0xff) {
    rb_raise(Error, "Encrypt archive failed: Password is too long");
  }

  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  changed = _zip_changed(p_archive->archive, &survivors);

  if (zip_close(p_archive->archive) == -1) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Encrypt archive failed: %s", zip_strerror(p_archive->archive));
  }

  if (!NIL_P(p_archive->buffer) && changed) {
    rb_funcall(p_archive->buffer, rb_intern("replace"), 1, rb_funcall(self, rb_intern("read"), 0));
  }

  p_archive->archive = NULL;
  p_archive->flags = (p_archive->flags & ~(ZIP_CREATE | ZIP_EXCL));

  retval = zipruby_archive_s_encrypt(Archive, p_archive->path, password);

  if ((p_archive->archive = zip_open(RSTRING_PTR(p_archive->path), p_archive->flags, &errorp)) == NULL) {
    char errstr[ERRSTR_BUFSIZE];
    zip_error_to_str(errstr, ERRSTR_BUFSIZE, errorp, errno);
    rb_raise(Error, "Encrypt archive failed: %s", errstr);
  }

  return retval;
}

/* */
static VALUE zipruby_archive_read(VALUE self) {
  VALUE retval = Qnil;
  struct zipruby_archive *p_archive;
  FILE *fzip;
  char buf[DATA_BUFSIZE];
  ssize_t n;
  int block_given;

  Data_Get_Struct(self, struct zipruby_archive, p_archive);

  if (NIL_P(p_archive->path)) {
    rb_raise(rb_eRuntimeError, "invalid Zip::Archive");
  }

#ifdef _WIN32
  if (fopen_s(&fzip, RSTRING_PTR(p_archive->path), "rb") != 0) {
    rb_raise(Error, "Read archive failed: Cannot open archive");
  }
#else
  if ((fzip = fopen(RSTRING_PTR(p_archive->path), "rb")) == NULL) {
    rb_raise(Error, "Read archive failed: Cannot open archive");
  }
#endif

  block_given = rb_block_given_p();

  while ((n = fread(buf, 1, sizeof(buf), fzip)) > 0) {
    if (block_given) {
      rb_yield(rb_str_new(buf, n));
    } else {
      if (NIL_P(retval)) {
        retval = rb_str_new(buf, n);
      } else {
        rb_str_buf_cat(retval, buf, n);
      }
    }
  }

#if defined(RUBY_VM) && defined(_WIN32)
  _fclose_nolock(fzip);
#elif defined(RUBY_WIN32_H)
#undef fclose
  fclose(fzip);
#define fclose(f) rb_w32_fclose(f)
#else
  fclose(fzip);
#endif

  if (n == -1) {
    rb_raise(Error, "Read archive failed");
  }

  return retval;
}

/* */
static VALUE zipruby_archive_add_dir(VALUE self, VALUE name) {
  struct zipruby_archive *p_archive;

  Check_Type(name, T_STRING);
  Data_Get_Struct(self, struct zipruby_archive, p_archive);
  Check_Archive(p_archive);

  if (zip_add_dir(p_archive->archive, RSTRING_PTR(name)) == -1) {
    zip_unchange_all(p_archive->archive);
    zip_unchange_archive(p_archive->archive);
    rb_raise(Error, "Add dir failed - %s: %s", RSTRING_PTR(name), zip_strerror(p_archive->archive));
  }

  return Qnil;
}
