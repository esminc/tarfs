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
 * Version: $Id: tarfs_common.c,v 1.9 2009/11/01 23:24:03 tmori Exp $
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

#define GET_FREE_REGDATA	1
#define GET_FREE_DIRDATA	2
#define GET_FREE_IEXDATA	3
#define GET_FREE_INDDINODE	4
#define GET_FREE_DINODE		5
#define	MAX_BLOCKS	4096
// 空きデータ管理
static int allocData(mkfs_mgr_t *mp, int flag, char* bufp, uint64_t *blkno);
static int allocDinodeData(mkfs_mgr_t *mp, char* bufp, uint64_t *blkno);
static int _allocData(mkfs_mgr_t *mp, uint64_t *firstBlkp, uint32_t blocks);
// extent 管理
static int insertDataExtent(mkfs_mgr_t *mp, incore_dinode_t *ip, tarfs_dext_t *dep);
// ファイルシステム操作管理
static void setFirstDirData(char *dirbufp, uint64_t ino, uint64_t pino);

/*------------------------- 空きデータ管理 -----------------------*/
// 空きブロックを獲得する。
// mp: mkfs 実施用管理オブジェクト
// blocks：獲得ブロック数
static int
_allocData(mkfs_mgr_t *mp, uint64_t *firstBlkp, uint32_t blocks)
{
	int err;
	int i;
	tarfs_freeData_t *bufp;
	tarfs_freeData_t *orgbufp;
	uint64_t	off;
	uint64_t	soff;
	uint32_t	msize;
	if (*firstBlkp != TARBLK_NONE) {
		DBG(("_allocData1:EINVAL\n"));
		return EINVAL;
	}
	if (blocks > MAX_BLOCKS) {
		DBG(("_allocData2:EINVAL\n"));
		return EINVAL;
	}
	msize = TAR_BLOCKSIZE*blocks;
	orgbufp = bufp = malloc(msize);
	if (bufp == NULL) {
		err = errno;
		fprintf(stderr, "_allocData:malloc err=%d\n", err);
		return ENOMEM;
	}
	// setup free data image
	off = soff = mp->m_size;
	bufp->f_nextblkno = TARBLK_NONE;
	bufp->f_magic = TARFREE_MAGIC;
	off++;
	for (i = 1; i < blocks; i++) {
		bufp->f_nextblkno = off;
		bufp++;
		bufp->f_nextblkno = TARBLK_NONE;
		bufp->f_magic = TARFREE_MAGIC;
		off++;
	}
	// write down free data
	writeData(mp, (char*)orgbufp, soff, blocks);
	*firstBlkp = soff;
	free(orgbufp);
	return 0;
}

static int
allocData(mkfs_mgr_t *mp, int flag, char* bufp, uint64_t *blkno)
{
	int err;
	uint64_t *firstBlkp;
	tarfs_freeData_t *fdp;
	int num;

	if (flag == GET_FREE_REGDATA) {
		firstBlkp = &mp->m_sb.tarfs_flist_regdata;
		num = ALLOC_REGDATA_NUM;
	} else if (flag == GET_FREE_DIRDATA){
		firstBlkp = &mp->m_sb.tarfs_flist_dirdata;
		num = ALLOC_DIRDATA_NUM;
	} else if (flag == GET_FREE_IEXDATA){
		firstBlkp = &mp->m_sb.tarfs_flist_iexdata;
		num = ALLOC_IEXDATA_NUM;
	} else if (flag == GET_FREE_INDDINODE){
		firstBlkp = &mp->m_sb.tarfs_flist_indDinode;
		num = ALLOC_INDDINODE_NUM;
	} else {
		DBG(("allocData:flag=%d:EINVAL\n", flag));
		return EINVAL;
	}
	if (*firstBlkp == TARBLK_NONE) {
		err = _allocData(mp, firstBlkp, num);
		if (err != 0) {
			return err;
		}
	}
	*blkno = *firstBlkp;
	readData(mp, bufp, (*blkno), 1);
	fdp = (tarfs_freeData_t *)bufp;
	*firstBlkp = fdp->f_nextblkno;
	return 0;
}

