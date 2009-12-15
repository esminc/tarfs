/*
 *  tarfs/tarent.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Version: $Id: tarfsmount.c,v 1.2 2009/10/20 10:50:24 nmiya Exp $
 *
 * Copyright (C) 2001
 * Kazuto Miyoshi (kaz@earth.email.ne.jp)
 * Hirokazu Takahashi (h-takaha@mub.biglobe.ne.jp)
 *
 */

#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <stdlib.h>
#include "tarfs_meta.h"
#include "tarfs_common.h"


// mp を初期化する。
// mp: mkfs 実施用管理オブジェクト
// fd: tar ファイルのファイル記述子
static int
init(mkfs_mgr_t *mp, int fd)
{
	int err;
	ssize_t res;
	uint64_t off;
	uint64_t blksz;

	err = ioctl(fd, BLKGETSIZE64, &blksz);
	if (err != 0) {
		fprintf(stderr, "init:ioctl(2) err=%d\n", errno);
		return 1;
	}

	memset(mp, 0, sizeof(mkfs_mgr_t));
	mp->m_fd = fd;
	mp->m_size = blksz/TAR_BLOCKSIZE;
	off = mp->m_size - 1;


	// root dinode
	res = pread(fd, &mp->m_sb, TAR_BLOCKSIZE, off*TAR_BLOCKSIZE);
	if (res != TAR_BLOCKSIZE){
		err = errno;
		fprintf(stderr, "init:pread err=%d\n",err);
		return err;
	}
	mp->m_root.i_ino = TARFS_FREE_INO;
	mp->m_root.i_pino = TARFS_ROOT_INO;
	res = pread(fd, &mp->m_free.i_dinode, TAR_BLOCKSIZE, 
		mp->m_sb.tarfs_free*TAR_BLOCKSIZE);
	if (res != TAR_BLOCKSIZE){
		err = errno;
		fprintf(stderr, "init:pread err=%d\n",err);
		return err;
	}
	mp->m_root.i_ino = TARFS_ROOT_INO;
	mp->m_root.i_pino = TARFS_ROOT_INO;
	res = pread(fd, &mp->m_root.i_dinode, TAR_BLOCKSIZE, 
		mp->m_sb.tarfs_root*TAR_BLOCKSIZE);
	if (res != TAR_BLOCKSIZE){
		err = errno;
		fprintf(stderr, "init:pread err=%d\n",err);
		return err;
	}
	return 0;
}

int
main(int argc, const char* argv[])
{
	int err;
	int fd;
	int mntflg = 0;
	char *fname;
	char *mntpoint;
	mkfs_mgr_t mgr;
	//incore_dinode_t inode;
	struct tarfs_mnt_arg arg;

	fname = (char*)argv[1];
	mntpoint = (char*)argv[2];
	fd = open(fname, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open %s err=%d\n", fname, errno);
		return 1;
	}
	err = init(&mgr, fd);
	if (err != 0) {
		fprintf(stderr, "init err=%d\n", err);
		return 1;
	}
	close(fd);

	arg.mnt_sb = mgr.m_sb;
	arg.mnt_dinode = mgr.m_free.i_dinode;

	mount(fname, mntpoint, "tarfs", mntflg, &arg);
	return 0;
}

