/*
 zip_crypt.c -- zip encryption support
 Copyright (c) 2008 SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
 based on zip_close.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define close(f) _close(f)
#define rename(s, d) (MoveFileExA((s), (d), MOVEFILE_REPLACE_EXISTING) ? 0 : -1)
#endif

#include "zip.h"
#include "zipint.h"

#define ZIPENC_HEAD_LEN 12
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static zipenc_crc32(uLong crc, char c) {
  return crc32(crc ^ 0xffffffffL, &c, 1) ^ 0xffffffffL;
}

static void update_keys(uLong *keys, char c) {
  keys[0] = zipenc_crc32(keys[0], c);
  keys[1] = keys[1] + (keys[0] & 0xff);
  keys[1] = keys[1] * 134775813L + 1;
  c = (char) (keys[1] >> 24);
  keys[2] = zipenc_crc32(keys[2], c);
}

static unsigned char decrypt_byte(uLong *keys) {
  unsigned short temp;

  temp = (unsigned short) (keys[2] | 2);
  return (temp * (temp ^ 1)) >> 8;
}

static void init_keys(uLong *keys, const char *password, size_t len) {
  int i;

  keys[0] = 305419896L;
  keys[1] = 591751049L;
  keys[2] = 878082192L;

  for (i = 0; i < len; i++) {
    update_keys(keys, password[i]);
  }
}

static int decrypt_header(unsigned long *keys, char *buffer, struct zip_dirent *de) {
  int i;
  char c;

  for (i = 0; i < ZIPENC_HEAD_LEN; i++) {
    c = buffer[i] ^ decrypt_byte(keys);
    update_keys(keys, c);
    buffer[i] = c;
  }

  if (de->bitflags & ZIP_GPBF_DATA_DESCRIPTOR) {
    unsigned short dostime, dosdate;
    _zip_u2d_time(de->last_mod, &dostime, &dosdate);
    return ((c & 0xff) == (dostime >> 8)) ? 0 : -1;
  } else {
    return ((c & 0xff) == (de->crc >> 24)) ? 0 : -1;
  }
}

static void decrypt_data(uLong *keys, char *buffer, size_t n) {
  int i;

  for (i = 0; i < n; i++) {
    char temp = buffer[i] ^ decrypt_byte(keys);
    update_keys(keys, temp);
    buffer[i] = temp;
  }
}

static int copy_decrypt(FILE *src, off_t len, const char *pwd, int pwdlen, struct zip_dirent *de, FILE *dest, struct zip_error *error, int *wrongpwd) {
  char buf[BUFSIZE];
  uLong keys[3];
  int n;

  *wrongpwd = 0;

  if (len == 0) {
    return 0;
  }

  init_keys(keys, pwd, pwdlen);

  if (fread(buf, 1, ZIPENC_HEAD_LEN, src) < 0) {
    _zip_error_set(error, ZIP_ER_READ, errno);
  }

  if (decrypt_header(keys, buf, de) == -1) {
    // XXX: _zip_error_set
    *wrongpwd = 1;
    return -1;
  }

  while (len > 0) {
    if ((n = fread(buf, 1, MIN(len,  sizeof(buf)), src)) < 0) {
      _zip_error_set(error, ZIP_ER_READ, errno);
      return -1;
    } else if (n == 0) {
      _zip_error_set(error, ZIP_ER_EOF, 0);
      return -1;
    }

    decrypt_data(keys, buf, n);

    if (fwrite(buf, 1, n, dest) != ((size_t) n)) {
      _zip_error_set(error, ZIP_ER_WRITE, errno);
      return -1;
    }

    len -= n;
  }

  return 0;
}

static void generate_random_header(unsigned long *keys, char *buffer) {
  static int initialized = 0;
  int i;

  if (!initialized) {
    srand((unsigned) time(NULL));
    initialized = 1;
  }

  for (i = 0; i < ZIPENC_HEAD_LEN - 2; i++) {
    char temp = decrypt_byte(keys);
    char c = rand() % 0xff;
    update_keys(keys, c);
    buffer[i] = temp ^ c;
  }
}

static void encrypt_header(unsigned long *keys, char *buffer, struct zip_dirent *de) {
  int i;
  char c, temp;

  for (i = 0; i < ZIPENC_HEAD_LEN - 2; i++) {
    temp = decrypt_byte(keys);
    c = buffer[i];
    update_keys(keys, c);
    buffer[i] = temp ^ c;
  }

  temp = decrypt_byte(keys);
  c = (de->crc >> 16) & 0xff;
  update_keys(keys, c);
  buffer[ZIPENC_HEAD_LEN - 2] = temp ^ c;

  temp = decrypt_byte(keys);
  c = (de->crc >> 24) & 0xff;
  update_keys(keys, c);
  buffer[ZIPENC_HEAD_LEN - 1] = temp ^ c;
}

static void encrypt_data(uLong *keys, char *buffer, size_t n) {
  int i;

  for (i = 0; i < n; i++) {
    char temp = decrypt_byte(keys);
    char c = buffer[i];
    update_keys(keys, c);
    buffer[i] = temp ^ c;
  }
}

static int copy_encrypt(FILE *src, off_t len, const char *pwd, int pwdlen, struct zip_dirent *de, FILE *dest, struct zip_error *error) {
  char header[ZIPENC_HEAD_LEN];
  char buf[BUFSIZE];
  uLong keys[3];
  int n;

  if (len == 0) {
    return 0;
  }

  init_keys(keys, pwd, pwdlen);
  generate_random_header(keys, header);
  init_keys(keys, pwd, pwdlen);
  encrypt_header(keys, header, de);

  if (fwrite(header, 1, ZIPENC_HEAD_LEN, dest) != ((size_t) ZIPENC_HEAD_LEN)) {
    _zip_error_set(error, ZIP_ER_WRITE, errno);
  }

  while (len > 0) {
    if ((n = fread(buf, 1, MIN(len,  sizeof(buf)), src)) < 0) {
      _zip_error_set(error, ZIP_ER_READ, errno);
      return -1;
    } else if (n == 0) {
      _zip_error_set(error, ZIP_ER_EOF, 0);
      return -1;
    }

    encrypt_data(keys, buf, n);

    if (fwrite(buf, 1, n, dest) != ((size_t) n)) {
      _zip_error_set(error, ZIP_ER_WRITE, errno);
      return -1;
    }

    len -= n;
  }

  return 0;
}

static int _zip_crypt(struct zip *za, const char *pwd, int pwdlen, int decrypt, int *wrongpwd) {
  int translated = 0;
  int i, error = 0;
  char *temp;
  FILE *out;
#ifndef _WIN32
  mode_t mask;
#endif
  struct zip_cdir *cd;
  struct zip_dirent de;
  int reopen_on_error = 0;

  if (za == NULL) {
    return -1;
  }

  if (za->nentry < 1) {
    _zip_free(za);
    return 0;
  }

  if ((cd = _zip_cdir_new(za->nentry, &za->error)) == NULL) {
    return -1;
  }

  for (i = 0; i < za->nentry; i++) {
    _zip_dirent_init(&cd->entry[i]);
  }

  if (_zip_cdir_set_comment(cd, za) == -1) {
    _zip_cdir_free(cd);
    return -1;
  }

  if ((temp = _zip_create_temp_output(za, &out)) == NULL) {
    _zip_cdir_free(cd);
    return -1;
  }

  for (i = 0; i < za->nentry; i++) {
    struct zip_dirent fde;
    int encrypted;
    unsigned int comp_size;

    if (fseeko(za->zp, za->cdir->entry[i].offset, SEEK_SET) != 0) {
      _zip_error_set(&za->error, ZIP_ER_SEEK, errno);
      error = 1;
      break;
    }

    if (_zip_dirent_read(&de, za->zp, NULL, 0, 1, &za->error) != 0) {
      error = 1;
      break;
    }

    memcpy(&fde, &de, sizeof(fde));
    encrypted = de.bitflags & ZIP_GPBF_ENCRYPTED;

    if (de.bitflags & ZIP_GPBF_DATA_DESCRIPTOR) {
      de.crc = za->cdir->entry[i].crc;
      de.comp_size = za->cdir->entry[i].comp_size;
      de.uncomp_size = za->cdir->entry[i].uncomp_size;
      de.bitflags &= ~ZIP_GPBF_DATA_DESCRIPTOR;
    }

    memcpy(cd->entry + i, za->cdir->entry + i, sizeof(cd->entry[i]));
    comp_size = cd->entry[i].comp_size;

    if (cd->entry[i].bitflags & ZIP_GPBF_DATA_DESCRIPTOR) {
      cd->entry[i].bitflags &= ~ZIP_GPBF_DATA_DESCRIPTOR;
    }

    cd->entry[i].offset = ftello(out);

    if (decrypt && encrypted) {
      de.comp_size -= ZIPENC_HEAD_LEN;
      de.bitflags &= ~ZIP_GPBF_ENCRYPTED;
      cd->entry[i].comp_size -= ZIPENC_HEAD_LEN;
      cd->entry[i].bitflags &= ~ZIP_GPBF_ENCRYPTED;
      translated = 1;
    } else if (!decrypt && !encrypted) {
      de.comp_size += ZIPENC_HEAD_LEN;
      de.bitflags |= ZIP_GPBF_ENCRYPTED;
      cd->entry[i].comp_size += ZIPENC_HEAD_LEN;
      cd->entry[i].bitflags |= ZIP_GPBF_ENCRYPTED;
      translated = 1;
    }

    if (_zip_dirent_write(&de, out, 1, &za->error) < 0) {
      error = 1;
      break;
    }

    if (decrypt && encrypted) {
      error = (copy_decrypt(za->zp, comp_size, pwd, pwdlen, &fde, out, &za->error, wrongpwd) < 0);
    } else if (!decrypt && !encrypted) {
      error = (copy_encrypt(za->zp, comp_size, pwd, pwdlen, &fde, out, &za->error) < 0);
    } else {
      error = (copy_data(za->zp, comp_size, out, &za->error) < 0);
    }

    if (error) {
      break;
    }

    _zip_dirent_finalize(&de);
  }

  if (!error && _zip_cdir_write(cd, out, &za->error) < 0) {
    error = 1;
  }

  cd->nentry = 0;
  _zip_cdir_free(cd);

  if (error) {
    _zip_dirent_finalize(&de);
    fclose(out);
    remove(temp);
    free(temp);
    return -1;
  }

  if (fclose(out) != 0) {
    _zip_error_set(&za->error, ZIP_ER_CLOSE, errno);
    remove(temp);
    free(temp);
    return -1;
  }

  if (za->zp) {
    fclose(za->zp);
    za->zp = NULL;
    reopen_on_error = 1;
  }

  if (rename(temp, za->zn) != 0) {
    _zip_error_set(&za->error, ZIP_ER_RENAME, errno);
    remove(temp);
    free(temp);

    if (reopen_on_error) {
      za->zp = fopen(za->zn, "rb");
    }

    return -1;
  }

#ifndef _WIN32
  mask = umask(0);
  umask(mask);
  chmod(za->zn, 0666&~mask);
#endif

  free(temp);
  return translated;
}

int zip_decrypt(const char *path, const char *pwd, int pwdlen, int *errorp, int *wrongpwd) {
  struct zip *za;
  int res;

  if (pwd == NULL || pwdlen < 1) {
    return -1;
  }

  if ((za = zip_open(path, 0, errorp)) == NULL) {
    return -1;
  }

  res = _zip_crypt(za, pwd, pwdlen, 1, wrongpwd);
  _zip_free(za);

  return res;
}

int zip_encrypt(const char *path, const char *pwd, int pwdlen, int *errorp) {
  struct zip *za;
  int res;

  if (pwd == NULL || pwdlen < 1) {
    return -1;
  }

  if ((za = zip_open(path, 0, errorp)) == NULL) {
    return -1;
  }

  res = _zip_crypt(za, pwd, pwdlen, 0, NULL);
  _zip_free(za);

  return res;
}
