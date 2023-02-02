#ifndef SABOTAR_H
#define SABOTAR_H

#define SBTAR_API extern

#define SBTAR_VERSION "0.2.0"

#ifndef SBTAR_MALLOC
    #define SBTAR_MALLOC malloc
#endif

#define STR(x) #x
#define SBTAR_ASSERT(expr) \
if (!(expr)) { \
    fprintf(stderr, "Assertion failed: %s\n", STR(expr)); \
    exit(1); \
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
#define SBTAR_MTIME_SIZE 12
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
    };                        /* 512 */
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
    int chksum;
    char name[100];
    char linkname[100];
    char uname[32], gname[32];
};

SBTAR_API void sbtar_open(sbtar_t *tar, const char *filename);
SBTAR_API void sbtar_new(sbtar_t *tar, const char* filename);
SBTAR_API void sbtar_close(sbtar_t *tar);

SBTAR_API int sbtar_next(sbtar_t *tar);
SBTAR_API void sbtar_rewind(sbtar_t *tar);
SBTAR_API void sbtar_seek(sbtar_t *tar, unsigned offset);

SBTAR_API void sbtar_header(sbtar_t *tar, sbtar_header_t *h);
SBTAR_API void sbtar_posix(sbtar_t *tar, posix_header_t* h);
SBTAR_API void sbtar_data(sbtar_t *tar, void *buf, unsigned size);

SBTAR_API void sbtar_ls(sbtar_t *tar);
SBTAR_API int sbtar_find(sbtar_t *tar, const char *filename);

SBTAR_API const char* sbtar_read(sbtar_t *tar, const char *filename);
SBTAR_API void sbtar_write(sbtar_t *tar, const char *filename, const char *text, unsigned size);
SBTAR_API void sbtar_write_dir(sbtar_t *tar, const char *dirname);
/*SBTAR_API void sbtar_write(sbtar_t *tar, const char *file, const char *text);*/

#endif /* SABOTAR_H */

#ifdef SBTAR_IMPLEMENTATION

#include <string.h>
#include <time.h>

#define USER_MODE_MASK    0x7
#define GROUP_MODE_MASK  0x28
#define OTHER_MODE_MASK 0x1C0

static const char *_aux[] = {
    "---", "--x", "-w-", "-wx", 
    "r--", "r-x", "rw-", "rwx"
};

static int calc_checksum(posix_header_t h) {
    int sum = 0;
    int i;
    char* bytes = (char*)&h;
    for (i = 0; i < 8; i++) {
        /* printf("%c ", p.chksum[i]); */
        h.chksum[i] = ' ';
    }
    for (i = 0; i < SBTAR_BLOCK_SIZE; i++) sum += bytes[i];
    return sum;
}

static unsigned int oct2int(const char* c, int size) {
    unsigned int oct = 0;
    int i = 0;
    while (c[i] != 0 && i < size) oct = (oct << 3) | (c[i++] - '0');
    return oct;
}

#if 0
static void int2oct(long n, char* out, int size) {

}

long _str_oct_to_dec(const char *bytes, int sz) {
    long size = 0;
    int i, mul;
    for (i = sz, mul = 1; i >= 0; mul *= 8, i--)
        if ( (bytes[i] >= '1') && (bytes[i] < '8')) size += (bytes[i] - '0') * mul;

    return size;
}

void _dec_to_str_oct(long size) {

}
#endif

long _header_size(const posix_header_t *h) {
    SBTAR_ASSERT(h != NULL);
    const char *bytes = h->size;
    /*long size = 0;
    int i, mul;
    for (i = SBTAR_SZ_SIZE - 2, mul = 1; i >= 0; mul*=8,i--)
        if ( (sz[i] >= '1') && (sz[i] < '8')) size += (sz[i] - '0') * mul;

    return size;*/
    return oct2int(bytes, SBTAR_SZ_SIZE);
}

