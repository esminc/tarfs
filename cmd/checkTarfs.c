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
 * Version: $Id: checkTarfs.c,v 1.5 2009/10/02 05:45:32 tmori Exp $
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
	struct stat buf_stat;
	uint64_t off;

	err = fstat(fd, &buf_stat);
	if (err < 0) {
		fprintf(stderr, "init_mkfs:fstat err=%d\n", errno);
		return 1;
	}

	memset(mp, 0, sizeof(mkfs_mgr_t));
	mp->m_fd = fd;
	mp->m_size = buf_stat.st_size/TAR_BLOCKSIZE;
	off = mp->m_size - 1;

	// super block
	res = pread(fd, &mp->m_sb, TAR_BLOCKSIZE, off*TAR_BLOCKSIZE);
	if (res != TAR_BLOCKSIZE){
		fprintf(stderr, "init:pread off=%lld err=%d\n", off, err);
		return err;
	}
	mp->m_root.i_ino = TARFS_FREE_INO;
	mp->m_root.i_pino = TARFS_ROOT_INO;
	res = pread(fd, &mp->m_free.i_dinode, TAR_BLOCKSIZE, mp->m_sb.tarfs_free*TAR_BLOCKSIZE);
	return 0;
}

int
main(int argc, const char* argv[])
{
	int err;
	int fd;
	char *fname;
	mkfs_mgr_t mgr;
	incore_dinode_t inode;

	if (argc != 3 && argc != 4) {
		fprintf(stderr, "Usage: %s tarfile ino [s]\n", argv[0]);
		return 1;
	}
	fname = (char*)argv[1];
	inode.i_ino = strtol(argv[2], NULL, 0);
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
	if (argc == 4) {
		printf("******* superblock ********\n");
		printSblock(&mgr.m_sb);
		printf("****************************\n");
	}

	err = getDinode(&mgr, &inode);
	if (err != 0) {
		fprintf(stderr, "getDinode ino=%llu err=%d\n", inode.i_ino, err);
		return 1;
	}
	printf("*******  dinode(%llu) ********\n", inode.i_ino);
	printDinode(inode.i_ino, &inode.i_dinode);
	printf("****************************\n");

	if (inode.i_dinode.di_mode & S_IFDIR) {
		printf("*******  dir data(%llu) ********\n", inode.i_ino);
		err = tarfs_readdir(&mgr, &inode.i_dinode);
		printf("****************************\n");
		if (err != 0) {
			fprintf(stderr, "tarfs_readdir err=%d\n", err);
			return 1;
		}
	}

	close(fd);
	return 0;
}

/* ----------------------デバッグ用-----------------------------------*/
void
printDinode(uint64_t ino, tarfs_dinode_t *dp)
{
	int i;
	int n;
	printf("#ino:%llu\n", ino);
	printf("#di_magic:0x%llx\n", dp->di_magic);
	printf("#di_mode:0x%x\n", dp->di_mode);
	printf("#di_uid:%u\n", dp->di_uid);
	printf("#di_gid:%u\n", dp->di_gid);
	printf("#di_nlink:%u\n", dp->di_nlink);
	printf("#di_blocks:%llu\n", dp->di_blocks);
	printf("#di_size:%llu\n", dp->di_size);
	printf("#di_atime:%llu\n", dp->di_atime);
	printf("#di_mtime:%llu\n", dp->di_mtime);
	printf("#di_ctime:%llu\n", dp->di_ctime);
	printf("#di_header:%llu\n", dp->di_header);
	printf("#di_nExtents:%llu\n", dp->di_nExtents);
	printf("#di_rootIndExt:%llu\n", dp->di_rootIndExt);
	printf("#di_flags:0x%x\n", dp->di_flags);
	if (dp->di_nExtents < MAX_DINODE_DIRECT) {
		n = dp->di_nExtents;
	} else {
		n = MAX_DINODE_DIRECT;
	}
	for (i = 0; i < n; i++) {
		printf("#di_directExtent[%d].de_off:%llu\n", 
				i, dp->di_directExtent[i].de_off);
		printf("#di_directExtent[%d].de_blkno:%llu\n", 
				i, dp->di_directExtent[i].de_blkno);
		printf("#di_directExtent[%d].de_nblks:%llu\n", 
				i, dp->di_directExtent[i].de_nblks);
	}
	return;
}
void
printSblock(tarfs_sblock_t *sbp)
{
	printf("#tarfs_magic:0x%llx\n", sbp->tarfs_magic);
	printf("#tarfs_inode:%llu\n", sbp->tarfs_inodes);
	printf("#tarfs_size:%llu\n", sbp->tarfs_size);
	printf("#tarfs_fsize:%llu\n", sbp->tarfs_fsize);
	printf("#tarfs_dsize:%llu\n", sbp->tarfs_dsize);
	printf("#tarfs_flist_regdata:%llu\n", sbp->tarfs_flist_regdata);
	printf("#tarfs_flist_dirdata:%llu\n", sbp->tarfs_flist_dirdata);
	printf("#tarfs_flist_iexdata:%llu\n", sbp->tarfs_flist_iexdata);
	printf("#tarfs_flist_indDinode:%llu\n", sbp->tarfs_flist_indDinode);
	printf("#tarfs_flist_dinode:%llu\n", sbp->tarfs_flist_dinode);
	printf("#tarfs_root:%llu\n", sbp->tarfs_root);
	printf("#tarfs_free:%llu\n", sbp->tarfs_free);
	printf("#tarfs_flags:0x%llx\n", sbp->tarfs_flags);
	printf("#tarfs_maxino:%llu\n", sbp->tarfs_maxino);
	return;
}

// parentp のディレクトリ内に fname が存在するかどうかチェックする。
int 
tarfs_readdir(mkfs_mgr_t *mp, tarfs_dinode_t *parentp)
{
	int i;
	int err;
	uint64_t blkno = 0;
	uint64_t off = 0;
	uint64_t endoff = parentp->di_size;
	char dirbuf[TAR_BLOCKSIZE];
	tarfs_direct_t *dp;

	while (off < endoff) {
		if ((off & (TAR_BLOCKSIZE -1)) == 0) { // 512 align
			err = getDataBlock(mp, parentp, off/TAR_BLOCKSIZE, &blkno);
			if (err != 0) {
				return err;
			}
			readData(mp, dirbuf, blkno, 1);
			dp = (tarfs_direct_t*)dirbuf;
		}
		printf("d_ino=%4llu d_reclen=%3d d_namelen=%2d d_ftype=%d name=",
				dp->d_ino, dp->d_reclen, dp->d_namelen, dp->d_ftype);
		for (i=0;i<dp->d_namelen;i++){
			printf("%c", dp->d_name[i]);
		}
		printf("\n");
		off += dp->d_reclen;
		dp = (tarfs_direct_t*)(((char*)dp) + dp->d_reclen);
	}
	return 0;
}