static int
allocDinodeData(mkfs_mgr_t *mp, char* bufp, uint64_t *blkno)
{
	int err;
	uint64_t *firstBlkp;
	tarfs_freeData_t *fdp;
	tarfs_dext_t de;
	int num;

	firstBlkp = &mp->m_sb.tarfs_flist_dinode;
	num = ALLOC_DINODE_NUM;
	if (*firstBlkp == TARBLK_NONE) {
		err = _allocData(mp, firstBlkp, num);
		if (err != 0) {
			return err;
		}
		de.de_off   = mp->m_sb.tarfs_maxino;
		de.de_blkno = mp->m_sb.tarfs_flist_dinode;
		de.de_nblks = ALLOC_DINODE_NUM;
		err = insertDataExtent(mp, &mp->m_free, &de);
		//printDinode(0, &mp->m_free.i_dinode);
		if (err != 0) {
			return err;
		}
		writeData(mp, (char*)&mp->m_free.i_dinode, 
				mp->m_free.i_blkno, 1);
	}
	*blkno = *firstBlkp;
	readData(mp, bufp, (*blkno), 1);
	fdp = (tarfs_freeData_t *)bufp;
	*firstBlkp = fdp->f_nextblkno;
	return 0;
}
/* -------------------------------------------------------------------*/

/*------------------------- エクステント管理 -----------------------*/
static int
insertDataExtent(mkfs_mgr_t *mp, incore_dinode_t *ip, tarfs_dext_t *dep)
{
	int i;
	int err;
	tarfs_indExt_t indExt;
	tarfs_indExt_t new_indExt;
	uint64_t ind_off = 0;
	uint64_t new_ind_off;
	tarfs_dext_t *iextp;
	int n;
	if (ip->i_dinode.di_nExtents < MAX_DINODE_DIRECT) {
		n = ip->i_dinode.di_nExtents;
	} else {
		n = MAX_DINODE_DIRECT;
	}
	DBG(("insertDataExtent:dep->de_off=%llu de_nblks=%llu nExtents=%llu\n", 
		dep->de_off, dep->de_nblks, ip->i_dinode.di_nExtents));

	// search direct extents
	for (i = 0; i < n; i++) {
		iextp = &ip->i_dinode.di_directExtent[i];
		if ((iextp->de_blkno + iextp->de_nblks) == dep->de_blkno) {
			goto done;
		}
	}
	if (ip->i_dinode.di_nExtents == MAX_DINODE_DIRECT) {
		goto getblock;
	}
	
	if (ip->i_dinode.di_nExtents < MAX_DINODE_DIRECT) {
		int n = ip->i_dinode.di_nExtents;
		ip->i_dinode.di_directExtent[n] = *dep;
		ip->i_dinode.di_nExtents++;
		return 0;
	}
	// search indirect extents
	ind_off = ip->i_dinode.di_rootIndExt;
	while (1) {
		readData(mp, (char*)&indExt, ind_off, 1);
		DBG(("insertExtent:ind_off=%llu\n", ind_off));
		for (i = 0; i < indExt.ie_num; i++) {
			iextp = &indExt.ie_dext[i];
			if ((iextp->de_blkno + iextp->de_nblks) == dep->de_blkno) {
				DBG(("merged!!\n"));
				goto done;
			}
		}
		if (indExt.ie_next == TARBLK_NONE) {
			break;
		}
		ind_off = indExt.ie_next;
	}
	if (indExt.ie_num < MAX_INDEXT) {
		int n = indExt.ie_num;
		indExt.ie_dext[n] = *dep;
		indExt.ie_num++;
		writeData(mp, (char*)&indExt, ind_off, 1);
		DBG(("insertExtent:ind_off=%llu ie_num=%d\n", ind_off, indExt.ie_num));
		return 0;
	}
getblock:
	err = allocData(mp, GET_FREE_IEXDATA, (char*)&new_indExt, &new_ind_off);
	if (err != 0) {
		return err;
	}
	new_indExt.ie_magic = TARINDE_MAGIC;
	new_indExt.ie_next = TARBLK_NONE;
	new_indExt.ie_num = 1;
	new_indExt.ie_dext[0] = *dep;
	if (ip->i_dinode.di_nExtents == MAX_DINODE_DIRECT) {
		ip->i_dinode.di_rootIndExt = new_ind_off;
	} else {
		indExt.ie_next = new_ind_off;
		writeData(mp, (char*)&indExt, ind_off, 1);
	}
	writeData(mp, (char*)&new_indExt, new_ind_off, 1);
	DBG(("insertExtent:new_ind_off=%llu\n", new_ind_off));
	ip->i_dinode.di_nExtents++;
	return 0;
done:
	// merge!!
	iextp->de_nblks += dep->de_nblks;
	if (ind_off) {
		writeData(mp, (char*)&indExt, ind_off, 1);
	}
	return 0;
}

