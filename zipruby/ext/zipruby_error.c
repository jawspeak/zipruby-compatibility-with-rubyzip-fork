#include "zipruby.h"
#include "zipruby_error.h"
#include "ruby.h"

extern VALUE Zip;
VALUE Error;

void Init_zipruby_error() {
  Error = rb_define_class_under(Zip, "Error", rb_eStandardError);
}
