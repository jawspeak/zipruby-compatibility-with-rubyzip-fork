#ifdef _WIN32
__declspec(dllexport) void Init_zipruby(void);
#endif

#include "zipruby.h"
#include "zipruby_zip.h"
#include "zipruby_archive.h"
#include "zipruby_file.h"
#include "zipruby_stat.h"
#include "zipruby_error.h"

void Init_zipruby() {
  Init_zipruby_zip();
  Init_zipruby_archive();
  Init_zipruby_file();
  Init_zipruby_stat();
  Init_zipruby_error();
}