// off に対応する物理ブロック番号を検索する。
// off: 512byte 単位の論理ファイルオフセット
// blkno: 512byte 単位の物理ブロック番号
int
getDataBlock(mkfs_mgr_t *mp, tarfs_dinode_t *ip, uint64_t off, uint64_t *blkno)
{
	int i;
	uint64_t inx;
	tarfs_indExt_t indExt;
	uint64_t ind_off;
	tarfs_dext_t *iextp;
	int n;
	DBG(("getDataBlock:off=%llu\n", off));
	if (ip->di_nExtents < MAX_DINODE_DIRECT) {
		n = ip->di_nExtents;
	} else {
		n = MAX_DINODE_DIRECT;
	}

	// search direct extents
	for (i = 0; i < n; i++) {
		iextp = &ip->di_directExtent[i];
		if (iextp->de_off <= off &&
				     off < (iextp->de_off + iextp->de_nblks)) {
			goto done;
		}
	}
	if (ip->di_nExtents <= MAX_DINODE_DIRECT) {
		DBG(("getDataBlock:n=%d:ind_off=%llu:next=%llu:EINVAL\n", 
			n, ind_off, ip->di_nExtents));
		return EINVAL;
	}
	// search indirect extents
	ind_off = ip->di_rootIndExt;
	while (1) {
		readData(mp, (char*)&indExt, ind_off, 1);
		DBG(("getDataBlock:ind_off=%lld ie_num=%d\n", ind_off, indExt.ie_num));
		for (i = 0; i < indExt.ie_num; i++) {
			iextp = &indExt.ie_dext[i];
			DBG(("getDataBlock:off=%lld nblks=%lld\n", 
				iextp->de_off, iextp->de_nblks));
			if (iextp->de_off <= off &&
					     off < (iextp->de_off + iextp->de_nblks)) {
				goto done;
			}
		}
		ind_off = indExt.ie_next;
		if (ind_off == TARBLK_NONE) {
			DBG(("getDataBlock:%lld:EINVAL\n", off));
			return EINVAL;
		}
	}
done:
	inx = off - iextp->de_off;
	*blkno = iextp->de_blkno + inx;
	return 0;
}
/* -------------------------------------------------------------------*/

/*------------------------- dinode 管理 -----------------------*/

