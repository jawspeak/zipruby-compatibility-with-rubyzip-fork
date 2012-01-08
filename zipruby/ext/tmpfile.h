#ifndef __TMPFILE_H__
#define __TMPFILE_H__

#include <stdio.h>

char *zipruby_tmpnam(void *data, int len);
void zipruby_rmtmp(const char *tmpfilnam);

#endif
