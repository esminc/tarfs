#include "tarfs_common_lnx.h"

static int tarfs_readdir(struct file * filp, void * dirent, filldir_t filldir);

struct file_operations tarfs_dir_operations = {
	read:		generic_read_dir,	/* just put error */
	readdir:	tarfs_readdir,
};

static int getDtype(int type)
{
	switch(type) {
	case TARFS_IFREG:
		return DT_REG;
	case TARFS_IFDIR:
		return DT_DIR;
	case TARFS_IFLNK:
		return DT_LNK;
	default:
		return DT_UNKNOWN;
	}
}

static int
tarfs_readdir(struct file * filp, void * dirent, filldir_t filldir)
{
	int err = 0;
	struct inode *ip;
	tarfs_dinode_t *parentp = NULL;
	tarfs_direct_t *dep = NULL;
	uint64_t off;
	uint64_t blkno;
	uint64_t endoff;
	tarfs_io_t io;

	ip = filp->f_dentry->d_inode;
	parentp = (tarfs_dinode_t *)ip->i_private;
        endoff = parentp->di_size;
	off = filp->f_pos & ~(TAR_BLOCKSIZE - 1);
	tarfs_io_oper.init(ip->i_sb, &io);

	//printk("tarfs_readdir:fops=%lld endoff=%lld\n", filp->f_pos, endoff);
	while (off < endoff) {
		//printk("tarfs_readdir:off=%lld\n", off);
		if ((off & (TAR_BLOCKSIZE -1)) == 0) { // 512 align
			if (io.io_data != NULL) {
				io.oper->bput(&io);
			}
			err = tarfs_getDataBlock(&io, parentp, off/TAR_BLOCKSIZE, &blkno);
			if (err != 0) {
				goto errdone;
			}
			err = io.oper->bread(&io, blkno);
			if (err != 0) {
				goto errdone;
			}
			dep = (tarfs_direct_t*)io.io_data;
		}
		if (off >= filp->f_pos && dep->d_ino != TARBLK_NONE) {
			err = filldir(dirent, dep->d_name, dep->d_namelen,
				filp->f_pos, dep->d_ino, getDtype(dep->d_ftype));
			if (err != 0) {
				goto errdone;
			}
		}
		if (off >= filp->f_pos) {
			filp->f_pos += dep->d_reclen;
			//printk("tarfs_readdir:filp->f_pos=%lld\n", filp->f_pos);
		}
		off += dep->d_reclen;
		dep = (tarfs_direct_t*)(((char*)dep) + dep->d_reclen);
	}
errdone: 
	if (io.io_data != NULL) {
		io.oper->bput(&io);
	}
	//printk("tarfs_readdir:ret=%d\n", err);
	return err;
}

