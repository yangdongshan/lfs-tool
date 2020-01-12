// Copyright 2019 Sergey Tyultyaev
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "vfs.h"

struct vfs *vfs_lfs_get(const char *image, bool write, size_t name_max, size_t io_size, size_t block_size,
                        size_t block_count);

int vfs_format(struct vfs *vfs);

int vfs_mount(struct vfs *vfs);

int vfs_unmount(struct vfs *vfs);

int vfs_remove(struct vfs *vfs, const char *path);

int vfs_rename(struct vfs *vfs, const char *oldpath, const char *newpath);

void *vfs_open(struct vfs *vfs, const char *pathname, int flags);

int vfs_close(struct vfs *vfs, void *fd);

int32_t vfs_read(struct vfs *vfs, void *fd, void *buf, size_t count);

int32_t vfs_write(struct vfs *vfs, void *fd, const void *buf, size_t count);

int32_t vfs_fsync(struct vfs *vfs, void *fd);

int32_t vfs_seek(struct vfs *vfs, void *fd, int32_t off, int whence);

int32_t vfs_tell(struct vfs *vfs, void *fd);

int32_t vfs_stat(struct vfs *vfs, const char *path, struct stat *s);


int vfs_mkdir(struct vfs *vfs, const char *pathname);

void * vfs_opendir(struct vfs *vfs, const char *path);

int vfs_closedir(struct vfs *vfs, void *dir);

struct vfs_dirent* vfs_readdir(struct vfs *vfs, void *dir);
