void sbtar_open(sbtar_t *tar, const char *filename) {
    SBTAR_ASSERT(tar != NULL);

    tar->fp = fopen(filename, "rb+");
    if (!tar->fp) {
        fprintf(stderr, "Failed to open %s\n", filename);
        exit(1);
    }
    tar->offset = 0;
    fseek(tar->fp, 0, SEEK_END);
    tar->size = ftell(tar->fp);
    fseek(tar->fp, 0, SEEK_SET);
}

void sbtar_new(sbtar_t *tar, const char* filename) {
    SBTAR_ASSERT(tar != NULL);
    tar->fp = fopen(filename, "wb+");
    if (!tar->fp) {
        fprintf(stderr, "Failed to create tar %s\n", filename);
        exit(EXIT_FAILURE);
    }
    posix_header_t p;
    tar->offset = 0;
    memset(&p, 0, sizeof(p));
    fwrite(&p, 1, SBTAR_BLOCK_SIZE, tar->fp);
    fwrite(&p, 1, SBTAR_BLOCK_SIZE, tar->fp);
    
    fseek(tar->fp, 0, SEEK_END);
    tar->size = ftell(tar->fp);
    fseek(tar->fp, 0, SEEK_SET);
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
    if (h.chksum == 0) return 0;

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
    fread(&posix, 1, SBTAR_BLOCK_SIZE, tar->fp);
    strcpy(h->name, posix.name);
    strcpy(h->linkname, posix.linkname);
    h->size = oct2int(posix.size, SBTAR_SZ_SIZE-1);
    h->mtime = oct2int(posix.mtime, SBTAR_MTIME_SIZE-1);
    h->type = posix.typeflag - '0';
    h->mode = oct2int(posix.mode, 7);
    h->chksum = oct2int(posix.chksum, 7);
    strcpy(h->uname, posix.uname);
    strcpy(h->gname, posix.gname);

    fseek(tar->fp, tar->offset, SEEK_SET);
}

void sbtar_posix(sbtar_t *tar, posix_header_t* h) {
    SBTAR_ASSERT(tar != NULL);
    SBTAR_ASSERT(h != NULL);
    fread(h, SBTAR_BLOCK_SIZE, 1, tar->fp);
    sbtar_seek(tar, tar->offset);
}

void sbtar_data(sbtar_t *tar, void *ptr, unsigned size) {
    char *buffer = ptr;
    SBTAR_ASSERT(tar != NULL);
    fseek(tar->fp, tar->offset + SBTAR_BLOCK_SIZE, SEEK_SET);

    fread(buffer, size, 1, tar->fp);
    buffer[size] = '\0';

    fseek(tar->fp, tar->offset, SEEK_SET);
}

int sbtar_find(sbtar_t *tar, const char *filename) {
    SBTAR_ASSERT(tar != NULL);
    SBTAR_ASSERT(filename != NULL);

    sbtar_rewind(tar);

    sbtar_header_t h;
    sbtar_header(tar, &h);
    if (!strcmp(h.name, filename)) return 1;

    while (sbtar_next(tar)) {
        sbtar_header(tar, &h);
        if (!strcmp(h.name, filename)) return 1;
    }
    return 0;
}

const char* sbtar_read(sbtar_t *tar, const char *filename) {
    SBTAR_ASSERT(tar != NULL);
    SBTAR_ASSERT(filename != NULL);
    if (!sbtar_find(tar, filename)) return NULL;

    sbtar_header_t h;
    sbtar_header(tar, &h);
    /* tar->offset += SBTAR_BLOCK_SIZE; */
    fseek(tar->fp, tar->offset + SBTAR_BLOCK_SIZE, SEEK_SET);

    long size = h.size;
    /* int bl_sz = (1 + (size / SBTAR_BLOCK_SIZE)) * SBTAR_BLOCK_SIZE; */
    char* out = malloc(size + 1);
    fread(out, size, 1, tar->fp);
    out[size] = '\0';
    fseek(tar->fp, tar->offset, SEEK_SET);
    return out;
}

