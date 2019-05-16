/*
 * Created 190514 lynnl
 *
 * Hello FUSE filesystem implementation
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fuse.h>

/**
 * Should only used for `char[]'  NOT `char *'
 * Assume ends with null byte('\0')
 */
#define STRLEN(s)           (sizeof(s) - 1)

#define FSNAME              "hello-fs"
#define LOG(fmt, ...)       (void) printf("[" FSNAME "]: " fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)   (void) fprintf(stderr, "[" FSNAME "]: (ERR) " fmt, ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)   LOG("(DBG) " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG("(WARN) " fmt, ##__VA_ARGS__)

#define UNUSED(arg, ...)    (void) ((void) (arg), ##__VA_ARGS__)

#define assert_nonnull(p)   assert((p) != NULL)

static const char *file_path = "/hello.txt";
static const char file_content[] = "Hello world!\n";
static const size_t file_size = STRLEN(file_content);

static int hello_getattr(const char *path, struct stat *stbuf)
{
    assert_nonnull(path);
    assert_nonnull(stbuf);

    LOG_DBG("getattr()  path: %s", path);

    if (strcmp(path, "/") == 0) {
        (void) memset(stbuf, 0, sizeof(*stbuf));

        /* Root directory of hello-fs */
        stbuf->st_mode = S_IFDIR | 0755;    /* rwxr-xr-x */
        stbuf->st_nlink = 3;
    } else if (strcmp(path, file_path) == 0) {
        (void) memset(stbuf, 0, sizeof(*stbuf));

        stbuf->st_mode = S_IFREG | 0444;    /* r--r--r-- */
        stbuf->st_nlink = 1;
        stbuf->st_size = file_size;
    } else {
        /* No need to memset `stbuf' since it met an error */

        return -ENOENT;     /* Reject everything else */
    }

    return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
    assert_nonnull(path);
    assert_nonnull(fi);

    LOG_DBG("open()  path: %s fi->flags: %#x", path, fi->flags);

    if (strcmp(path, file_path) != 0) {
        return -ENOENT;     /* We have only one regular file */
    }

    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        return -EACCES;     /* Only O_RDONLY access mode is allowed */
    }

    return 0;
}

static int hello_read(
        const char *path,
        char *buf,
        size_t sz,
        off_t off,
        struct fuse_file_info *fi)
{
    assert_nonnull(path);
    assert_nonnull(buf);
    assert(off >= 0);
    assert_nonnull(fi);

    LOG_DBG("read()  path: %s size: %zu off: %lld fi->flags: %#x", path, sz, off, fi->flags);

    if (strcmp(path, file_path) != 0) {
        return -ENOENT;
    }

    /* Trying to read past EOF of file_path */
    if ((size_t) off >= file_size) {
        LOG_WARN("Read past EOF?!  off: %lld size: %zu", off, sz);
        return 0;
    }

    if (off + sz > file_size)
        sz = file_size - off;   /* Trim the read to the file size */

    (void) memcpy(buf, file_content + off, sz);

    return sz;
}

static int hello_readdir(
        const char *path,
        void *buf,
        fuse_fill_dir_t filler,
        off_t off,
        struct fuse_file_info *fi)
{
    assert_nonnull(path);
    assert_nonnull(buf);
    assert_nonnull(filler);
    assert(off >= 0);
    assert_nonnull(fi);

    LOG_DBG("readdir()  path: %s off: %lld fi->flags: %#x", path, off, fi->flags);

    /* hello-fs have only one directory(e.g. the root directory) */
    if (strcmp(path, "/") != 0) return -ENOENT;

    (void) filler(buf, ".", NULL, 0);           /* Current directory */
    (void) filler(buf, "..", NULL, 0);          /* Parent directory */
    (void) filler(buf, file_path + 1, NULL, 0); /* The only one regular file */

    return 0;
}

/* All FUSE functionalities NYI */
static struct fuse_operations hello_fs_op = {
    .getattr = hello_getattr,
    .open = hello_open,
    .read = hello_read,
    .readdir = hello_readdir,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &hello_fs_op, NULL);
}