static int
setNewDirEntry(mkfs_mgr_t *mp, incore_dinode_t *parentp)
{
	int err;
	tarfs_dext_t de;
	uint64_t blkno;
	char buf[TAR_BLOCKSIZE];
	// get new entry
	err = allocData(mp, GET_FREE_DIRDATA, buf, &blkno);
	if (err != 0) {
		return err;
	}
	de.de_off = parentp->i_dinode.di_blocks;
	de.de_blkno = blkno;
	de.de_nblks = 1;
	err = insertDataExtent(mp, parentp, &de);
	if (err != 0) {
		return err;
	}
	setFirstDirData(buf, parentp->i_ino, parentp->i_pino);
	writeData(mp, buf, blkno, 1);
	parentp->i_dinode.di_blocks=1;
	parentp->i_dinode.di_size = TAR_BLOCKSIZE;
	writeData(mp,(char*)&parentp->i_dinode, parentp->i_blkno, 1);
	return 0;
}
// dinode 領域を確保する。
// mp: mkfs 実施用管理オブジェクト
// dp: dinode 格納領域
// ino: 獲得した inode 番号
// head_blkno: ヘッダ格納ブロック番号(ない場合は、ディレクトリとして認識する)
// ph: ヘッダ格納データ
// data_blkno: データ格納ブロック番号
int
allocDinode(mkfs_mgr_t *mp, incore_dinode_t *icp, int ftype)
{
	int err = 0;
	uint64_t now;
	struct posix_header *ph;
	incore_dinode_t *dp;

	dp = icp;
	ph = icp->i_ph;

	err = allocDinodeData(mp, (char*)&dp->i_dinode, &dp->i_blkno);
	if (err != 0) {
		fprintf(stderr, "allocDinode:allocDinodeData err=%d\n", err);
		return err;
	}
	DBG(("mp->m_sb.tarfs_flist_dinode=%llu\n", mp->m_sb.tarfs_flist_dinode));

	memset(&dp->i_dinode, 0, TAR_BLOCKSIZE);
	dp->i_dinode.di_magic = TARDINODE_MAGIC;
	now = getCurrentTime();
	dp->i_dinode.di_atime = now;
	dp->i_dinode.di_ctime = now;
	dp->i_dinode.di_mtime = now;
	dp->i_dinode.di_uid = 0;
	dp->i_dinode.di_gid = 0;
	dp->i_dinode.di_mode = (ftype|0755);
	dp->i_dinode.di_nlink = 1;
	if (ftype == TARFS_IFDIR) {
		dp->i_dinode.di_nlink++;
	}
	if (ph) {
		dp->i_dinode.di_mode = ftype;
		dp->i_dinode.di_mode |=getval8(ph->mode, sizeof(ph->mode));
		dp->i_dinode.di_uid =getval8(ph->uid, sizeof(ph->uid));
		dp->i_dinode.di_gid =getval8(ph->gid, sizeof(ph->gid));
		dp->i_dinode.di_mtime =getval8(ph->mtime, sizeof(ph->mtime));
		dp->i_dinode.di_mtime *=1000000;
		dp->i_dinode.di_atime = dp->i_dinode.di_ctime = dp->i_dinode.di_mtime;
		if (ftype == TARFS_IFREG) {
			dp->i_dinode.di_size =getval8(ph->size, sizeof(ph->size));
			dp->i_dinode.di_blocks = TARFS_BLOCKS(dp->i_dinode.di_size);
			if (dp->i_dinode.di_blocks > 0) {
				dp->i_dinode.di_nExtents = 1;
				dp->i_dinode.di_directExtent[0] = icp->i_de;
			}
		} else if (ftype == TARFS_IFLNK) {
			dp->i_dinode.di_size = strlen(ph->linkname);
			dp->i_dinode.di_blocks = 0;
		}
	}
	if (ftype == TARFS_IFREG) {
		mp->m_sb.tarfs_dsize += dp->i_dinode.di_blocks;
	}
	dp->i_ino = mp->m_sb.tarfs_maxino;
	if (ftype == TARFS_IFDIR) {
		err = setNewDirEntry(mp, dp);
		if (err != 0) {
			return err;
		}
	}
	mp->m_sb.tarfs_maxino++;
	mp->m_sb.tarfs_inodes++;
	writeData(mp, (char*)&dp->i_dinode, dp->i_blkno, 1);
	DBG(("arfs_flist_dinode=%llu\n", mp->m_sb.tarfs_flist_dinode));
	//printDinode(dp->i_ino, &dp->i_dinode);
	return 0;
}

int
allocFreeDinode(mkfs_mgr_t *mp, incore_dinode_t *icp)
{
	int err = 0;
	uint64_t now;
	incore_dinode_t *dp;
	tarfs_dext_t de;
	uint64_t *firstBlkp;
	tarfs_freeData_t * fdp;
	int num;

	firstBlkp = &mp->m_sb.tarfs_flist_dinode;
	num = ALLOC_DINODE_NUM;
	dp = icp;
	err = _allocData(mp, firstBlkp, num);
	if (err != 0) {
		return err;
	}
	de.de_off   = mp->m_sb.tarfs_maxino;
	de.de_blkno = mp->m_sb.tarfs_flist_dinode;
	de.de_nblks = ALLOC_DINODE_NUM;

	readData(mp, (char*)&dp->i_dinode, de.de_blkno, 1);
	fdp = (tarfs_freeData_t *)&dp->i_dinode;
	*firstBlkp = fdp->f_nextblkno;

	dp->i_blkno = de.de_blkno;
	memset(&dp->i_dinode, 0, TAR_BLOCKSIZE);
	dp->i_dinode.di_magic = TARDINODE_MAGIC;
	now = getCurrentTime();
	dp->i_dinode.di_atime = now;
	dp->i_dinode.di_ctime = now;
	dp->i_dinode.di_mtime = now;
	dp->i_dinode.di_header = TARBLK_NONE;
	dp->i_dinode.di_uid = 0;
	dp->i_dinode.di_gid = 0;
	dp->i_dinode.di_mode = (TARFS_IFREG|0755);
	dp->i_dinode.di_nlink = 1;
	dp->i_ino = mp->m_sb.tarfs_maxino;
	mp->m_sb.tarfs_maxino++;
	mp->m_sb.tarfs_inodes++;
	//printDinode(dp->i_ino, &dp->i_dinode);
	err = insertDataExtent(mp, &mp->m_free, &de);
	if (err == 0) {
		writeData(mp, (char*)&dp->i_dinode, dp->i_blkno, 1);
	}
	return err;
}