void sbtar_write(sbtar_t *tar, const char *filename, const char *text, unsigned size) {
    SBTAR_ASSERT(tar != NULL);
    SBTAR_ASSERT(filename != NULL);
    if (sbtar_find(tar, filename)) return;
    sbtar_rewind(tar);
    while (sbtar_next(tar));
    posix_header_t p;
    memset(&p, 0, sizeof(p));
    /* write name */
    strcpy(p.name, filename);
    /* write mode */
    strncpy(p.mode, "0000644", 8);
    /* write uid and gid */
    strncpy(p.uid, "0001750", 8);
    strncpy(p.gid, "0001750", 8);
    char aux[12];
    aux[11] = ' ';
    sprintf(aux, "%.11o", size);
    /* write size */
    strncpy(p.size, aux, 12);
    time_t t = time(NULL);
    sprintf(aux, "%.11lo", t);
    /* write mtime */
    strncpy(p.mtime, aux, 11);
    p.mtime[11] = 0;
    /* write typeflag */
    p.typeflag = '0';
    /* strcpy(p.linkname, ""); */
    /* write magic */
    memcpy(p.magic, "ustar ", 6);
    /* write version */
    p.version[0] = ' ';
    /* write uname and gname */
    strcpy(p.uname, "canoi");
    strcpy(p.gname, "canoi");
    /* strcpy(p.devmajor, "000000  ");
    strcpy(p.devminor, "000000  "); */
    int chksum = calc_checksum(p);
    sprintf(aux, "%.6o", chksum);
    aux[6] = 0;
    aux[7] = ' ';
    /* write checksum */
    memcpy(p.chksum, aux, 8);

    /* write blocks */

    fwrite(&p, 1, SBTAR_BLOCK_SIZE, tar->fp);
    int off = ftell(tar->fp);
    int blocks = 1 + (size / SBTAR_BLOCK_SIZE);
    posix_header_t paux;
    memset(&paux, 0, sizeof(paux));
    int i;
    for (i = 0; i < blocks + 2; i++) {
        fwrite(&paux, 1, SBTAR_BLOCK_SIZE, tar->fp);
    }
    tar->size = ftell(tar->fp);
    fseek(tar->fp, off, SEEK_SET);
    fwrite(text, 1, size, tar->fp);
}

void sbtar_write_dir(sbtar_t *tar, const char *dirname) {
    SBTAR_ASSERT(tar != NULL);
    SBTAR_ASSERT(dirname != NULL);
    if (sbtar_find(tar, dirname)) return;
    sbtar_rewind(tar);
    while (sbtar_next(tar));
    posix_header_t p;
    memset(&p, 0, sizeof(p));
    strcpy(p.name, dirname);
    /* write mode */
    strncpy(p.mode, "0000755", 8);
    /* write uid and gid */
    strncpy(p.uid, "0001750", 8);
    strncpy(p.gid, "0001750", 8);
    char aux[12];
    aux[11] = ' ';
    sprintf(aux, "%.11o", 0);
    /* write size */
    strncpy(p.size, aux, 12);
    time_t t = time(NULL);
    sprintf(aux, "%.11lo", t);
    /* write mtime */
    strncpy(p.mtime, aux, 11);
    p.mtime[11] = 0;
    /* write type flag */
    p.typeflag = '5';
    /* strcpy(p.linkname, ""); */
    /* write magic */
    memcpy(p.magic, "ustar ", 6);
    /* write version */
    p.version[0] = ' ';
    /* write uname and gname */
    strcpy(p.uname, "canoi");
    strcpy(p.gname, "canoi");
    /* strcpy(p.devmajor, "000000  ");
    strcpy(p.devminor, "000000  "); */
    int chksum = calc_checksum(p);
    sprintf(aux, "%.6o", chksum);
    aux[6] = 0;
    aux[7] = ' ';
    /* write checksum */
    memcpy(p.chksum, aux, 8);

    /* write blocks */

    fwrite(&p, 1, SBTAR_BLOCK_SIZE, tar->fp);
    memset(&p, 0, sizeof(p));
    int i;
    for (i = 0; i < 2; i++) fwrite(&p, 1, SBTAR_BLOCK_SIZE, tar->fp);
    tar->size = ftell(tar->fp);
}


#endif /* SBTAR_IMPLEMENTATION */