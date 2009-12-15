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
 * Version: $Id: mkfs_tar.c,v 1.18 2009/11/01 23:20:28 tmori Exp $
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

static struct posix_header *
getph(int fd, uint64_t *blkp, struct posix_header *ph, char**pathp)
{
	ssize_t res;
	uint32_t size;

	switch(ph->typeflag){
	case TARFS_REGTYPE:
	case TARFS_AREGTYPE:
	case TARFS_LNKTYPE:
	case TARFS_SYMTYPE:
	case TARFS_CHRTYPE:
	case TARFS_BLKTYPE:
	case TARFS_DIRTYPE:
	case TARFS_FIFOTYPE:
	case TARFS_CONTTYPE:
		*pathp = ph->name;
		/* Posix header only */
		break;
	case TARFS_GNU_LONGNAME:
		DBG(("TARFS_GNU_LONGNAME\n"));
		size=getval8(ph->size, sizeof(ph->size)); /* path size */
		size = (size+TAR_BLOCKSIZE-1)/TAR_BLOCKSIZE;
		(*blkp)++;
		res = pread(fd, (*pathp), size*TAR_BLOCKSIZE, (*blkp)*TAR_BLOCKSIZE);
		if (res != (size*TAR_BLOCKSIZE)) {
			printf("tarfs: failed to read headers\n");
			return NULL;
		}
		DBG(("*pathp=%s\n", *pathp));
		*blkp += size;
		res = pread(fd, ph, TAR_BLOCKSIZE, (*blkp)*TAR_BLOCKSIZE);
		if (res != TAR_BLOCKSIZE) {
			printf("tarfs: failed to read headers\n");
			return NULL;
		}
		break;
	default:
		/* TARFS_GNU_LONGLINK */
		printf("tarfs: invalid or unsupported typeflag(%c). "
				"Headers can not be skipped correctly.\n", ph->typeflag);
		ph = NULL;
		break;
	}
	return ph;
}

// path からファイル名を一つ取得する。
static int
getFname(char* path, int start, char* fname, int *next, int len)
{
	int isLast = 1;
	int i = 0;
	char *p;
	
	p = &path[start];

	while ((start+i) < len) {
		if (*p == '/') {
			isLast = 0;
			break;
		} else if (*p == '\0') {
			isLast = 1;
			break;
		}
		fname[i] = *p;
		i++;
		p++;
	}
	if ((start+i) >= len) {
		printf("start=%d, i=%d, len=%d\n", start, i , len);
		return -1;
	}
	if (*p == '/' && ((start+i+1) == len || *(p+1) == '\0')) {
		isLast = 1;
	}
	fname[i] = '\0';
	*next = start+i+1;
	//printf("fname=%s next=%d isLast=%d\n", fname, *next, isLast);
	return isLast;
}

static int
mkFilePath(mkfs_mgr_t *mp, char* pathp, incore_dinode_t *icp)
{
	int len;
	int start = 0;
	int next = 0;
	int err;
	int isLast = 0;
	char *path;
	char fname[MAXNAMLEN+1];
	incore_dinode_t *parentp;
	incore_dinode_t parent;
	incore_dinode_t child;

	path = pathp;
	len = strlen(path) + 1;
	DBG(("mkFilePath=%s len=%d\n", path, len));
	parentp = &mp->m_root;
	while (1) {
		start = next;
		isLast = getFname(path, start, fname, &next, len);
		if (isLast == 0) {
			child.i_pino = parentp->i_ino;
			child.i_dinode.di_header = TARBLK_NONE;
			child.i_mp = mp;
			child.i_ph = NULL;
			err = tarfs_create(mp, parentp, fname, &child);
			if (err == EEXIST) {
				err = 0;
			}
			parent = child;
			parentp = &parent;
		} else if (isLast == 1) {
			icp->i_pino = parentp->i_ino;
			err = tarfs_create(mp, parentp, fname, icp);
			if (err == EEXIST) {
				err = 0;
			}
		} else {
			printf("invalid path=%d\n", isLast);
			err = EINVAL;
		}
		if (isLast || err != 0) {
			break;
		}
	}
	return err;
}

/*
 * Parse tar archive and gather necessary information
 */
static int 
parse_tar(mkfs_mgr_t *mp)
{
	int err;
	char buf[TAR_BLOCKSIZE];
	char path[4096], *pathp;
	ssize_t res;
	uint64_t tarblk;
	struct posix_header *ph;
	uint64_t size;
	uint64_t datasz;
	incore_dinode_t ic;

	tarblk=0;
	while (1) {
		res = pread(mp->m_fd, buf, TAR_BLOCKSIZE, tarblk*TAR_BLOCKSIZE);
		if (res != TAR_BLOCKSIZE){
			fprintf(stderr, "parse_tar:pread(2) err=%d\n", errno);
			return -1;
		}
		ph = (struct posix_header*)buf;
		if (ph->name[0] == '\0'){
			//fprintf(stderr, "parse_tar:end search\n");
			break;
		}
		/* Skip headers */
		pathp = path;
		ph = getph(mp->m_fd, &tarblk, ph, &pathp);
		if (!ph){
			break;
		}
		size = 0;
		if (ph->typeflag == TARFS_REGTYPE) {
			size += getval8(ph->size, sizeof(ph->size));
		}
		DBG(("typeflag=%c path=%s:size=%llu\n", ph->typeflag, ph->name, size));

		datasz = (size+TAR_BLOCKSIZE-1)/TAR_BLOCKSIZE;
		ic.i_dinode.di_header = tarblk;
		ic.i_mp = mp;
		ic.i_ph = ph;
		ic.i_de.de_off = 0;
		ic.i_de.de_blkno = tarblk + 1;
		ic.i_de.de_nblks = datasz;
		err = mkFilePath(mp, pathp, &ic);
		if (err != 0){
			fprintf(stderr, "mkFilePath err=%d\n", err);
			return -1;
		}
		/* Skip contents of the file */
		tarblk += datasz + 1;
	}
	return 0;
}