int
getDinode(mkfs_mgr_t *mp, incore_dinode_t *ip)
{
	int err;
	DBG(("getDinode:enter\n"));
	err = getDataBlock(mp, &mp->m_free.i_dinode, ip->i_ino, &ip->i_blkno);
	if (err != 0) {
		DBG(("getDinode:err exit\n"));
		return err;
	}
	readData(mp, (char*)&ip->i_dinode, ip->i_blkno, 1);
	DBG(("getDinode:ok exit\n"));
	return 0;
}
/* -------------------------------------------------------------------*/

/*------------------------- ファイルシステム操作管理 -----------------------*/
static void
setFirstDirData(char *dirbufp, uint64_t ino, uint64_t pino)
{
	tarfs_direct_t *dp;
	int dot_len;

	dp = (tarfs_direct_t *) dirbufp;
	dp->d_ino = ino;
	dp->d_name[0] = '.';
	dp->d_namelen = 1;
	dp->d_ftype = TARFS_FTYPE_DIR;
	dot_len = dp->d_reclen = DIRENT_SIZE(1);
	
	dp = (tarfs_direct_t *)((char*)dp + dp->d_reclen);
	dp->d_ino = pino;
	dp->d_name[0] = '.';
	dp->d_name[1] = '.';
	dp->d_namelen = 2;
	dp->d_ftype = TARFS_FTYPE_DIR;
	dp->d_reclen = TAR_BLOCKSIZE - dot_len;
	return;
}

struct lookup_op {
	char 		*lo_fname;
	int 		lo_namelen;
	int 		lo_needspace;
	uint64_t	lo_slotoff;
	uint64_t	lo_slotblkno;
	char		lo_slotbuf[TAR_BLOCKSIZE];
	uint64_t	lo_found;
};

static int 
tarfs_lookup(mkfs_mgr_t *mp, tarfs_dinode_t *parentp, struct lookup_op *op)
{
	int err;
	uint64_t off = 0;
	uint64_t endoff = parentp->di_size;
	uint64_t dirblkno;
	char 	 dirbuf[TAR_BLOCKSIZE];
	tarfs_direct_t *dp;
	int freesize;

	while (off < endoff) {
		if ((off & (TAR_BLOCKSIZE -1)) == 0) { // 512 align
			err = getDataBlock(mp, parentp, 
					off/TAR_BLOCKSIZE, &dirblkno);
			if (err != 0) {
				return err;
			}
			readData(mp, dirbuf, dirblkno, 1);
			dp = (tarfs_direct_t*)dirbuf;
		}
		/* exist entry check */
		if (dp->d_ino != TARBLK_NONE && dp->d_namelen == op->lo_namelen &&
			!strncmp(dp->d_name, op->lo_fname, op->lo_namelen)) {
			op->lo_found = dp->d_ino;
			err = EEXIST;
			goto done;
		}
		/* free space check */
		if (op->lo_slotoff == TARBLK_NONE) {
			freesize = dp->d_reclen;
			if (dp->d_ino != TARBLK_NONE) {
				freesize -= (int)DIRECT_SIZE(dp);
			}
			if (freesize > 0 && freesize >= op->lo_needspace) {
				op->lo_slotoff = off;
				op->lo_slotblkno = dirblkno;
				memcpy(op->lo_slotbuf, dirbuf, TAR_BLOCKSIZE);
			}
		}
		off += dp->d_reclen;
		dp = (tarfs_direct_t*)(((char*)dp) + dp->d_reclen);
	}
	err = ENOENT;

done:
	return err;
}

