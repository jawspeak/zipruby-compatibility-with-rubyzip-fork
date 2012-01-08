#include <string.h>

#include "zip.h"
#include "zipint.h"
#include "zipruby_zip_source_proc.h"
#include "ruby.h"

static VALUE proc_call(VALUE proc) {
  return rb_funcall(proc, rb_intern("call"), 0);
}

static ssize_t read_proc(void *state, void *data, size_t len, enum zip_source_cmd cmd) {
  struct read_proc *z;
  VALUE src;
  char *buf;
  size_t n;
  int status = 0;

  z = (struct read_proc *) state;
  buf = (char *) data;

  switch (cmd) {
  case ZIP_SOURCE_OPEN:
    return 0;

  case ZIP_SOURCE_READ:
    src = rb_protect(proc_call, z->proc, &status);

    if (status != 0) {
      VALUE message, clazz;

#if defined(RUBY_VM)
      message = rb_funcall(rb_errinfo(), rb_intern("message"), 0);
      clazz = CLASS_OF(rb_errinfo());
#else
      message = rb_funcall(ruby_errinfo, rb_intern("message"), 0);
      clazz = CLASS_OF(ruby_errinfo);
#endif

      rb_warn("Error in Proc: %s (%s)", RSTRING_PTR(message), rb_class2name(clazz));
      return -1;
    }


    if (TYPE(src) != T_STRING) {
      src = rb_funcall(src, rb_intern("to_s"), 0);
    }

    n = RSTRING_LEN(src);

    if (n > 0) {
      n = (n > len) ? len : n;
      memcpy(buf, RSTRING_PTR(src), n);
    }

    return n;

  case ZIP_SOURCE_CLOSE:
    return 0;

  case ZIP_SOURCE_STAT:
    {
      struct zip_stat *st = (struct zip_stat *)data;
      zip_stat_init(st);
      st->mtime = z->mtime;
      return sizeof(*st);
    }

  case ZIP_SOURCE_ERROR:
    return 0;

  case ZIP_SOURCE_FREE:
    free(z);
    return 0;
  }

  return -1;
}

struct zip_source *zip_source_proc(struct zip *za, struct read_proc *z) {
  struct zip_source *zs;
  zs = zip_source_function(za, read_proc, z);
  return zs;
}
