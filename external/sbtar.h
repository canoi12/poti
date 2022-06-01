/*=====================================================================================*
  MIT License

  Copyright (c) 2021 Canoi Gomes

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*=====================================================================================*/
#ifndef SABOTAR_H
#define SABOTAR_H

#define SBTAR_API extern

#ifndef SBTAR_MALLOC
    #define SBTAR_MALLOC malloc
#endif

#ifdef _WIN32
#define SBTAR_STRCPY strcpy_s
#else
#define SBTAR_STRCPY strcpy
#endif

#define STR(x) #x
#define SBTAR_ASSERT(expr)\
    if (!(expr)) {\
        fprintf(stderr, "Assertion failed: %s\n", STR(expr));\
        exit(1);\
    }

#define SBTAR_NAME_OFFSET     0
#define SBTAR_MODE_OFFSET     100
#define SBTAR_UID_OFFSET      108
#define SBTAR_GID_OFFSET      116
#define SBTAR_SIZE_OFFSET     124
#define SBTAR_MTIME_OFFSET    136
#define SBTAR_CHKSUM_OFFSET   148
#define SBTAR_TYPE_OFFSET     156
#define SBTAR_LINKNAME_OFFSET 157
#define SBTAR_MAGIC_OFFSET    257
#define SBTAR_VERSION_OFFSET  263
#define SBTAR_UNAME_OFFSET    265
#define SBTAR_GNAME_OFFSET    297
#define SBTAR_DEVMAJ_OFFSET   329
#define SBTAR_DEVMIN_OFFSET   337
#define SBTAR_PREFIX_OFFSET   345

#define TMAGIC "ustar"

#define SBTAR_BLOCK_SIZE 512
#define SBTAR_NAME_SIZE 100
#define SBTAR_SZ_SIZE 12
#define SBTAR_MAGIC_SIZE 5

#include <stdio.h>
#include <stdlib.h>

enum {
  SBTAR_TREG = 0,
  SBTAR_TLINK,
  SBTAR_TSYM,
  SBTAR_TCHR,
  SBTAR_TBLK,
  SBTAR_TDIR,
  SBTAR_TFIFO
};

typedef struct sbtar_t sbtar_t;
typedef struct sbtar_header_t sbtar_header_t;
typedef struct posix_header_t posix_header_t;

/* POSIX header */
struct posix_header_t {
  struct {
    char name[100];       /* 0 */
    char mode[8];         /* 100 */
    char uid[8];          /* 108 */
    char gid[8];          /* 116 */
    char size[12];        /* 124 */
    char mtime[12];       /* 136 */
    char chksum[8];       /* 148 */
    char typeflag;        /* 156 */
    char linkname[100];   /* 157 */
    char magic[6];        /* 257 */
    char version[2];      /* 263 */
    char uname[32];       /* 265 */
    char gname[32];       /* 297 */
    char devmajor[8];     /* 329 */
    char devminor[8];     /* 337 */
    char prefix[155];     /* 345 */
    char _[12];           /* 500 */
  };                      /* 512 */
};

struct sbtar_t {
  unsigned offset;
  unsigned size;

  FILE *fp;
};

struct sbtar_header_t {
  unsigned size;
  unsigned mode;
  unsigned type;
  unsigned mtime;
  char name[100];
  char linkname[100];
  char uname[32], gname[32];
};

SBTAR_API int sbtar_open(sbtar_t *tar, const char *filename);
SBTAR_API void sbtar_close(sbtar_t *tar);

SBTAR_API int sbtar_next(sbtar_t *tar);
SBTAR_API void sbtar_rewind(sbtar_t *tar);
SBTAR_API void sbtar_seek(sbtar_t *tar, unsigned offset);

SBTAR_API void sbtar_header(sbtar_t *tar, sbtar_header_t *h);
SBTAR_API void sbtar_data(sbtar_t *tar, void *buf, unsigned size);

SBTAR_API void sbtar_ls(sbtar_t *tar);
SBTAR_API int sbtar_find(sbtar_t *tar, const char *filename);

SBTAR_API void sbtar_read(sbtar_t *tar, const char *filename, size_t size, char *out);
SBTAR_API void sbtar_write(sbtar_t *tar, const char *filename, const char *text, unsigned size);
SBTAR_API void sbtar_write_dir(sbtar_t *tar, const char *dirname);
/*SBTAR_API void sbtar_write(sbtar_t *tar, const char *file, const char *text);*/

#endif /* SABOTAR_H */

#ifdef SBTAR_IMPLEMENTATION

#include <string.h>

#define USER_MODE_MASK    0x7
#define GROUP_MODE_MASK  0x28
#define OTHER_MODE_MASK 0x1C0

static const char *_aux[] = {
  "---", "--x", "-w-", "-wx", 
  "r--", "r-x", "rw-", "rwx"
};

long _str_oct_to_dec(const char *bytes, int sz) {
  long size = 0;
  int i, mul;
  for (i = sz, mul = 1; i >= 0; mul *= 8, i--)
    if ( (bytes[i] >= '1') && (bytes[i] < '8')) size += (bytes[i] - '0') * mul;

  return size;
}

void _dec_to_str_oct(long size) {

}



long _header_size(const posix_header_t *h) {
  SBTAR_ASSERT(h != NULL);
  const char *bytes = h->size;
  /*long size = 0;
  int i, mul;
  for (i = SBTAR_SZ_SIZE - 2, mul = 1; i >= 0; mul*=8,i--)
    if ( (sz[i] >= '1') && (sz[i] < '8')) size += (sz[i] - '0') * mul;

  return size;*/
  return _str_oct_to_dec(bytes, SBTAR_SZ_SIZE - 2);
}