// ftype に応じたファイルを作成する。
int
tarfs_create(mkfs_mgr_t *mp, incore_dinode_t *parentp, 
		char *fname, incore_dinode_t *icp)
{
	int err;
	struct lookup_op op;
	int namelen;
	tarfs_direct_t *dp;
	tarfs_direct_t *ep;
	tarfs_dext_t de;
	int ftype;

	namelen = strlen(fname);

	op.lo_fname = fname;
	op.lo_namelen = namelen;
	op.lo_slotoff = TARBLK_NONE;
	op.lo_needspace = DIRENT_SIZE(namelen);
	if (icp->i_ph == NULL) {
		ftype = TARFS_IFDIR;
	} else {
		ftype = getFtype(icp->i_ph->typeflag);
	}
	err = tarfs_lookup(mp, &parentp->i_dinode, &op);
	if (err == EEXIST) {
		icp->i_ino = op.lo_found;
		err = getDinode(mp, icp);
		return err;
	} else if (err != ENOENT) {
		return err;
	}
	DBG(("fname=%s\n", fname));
	err = allocDinode(mp, icp, ftype);
	if (err != 0) {
		return err;
	}
	if ((icp->i_dinode.di_mode & TARFS_IFMT) == TARFS_IFDIR) {
		parentp->i_dinode.di_nlink++;
	}
	// existing entry
	if (op.lo_slotoff != TARBLK_NONE) {
		int inx = (int)(op.lo_slotoff & (uint64_t)(TAR_BLOCKSIZE -1));

		dp = (tarfs_direct_t *)&op.lo_slotbuf[inx];
		DBG(("slotoff=%lld\n", op.lo_slotoff));
		DBG(("inx=%d\n", inx));
		DBG(("dpsize=%d\n", DIRENT_SIZE(dp->d_namelen)));
		DBG(("dp->d_ino=%llu\n", dp->d_ino));
		DBG(("dp->d_namelen=%d\n", dp->d_namelen));
		DBG(("dp->d_reclen=%d\n", dp->d_reclen));
		if (dp->d_ino == TARBLK_NONE) {
			dp->d_ino = icp->i_ino;
			dp->d_ftype = TARFS_FTYPE_DINODE2DIR(icp->i_dinode.di_mode);
			dp->d_namelen = namelen;
			memcpy(dp->d_name, fname, namelen);
		} else {
			int org_reclen;
			org_reclen = dp->d_reclen;
			dp->d_reclen = DIRENT_SIZE(dp->d_namelen);
			ep = (tarfs_direct_t *)((char*)dp + dp->d_reclen);
			ep->d_ino = icp->i_ino;
			ep->d_ftype = TARFS_FTYPE_DINODE2DIR(icp->i_dinode.di_mode);
			ep->d_namelen = namelen;
			ep->d_reclen = org_reclen - DIRENT_SIZE(dp->d_namelen);
			memcpy(ep->d_name, fname, namelen);
			//DBG(("epsize=%d\n", DIRENT_SIZE(namelen)));
			//DBG(("ep->d_reclen=%d\n", ep->d_reclen));
			ASSERT((ep->d_reclen + inx + dp->d_reclen) <= TAR_BLOCKSIZE);
		}
		writeData(mp, (char*)op.lo_slotbuf, op.lo_slotblkno, 1);
		writeData(mp,(char*)&parentp->i_dinode, parentp->i_blkno, 1);
		return 0;
	}
	// get new entry
	err = allocData(mp, GET_FREE_DIRDATA, op.lo_slotbuf, &op.lo_slotblkno);
	if (err != 0) {
		return err;
	}
	de.de_off = parentp->i_dinode.di_blocks;
	de.de_blkno = op.lo_slotblkno;
	de.de_nblks = 1;
	err = insertDataExtent(mp, parentp, &de);
	if (err != 0) {
		return err;
	}
	ep = (tarfs_direct_t *)op.lo_slotbuf;
	ep->d_reclen = TAR_BLOCKSIZE;
	ep->d_ino = icp->i_ino;
	ep->d_ftype = TARFS_FTYPE_DINODE2DIR(icp->i_dinode.di_mode);
	ep->d_namelen = namelen;
	memcpy(ep->d_name, fname, namelen);

	DBG(("di_size=%llu\n", parentp->i_dinode.di_size));
	parentp->i_dinode.di_blocks++;
	parentp->i_dinode.di_size += TAR_BLOCKSIZE;
	writeData(mp, (char*)op.lo_slotbuf, op.lo_slotblkno, 1);
	writeData(mp,(char*)&parentp->i_dinode, parentp->i_blkno, 1);
	//writeData(mp, (char*)&icp->i_dinode, icp->i_blkno, 1);
	return 0;
}
/* -------------------------------------------------------------------*/