// mp を初期化する。
// mp: mkfs 実施用管理オブジェクト
// fd: tar ファイルのファイル記述子
static int
init_mkfs(mkfs_mgr_t *mp, int fd)
{
	//int i;
	int err;
	struct stat buf_stat;
	ssize_t res;
	uint64_t off;
	struct tarfs_sblock sb;
	//incore_dinode_t child;
	//incore_dinode_t parent;
	//incore_dinode_t *parentp;

	err = fstat(fd, &buf_stat);
	if (err < 0) {
		fprintf(stderr, "init_mkfs:fstat err=%d\n", errno);
		return 1;
	}
        off = (buf_stat.st_size/TAR_BLOCKSIZE) - 1;
	res = pread(fd, &sb, TAR_BLOCKSIZE, off*TAR_BLOCKSIZE);
	if (res != TAR_BLOCKSIZE){
		fprintf(stderr, "init_mkfs:pread err=%d\n", err);
	}
	if (sb.tarfs_magic == TARSBLOCK_MAGIC) {
		if (sb.tarfs_flags == TARSBLOCK_FLAG_OK) {
			return EEXIST;
		} else {
			(void)ftruncate(mp->m_fd, mp->m_orgsize*TAR_BLOCKSIZE);
		}
	}
	memset(mp, 0, sizeof(mkfs_mgr_t));
	mp->m_fd = fd;
	mp->m_size = buf_stat.st_size/TAR_BLOCKSIZE;
	mp->m_orgsize = buf_stat.st_size/TAR_BLOCKSIZE;

	// super block
	mp->m_sb.tarfs_magic = TARSBLOCK_MAGIC;
	mp->m_sb.tarfs_size = 0;
	mp->m_sb.tarfs_dsize = 0;
	mp->m_sb.tarfs_flist_regdata = TARBLK_NONE;
	mp->m_sb.tarfs_flist_dirdata = TARBLK_NONE;
	mp->m_sb.tarfs_flist_iexdata = TARBLK_NONE;
	mp->m_sb.tarfs_flist_indDinode = TARBLK_NONE;
	mp->m_sb.tarfs_flist_dinode = TARBLK_NONE;
	mp->m_sb.tarfs_free = (buf_stat.st_size/TAR_BLOCKSIZE);
	mp->m_sb.tarfs_root = (buf_stat.st_size/TAR_BLOCKSIZE)+1;
	mp->m_sb.tarfs_flags = TARSBLOCK_FLAG_NG;

	// free dinode
	mp->m_free.i_mp = mp;
	mp->m_free.i_ph = NULL;
	mp->m_free.i_ino = 0;
	mp->m_free.i_pino = TARFS_ROOT_INO;
	err = allocFreeDinode(mp, &mp->m_free);
	if (err != 0){
		fprintf(stderr, "init_mkfs:allocDinode err=%d\n", err);
		return err;
	}

	// root dinode
	mp->m_root.i_mp = mp;
	mp->m_root.i_ph = NULL;
	mp->m_root.i_ino = TARFS_ROOT_INO;
	mp->m_root.i_pino = TARFS_ROOT_INO;
	err = allocDinode(mp, &mp->m_root, TARFS_IFDIR);
	if (err != 0){
		fprintf(stderr, "init_mkfs:allocDinode err=%d\n", err);
		return err;
	}

	return 0;
}

int
main(int argc, const char* argv[])
{
	int err;
	int fd;
	char *fname;
	mkfs_mgr_t mgr;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s tarfile\n", argv[0]);
		return 1;
	}
	fname = (char*)argv[1];
	fd = open(fname, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open %s err=%d\n", fname, errno);
		return 1;
	}
	err = init_mkfs(&mgr, fd);
	if (err == EEXIST) {
		//printf("meta-data is already standby.. so nop.\n");
		close(fd);
		return 0;
	} else if (err!= 0) {
		fprintf(stderr, "init_mkfs err=%d\n", err);
		return 1;
	}
	err = parse_tar(&mgr);
	if (err != 0) {
		do_rollback(&mgr);
	}
	mgr.m_sb.tarfs_flags = TARSBLOCK_FLAG_OK;
	writeData(&mgr, (char*)&mgr.m_sb, mgr.m_size, 1);

	close(fd);
	return 0;
}

