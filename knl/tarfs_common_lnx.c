#include "tarfs_common_lnx.h"


static int32_t lnx_io_init(void* dev, tarfs_io_t * iop);
static int32_t lnx_bread(tarfs_io_t *iop, uint64_t blkno);
static int32_t lnx_bget(tarfs_io_t *iop, uint64_t blkno);
static int32_t lnx_bput(tarfs_io_t *iop);
const tarfs_io_operation_t tarfs_io_oper = {
	.init	=	lnx_io_init,
	.bread	=	lnx_bread,
	.bget	= 	lnx_bget,
	.bput	=	lnx_bput,
};


static int32_t
lnx_io_init(void* dev, tarfs_io_t * iop)
{
	iop->io_dev = dev;
	iop->io_bh = NULL;
	iop->io_data = NULL;
	iop->oper = &tarfs_io_oper;
	DBG(("lnx_io_init:iop=%p\n", iop));
	return 0;
}
static int32_t
lnx_bread(tarfs_io_t *iop, uint64_t blkno)
{
	struct super_block *sb = iop->io_dev;
	struct buffer_head *bh;
	iop->io_blkno = blkno;
	DBG(("lnx_bread:iop=%p blkno=%llu\n", iop, blkno));
	bh = sb_bread(sb, iop->io_blkno);
	if (bh == NULL) {
		DBG(("lnx_bread:err exit\n"));
		return -EIO;
	}
	iop->io_bh = bh;
	iop->io_data = bh->b_data;
	DBG(("lnx_bread:ok exit:%p\n",bh));
	return 0;
}

static int32_t
lnx_bget(tarfs_io_t *iop, uint64_t blkno)
{
	struct super_block *sb;
	struct buffer_head *bh;

	if (iop == NULL) {
		return -EINVAL;
	}
 	sb = iop->io_dev;
	iop->io_blkno = blkno;
	bh = sb_getblk(sb, iop->io_blkno);
	if (bh == NULL) {
		return -EIO;
	}
	iop->io_bh = bh;
	iop->io_data = bh->b_data;
	return 0;
}

static int32_t
lnx_bput(tarfs_io_t *iop)
{
	if (iop && iop->io_bh) {
		DBG(("lnx_bput:iop=%p bh=%p\n", iop, iop->io_bh));
		brelse ((struct buffer_head*)iop->io_bh);
		DBG(("brelse:ok\n"));
		iop->io_bh = NULL;
		iop->io_data = NULL;
	}
	return 0;
}

