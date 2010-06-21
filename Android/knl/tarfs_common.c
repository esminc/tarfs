#include "tarfs_common.h"

// off に対応する物理ブロック番号を検索する。
// off: 512byte 単位の論理ファイルオフセット
// blkno: 512byte 単位の物理ブロック番号
int
tarfs_getDataBlock(tarfs_io_t *iop, tarfs_dinode_t *ip, uint64_t off, uint64_t *blkno)
{
	int err;
	int i;
	uint64_t inx;
	tarfs_indExt_t *indExtp;
	uint64_t ind_off;
	tarfs_dext_t *iextp;
	int n;

	if (ip->di_nExtents <= MAX_DINODE_DIRECT) {
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
		return -EINVAL;
	}
	// search indirect extents
	ind_off = ip->di_rootIndExt;
	while (1) {
		err = iop->oper->bread(iop, ind_off);
		if (err != 0) {
			goto errdone;
		}
		indExtp = (tarfs_indExt_t*)iop->io_data;
		ASSERT(indExtp->ie_magic == TARINDE_MAGIC);
		for (i = 0; i < indExtp->ie_num; i++) {
			iextp = &indExtp->ie_dext[i];
			if (iextp->de_off <= off &&
					     off < (iextp->de_off + iextp->de_nblks)) {
				goto done;
			}
		}
		ind_off = indExtp->ie_next;
		if (ind_off == TARBLK_NONE) {
			err = -EINVAL;
			goto errdone;
		}
		iop->oper->bput(iop);
	}
done:
	err = 0;
	inx = off - iextp->de_off;
	*blkno = iextp->de_blkno + inx;
errdone:
	if (iop->io_data) {
		iop->oper->bput(iop);
	}
	return err;
}
/* -------------------------------------------------------------------*/

