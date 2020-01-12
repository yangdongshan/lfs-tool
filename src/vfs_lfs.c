/**
 * Copyright 2019 Sergey Tyultyaev
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vfs_lfs.h"

#include "macro.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "vfs.h"
#include "lfs/lfs.h"

#define BLOCK_SIZE 4096
#define IO_SIZE 256

struct context
{
    FILE *file;
};

static lfs_t *g_lfs = NULL;

static int fs_read(const struct lfs_config *c, lfs_block_t block,
                   lfs_off_t off, void *buffer, lfs_size_t size);
static int fs_prog(const struct lfs_config *c, lfs_block_t block,
                   lfs_off_t off, const void *buffer, lfs_size_t size);
static int fs_erase(const struct lfs_config *c, lfs_block_t block);
static int fs_sync(const struct lfs_config *c);

static struct lfs_config m_lfs_config = {
    .read = fs_read,
    .prog = fs_prog,
    .erase = fs_erase,
    .sync = fs_sync,
    .read_size = IO_SIZE,
    .prog_size = IO_SIZE,
    .block_size = BLOCK_SIZE,
    .cache_size = IO_SIZE,
    .lookahead_size = IO_SIZE,
    .block_cycles = -1,
};

static struct context m_context = {0};

static pthread_mutex_t mutex;

static int fs_read(const struct lfs_config *c, lfs_block_t block,
                   lfs_off_t off, void *buffer, lfs_size_t size)
{
    int result = 0;
    struct context *context = c->context;

    size_t offset = c->block_size * block + off;

    int err = fseek(context->file, offset, SEEK_SET);
    CHECK_ERROR(err == 0, -1, "fseek() failed: %d", err);

    size_t bytes = fread(buffer, 1, size, context->file);
    CHECK_ERROR(bytes == size, -1, "fread() failed: off: %u, size: %u, bytes: %ld", off, size, bytes);

    //INFO("read block: %u, off: %u", block, off);
    //INFO("read offset: %u, size: %u", offset, size);

done:
    return result;
}

static int fs_prog(const struct lfs_config *c, lfs_block_t block,
                   lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int result = 0;
    struct context *context = c->context;

    size_t offset = c->block_size * block + off;

    int err = fseek(context->file, offset, SEEK_SET);
    CHECK_ERROR(err == 0, -1, "fseek() failed: %d", err);

    size_t bytes = fwrite(buffer, 1, size, context->file);
    CHECK_ERROR(bytes == size, -1, "fwrite() failed");

done:
    return result;
}

static int fs_erase(const struct lfs_config *c, lfs_block_t block)
{
    int result = 0;
    struct context *context = c->context;

    size_t offset = c->block_size * block;

    int err = fseek(context->file, offset, SEEK_SET);
    CHECK_ERROR(err == 0, -1, "fseek() failed: %d", err);

    for (size_t i = 0; i < c->block_size; i++) {
        int c = fputc(0xFF, context->file);
        CHECK_ERROR(c == 0xff, -1, "fputc() failed: %d", c);
    }

done:
    return result;
}

static int fs_sync(const struct lfs_config *c)
{
    struct context *context = c->context;
    return fflush(context->file) != EOF ? 0 : -1;
}

static int vfs_lock_init(void)
{
	return pthread_mutex_init(&mutex, NULL);
}

static int vfs_lock(void)
{
	return pthread_mutex_lock(&mutex);
}

static int vfs_unlock(void)
{
	return pthread_mutex_unlock(&mutex);
}

static int vfs_lock_destory(void)
{
	return pthread_mutex_destroy(&mutex);
}

int vfs_format(struct vfs *vfs)
{
    int result = 0;

    lfs_t *lfs = NULL;

    CHECK_ERROR(vfs != NULL, -1, "vfs == NULL");

    lfs = malloc(sizeof(*lfs));
    CHECK_ERROR(lfs != NULL, -1, "format() failed");

    result = lfs_format(lfs, &m_lfs_config);
    CHECK_ERROR(result == 0, -1, "lfs_format() failed: %d", result);

done:
    if (lfs != NULL)
		free(lfs);

    return result;
}


int vfs_mount(struct vfs *vfs)
{
    int result = 0;

    CHECK_ERROR(vfs != NULL, -1, "vfs == NULL");

	CHECK_ERROR(g_lfs == NULL, -1, "g_lfs != NULL");

    g_lfs = malloc(sizeof(*g_lfs));
    CHECK_ERROR(g_lfs != NULL, -1, "malloc() failed");

	vfs_lock_init();

    result = lfs_mount(g_lfs, &m_lfs_config);
    CHECK_ERROR(result == 0, -1, "lfs_mount() failed: %d", result);

done:
    if (result != 0) {
        free(g_lfs);
		g_lfs = NULL;
		vfs_lock_destory();
    }
    return result;
}

int vfs_unmount(struct vfs *vfs)
{
    int result = 0;

    if (g_lfs == NULL)
		return -1;

    result = lfs_unmount(g_lfs);
    CHECK_ERROR(result == 0, -1, "lfs_unmount() failed: %d", result);

    free(g_lfs);
	g_lfs = NULL;

	vfs_lock_destory();

done:
    return result;
}

int vfs_remove(struct vfs *vfs, const char *path)
{
	int result;

    CHECK_ERROR(path != NULL, -1, "path == NULL");

	vfs_lock();
    int err = lfs_remove(g_lfs, path);
	vfs_unlock();
    CHECK_ERROR(err == 0, -1, "lfs_remove() failed: %d", err);

done:
    return result;
}

int vfs_rename(struct vfs *vfs, const char *oldpath, const char *newpath)
{
	int result;

    CHECK_ERROR(oldpath != NULL, -1, "oldpath == NULL");
	CHECK_ERROR(newpath != NULL, -1, "newpath == NULL");

	vfs_lock();
    int err = lfs_rename(g_lfs, oldpath, newpath);
	vfs_unlock();
    CHECK_ERROR(err == 0, -1, "lfs_rename() failed: %d", err);

done:
    return result;
}

void *vfs_open(struct vfs *vfs, const char *pathname, int flags)
{
    void *result = NULL;

    lfs_file_t *file = NULL;

    CHECK_ERROR(pathname != NULL, NULL, "pathname == NULL");

    file = malloc(sizeof(*file));

    CHECK_ERROR(file != NULL, NULL, "malloc() failed");

    int lfs_flags = 0;
    if (flags & O_RDONLY) {
        lfs_flags |= LFS_O_RDONLY;
    }
    if (flags & O_RDWR) {
        lfs_flags |= LFS_O_RDWR;
    }
    if (flags & O_WRONLY) {
        lfs_flags |= LFS_O_WRONLY;
    }
    if (flags & O_TRUNC) {
        lfs_flags |= LFS_O_TRUNC;
    }
    if (flags & O_CREAT) {
        lfs_flags |= LFS_O_CREAT;
    }
    if (flags & O_APPEND) {
        lfs_flags |= LFS_O_APPEND;
    }

	vfs_lock();
    int err = lfs_file_open(g_lfs, file, pathname, lfs_flags);
	vfs_unlock();

    CHECK_ERROR(err >= 0, NULL, "lfs_file_open() failed: %d", err);

    result = file;

done:
    if (result == NULL) {
        free(file);
    }
    return result;
}

int vfs_close(struct vfs *vfs, void *fd)
{
    int result = 0;

    lfs_file_t *file = NULL;

    CHECK_ERROR(fd != NULL, -1, "fd == NULL");

    file = fd;

	vfs_lock();
    int err = lfs_file_close(g_lfs, file);
	vfs_unlock();

    CHECK_ERROR(err == 0, -1, "lfs_file_close() failed: %d", err);

    free(file);

done:
    return result;
}

int32_t vfs_read(struct vfs *vfs, void *fd, void *buf, size_t count)
{
    int32_t result = 0;

    CHECK_ERROR(fd != NULL, -1, "fd == NULL");
    CHECK_ERROR(buf != NULL, -1, "buf == NULL");

    lfs_file_t *file = fd;

	vfs_lock();
    result = lfs_file_read(g_lfs, file, buf, count);
	vfs_unlock();

    CHECK_ERROR(result >= 0, -1, "lfs_file_read() failed: %d", result);

done:
    return result;
}

int32_t vfs_write(struct vfs *vfs, void *fd, const void *buf, size_t count)
{
    int32_t result = 0;

    CHECK_ERROR(fd != NULL, -1, "fd == NULL");
    CHECK_ERROR(buf != NULL, -1, "buf == NULL");

    lfs_file_t *file = fd;

	vfs_lock();
    result = lfs_file_write(g_lfs, file, buf, count);
	vfs_unlock();
    CHECK_ERROR(result >= 0, -1, "lfs_file_write() failed: %d", result);

done:
    return result;
}

int32_t vfs_fsync(struct vfs *vfs, void *fd)
{
    int32_t result = 0;

    CHECK_ERROR(fd != NULL, -1, "fd == NULL");

    lfs_file_t *file = fd;

	vfs_lock();
    result = lfs_file_sync(g_lfs, file);
	vfs_unlock();

    CHECK_ERROR(result >= 0, -1, "lfs_file_sync() failed: %d", result);

done:
    return result;
}

int32_t vfs_seek(struct vfs *vfs, void *fd, int32_t off, int whence)
{
	int32_t result = 0;

    CHECK_ERROR(fd != NULL, -1, "fd == NULL");

    lfs_file_t *file = fd;

	vfs_lock();
	result = lfs_file_seek(g_lfs, file, off, whence);
	vfs_unlock();
	CHECK_ERROR(result >= 0, -1, "lfs_file_sync() failed: %d", result);

done:
	return result;
}


int32_t vfs_tell(struct vfs *vfs, void *fd)
{
	int32_t result = 0;

    CHECK_ERROR(fd != NULL, -1, "fd == NULL");

    lfs_file_t *file = fd;

	vfs_lock();
	result = lfs_file_tell(g_lfs, file);
	vfs_unlock();

	CHECK_ERROR(result >= 0, -1, "lfs_file_tell() failed: %d", result);

done:
	return result;
}

int32_t vfs_stat(struct vfs *vfs, const char *path, struct stat *s)
{
	int32_t result = 0;

    CHECK_ERROR(path != NULL, -1, "path== NULL");

	struct lfs_info info;

	vfs_lock();
	result = lfs_stat(g_lfs, path, &info);
	vfs_unlock();

	if (!result) {
		s->st_size = info.size;
		s->st_mode = S_IRWXU | S_IRWXG | S_IRWXO |
		             ((info.type == LFS_TYPE_DIR)? S_IFDIR: S_IFREG);
	}

	CHECK_ERROR(result >= 0, -1, "lfs_file_tell() failed: %d", result);

done:
	return result;
}


int vfs_mkdir(struct vfs *vfs, const char *pathname)
{
    int result = 0;

    CHECK_ERROR(pathname != NULL, -1, "pathname == NULL");

	vfs_lock();
    int err = lfs_mkdir(g_lfs, pathname);
	vfs_unlock();

    CHECK_ERROR(err == 0 || err == LFS_ERR_EXIST, -1, "lfs_mkdir() failed: %d", err);

done:
    return result;
}

void * vfs_opendir(struct vfs *vfs, const char *path)
{
    void *result = NULL;

    lfs_dir_t *dir = NULL;

    CHECK_ERROR(path != NULL, NULL, "path == NULL");

    dir = malloc(sizeof(*dir));
    CHECK_ERROR(dir != NULL, NULL, "malloc() failed");

	vfs_lock();
    int err = lfs_dir_open(g_lfs, dir, path);
	vfs_unlock();

    CHECK_ERROR(err == 0, NULL, "lfs_dir_open() failed: %d", err);

    result = dir;

done:
    if (result == NULL) {
        free(dir);
    }
    return result;
}

int vfs_closedir(struct vfs *vfs, void *dir)
{
    int result = 0;

    CHECK_ERROR(dir != NULL, -1, "dir == NULL");

    lfs_dir_t *lfs_dir = dir;

	vfs_lock();
	int err = lfs_dir_close(g_lfs, lfs_dir);
	vfs_unlock();

    CHECK_ERROR(err == 0, -1, "lfs_dir_close() failed: %d", err);

    free(lfs_dir);

done:
    return result;
}

struct vfs_dirent* vfs_readdir(struct vfs *vfs, void *dir)
{
    struct vfs_dirent *result = NULL;

    CHECK_ERROR(dir != NULL, NULL, "dir == NULL");

    lfs_dir_t *lfs_dir = dir;

    struct lfs_info info = {0};

	vfs_lock();
    int err = lfs_dir_read(g_lfs, lfs_dir, &info);
	vfs_unlock();

    CHECK_ERROR(err >= 0, NULL, "lfs_dir_read() failed: %d", err);

    static struct vfs_dirent dirent = {0};

    if (err == 0)
    {
        dirent.name[0] = '\0';
        dirent.type = VFS_TYPE_END;
    }
    else
    {
        CHECK_ERROR(strlen(info.name) < sizeof(dirent.name), NULL, "info.name is too small");
        strncpy(dirent.name, info.name, sizeof(dirent.name) - 1);
        dirent.type = info.type == LFS_TYPE_REG ? VFS_TYPE_FILE : VFS_TYPE_DIR;
    }

    result = &dirent;

done:
    return result;
}


static struct vfs vfs_lfs = {
	.format = vfs_format,
    .mount = vfs_mount,
    .unmount = vfs_unmount,
    .remove = vfs_remove,
    .rename = vfs_rename,

    .open = vfs_open,
    .close = vfs_close,
    .read = vfs_read,
    .write = vfs_write,
    .fsync = vfs_fsync,
    .seek = vfs_seek,
    .tell = vfs_tell,

	.mkdir = vfs_mkdir,
    .opendir = vfs_opendir,
    .closedir = vfs_closedir,
    .readdir = vfs_readdir,
};

struct vfs *vfs_lfs_get(const char *image, bool write, size_t name_max, size_t io_size, size_t block_size,
                        size_t block_count)
{
    struct vfs *result = NULL;

    m_context.file = fopen(image, write ? "w+b" : "rb");
    CHECK_ERROR(m_context.file != NULL, NULL, "fopen() failed: %s", strerror(errno));

    m_lfs_config.context = &m_context;

    if (io_size != 0) {
        m_lfs_config.read_size = io_size;
        m_lfs_config.prog_size = io_size;
        m_lfs_config.cache_size = io_size;
        m_lfs_config.lookahead_size = io_size;
    }

    if (block_size != 0) {
        m_lfs_config.block_size = block_size;
    }

    m_lfs_config.block_count = block_count != 0 ? block_count : 4059;
    m_lfs_config.name_max = name_max;

    if (write) {
        for (size_t i = 0; i < m_lfs_config.block_count * m_lfs_config.block_size; i++) {
            int c = fputc(0xff, m_context.file);
            CHECK_ERROR(c == 0xFF, NULL, "fputc() failed: %d", c);
        }

        lfs_t lfs = {0};
        int err = lfs_format(&lfs, &m_lfs_config);
        CHECK_ERROR(err == 0, NULL, "lfs_format() failed: %d", err);
    }

    result = &vfs_lfs;

done:
    return result;
}