int sbtar_open(sbtar_t *tar, const char *filename) {
  SBTAR_ASSERT(tar != NULL);

#if defined(_WIN32)
  fopen_s(&tar->fp, filename, "rb+");
#else
  tar->fp = fopen(filename, "rb+");
#endif
 
  if (!tar->fp) {
    fprintf(stderr, "Failed to open %s\n", filename);
    return 0;
  }
  tar->offset = 0;
  fseek(tar->fp, 0, SEEK_END);
  tar->size = ftell(tar->fp);
  fseek(tar->fp, 0, SEEK_SET);
  return 1;
}

void sbtar_close(sbtar_t *tar) {
  SBTAR_ASSERT(tar != NULL);

  fclose(tar->fp);
  tar->offset = 0;
}

int sbtar_next(sbtar_t *tar) {
  SBTAR_ASSERT(tar != NULL);
  sbtar_header_t h;
  sbtar_header(tar, &h);

  int next = (1 + h.size/SBTAR_BLOCK_SIZE) * SBTAR_BLOCK_SIZE;
  /*printf("%d\n", h.type + 1);*/
  int type = h.type;
  if (type < 0 || (tar->offset + next >= tar->size)) return 0;
  if (h.size != 0) next += SBTAR_BLOCK_SIZE;
  sbtar_seek(tar, tar->offset + next);

  return 1;
}

void _print_header(sbtar_header_t h) {
  char type = '-';
  if (h.type == SBTAR_TDIR) type = 'd';

  int m1 = h.mode & USER_MODE_MASK;
  int m2 = (h.mode & GROUP_MODE_MASK) >> 3;
  int m3 = (h.mode & OTHER_MODE_MASK) >> 6;

  /*printf("%d %d %d\n", m3, m2, m1);*/
  printf("%c%s%s%s %s:%s %8db - %s\n", type, _aux[m3], _aux[m2], _aux[m1], h.uname, h.gname, h.size, h.name);
}

void sbtar_ls(sbtar_t *tar) {
  sbtar_rewind(tar);
  sbtar_header_t h;
  sbtar_header(tar, &h);

  while (sbtar_next(tar)) {
    _print_header(h);
    sbtar_header(tar, &h);
  }

  sbtar_rewind(tar);
}

void sbtar_rewind(sbtar_t *tar) {
  sbtar_seek(tar, 0);
}

void sbtar_seek(sbtar_t *tar, unsigned offset)  {
  SBTAR_ASSERT(tar != NULL);
  tar->offset = offset;
  fseek(tar->fp, tar->offset, SEEK_SET);
}

void sbtar_header(sbtar_t *tar, sbtar_header_t *header) {
  SBTAR_ASSERT(tar != NULL);
  SBTAR_ASSERT(header != NULL);
  sbtar_header_t *h = header;

  posix_header_t posix;
  fread(&posix, SBTAR_BLOCK_SIZE, 1, tar->fp);
  h->size = _header_size(&posix);
  h->type = posix.typeflag - '0';
  h->mode = _str_oct_to_dec(posix.mode, 8 - 3);
#ifdef _WIN32
  strcpy_s(h->name, strlen(posix.name), posix.name);
  strcpy_s(h->linkname, strlen(posix.linkname), posix.linkname);
  strcpy_s(h->uname, strlen(posix.uname), posix.uname);
  strcpy_s(h->gname, strlen(posix.gname), posix.gname);
#else
  strcpy(h->name, posix.name);
  strcpy(h->linkname, posix.linkname);
  strcpy(h->uname, posix.uname);
  strcpy(h->gname, posix.gname);
#endif
 /* h->mode*/

  sbtar_seek(tar, tar->offset);
}

void sbtar_data(sbtar_t *tar, void *ptr, unsigned size) {
  char *buffer = ptr;

  sbtar_seek(tar, tar->offset + SBTAR_BLOCK_SIZE);

  /*long sz = sbtar_header_size(header);*/
  /*long sz = header->size;*/

  /*int bl_sz = (1 + sz/SBTAR_BLOCK_SIZE) * SBTAR_BLOCK_SIZE;*/

  /*char buf[sz+1];*/
  fread(buffer, size, 1, tar->fp);

  sbtar_seek(tar, tar->offset - SBTAR_BLOCK_SIZE);
}

int sbtar_find(sbtar_t *tar, const char *filename) {
  SBTAR_ASSERT(tar != NULL);
  /*SBTAR_ASSERT(header != NULL);*/
  SBTAR_ASSERT(filename != NULL);

  /*fseek(tar->fp, 0, SEEK_END);
  size_t size = ftell(tar->fp);

  sbtar_seek(tar, 0);*/

  sbtar_rewind(tar);

  sbtar_header_t h;
  sbtar_header(tar, &h);
  if (!strcmp(h.name, filename)) return 1;

  while (sbtar_next(tar)) {
    sbtar_header(tar, &h);
    if (!strcmp(h.name, filename)) return 1;
  }

  printf("file %s not found\n", filename);
  return 0;
}

void sbtar_read(sbtar_t *tar, const char *filename, size_t size, char *out) {
    SBTAR_ASSERT(tar != NULL);
    SBTAR_ASSERT(filename != NULL);

    if (sbtar_find(tar, filename))
        sbtar_data(tar, out, size);
}

void sbtar_write(sbtar_t *tar, const char *filename, const char *text, unsigned size) {

}

void sbtar_write_dir(sbtar_t *tar, const char *dirname) {

}

#endif /* SBTAR_IMPLEMENTATION */