/* ----------------------IO操作-----------------------------------*/
void
do_rollback(mkfs_mgr_t *mp)
{
	(void)ftruncate(mp->m_fd, mp->m_orgsize*TAR_BLOCKSIZE);
	exit(1);
	return;
}

// data をディスクに書き戻す。
// mp: mkfs 実施用管理オブジェクト
// blkno: ブロック番号
// 前提：サイズは 512byte であること。
void
writeData(mkfs_mgr_t *mp, char *p, uint64_t blkno, ssize_t nblks)
{
	int err = 0;
	ssize_t res;
	DBG(("writeData:blkno=%llu size=%u\n", blkno, nblks));
	res = pwrite(mp->m_fd, p, TAR_BLOCKSIZE*nblks, blkno*(uint64_t)TAR_BLOCKSIZE);
	if (res != (TAR_BLOCKSIZE*nblks)) {
		err = errno;
		fprintf(stderr, "writeData:err = %d\n", err);
		do_rollback(mp);
		return;
	}
	if ((blkno + nblks) > mp->m_size) {
		mp->m_size = blkno + nblks;
		mp->m_sb.tarfs_size = (mp->m_size - mp->m_orgsize);
	}
	return;
}
void
readData(mkfs_mgr_t *mp, char *p, uint64_t blkno, ssize_t nblks)
{
	int err = 0;
	ssize_t res;
	DBG(("readData:blkno=%llu size=%u\n", blkno, nblks));
	res = pread(mp->m_fd, p, TAR_BLOCKSIZE*nblks, blkno*(uint64_t)TAR_BLOCKSIZE);
	if (res != (TAR_BLOCKSIZE*nblks)) {
		err = errno;
		fprintf(stderr, "readData:blkno=%lld err = %d\n", blkno, err);
		do_rollback(mp);
		return;
	}
	return;
}

/* -------------------------------------------------------------------*/

/* ----------------------その他-----------------------------------*/
/* 
 * Octal strtoul for tarfs.
 * tar size, mtime, etc. sometimes NOT null terminated.
 */
uint64_t
getval8(char *p, size_t siz)
{
	char buf[TAR_BLOCKSIZE];
	uint64_t val;
	int i;

	if (siz>TAR_BLOCKSIZE || siz <=0){
		DBG(("tarfs: zero or too long siz for getval8\n"));
	}
	for(i=0; i<siz; i++){
		if (!isspace(p[i]))	/* '\0' is control character */
			break;
	}

	memcpy(buf, p+i, siz-i);
	buf[siz-i]='\0';
	val=strtoull(buf, NULL, 8);
	//DBG("getval8 [%s] %ld\n", buf, val);
	return val;
}
uint32_t
getFtype(uint32_t typeflag)
{
	uint32_t ftype = -1;
	switch(typeflag){
	case TARFS_REGTYPE:
		ftype = TARFS_IFREG;
		break;
	case TARFS_DIRTYPE:
		ftype = TARFS_IFDIR;
		break;
	case TARFS_SYMTYPE:
		ftype = TARFS_IFLNK;
		break;
	default:
		break;
	}
	return ftype;
}

uint64_t 
getCurrentTime(void)
{
	uint64_t now;
	struct timeval tv;
	(void)gettimeofday(&tv, NULL);
	now = (((uint64_t)tv.tv_sec)*(uint64_t)1000000) + (uint64_t)tv.tv_usec;
	return now;
}
/* -------------------------------------------------------------------*/
/* -------------------------------------------------------------------*/

