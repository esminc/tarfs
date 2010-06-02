#include "tarfs_common_lnx.h"

static struct dentry *tarfs_lookup(struct inode * dir, struct dentry *dentry, struct nameidata *nd)
{
	int err = 0;
	tarfs_dinode_t *parentp = NULL;
	tarfs_direct_t *dep = NULL;
	uint64_t off;
	uint64_t blkno;
	uint64_t endoff;
	tarfs_io_t io;
	struct inode *ip = NULL;

	parentp = (tarfs_dinode_t *)dir->i_private;
       	endoff = parentp->di_size;
	off = 0;
	tarfs_io_oper.init(dir->i_sb, &io);

	while (off < endoff) {
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
		if (dep->d_ino != TARBLK_NONE && 
			dentry->d_name.len == dep->d_namelen &&
			!memcmp(dentry->d_name.name, dep->d_name, dentry->d_name.len)) {
			ip = tarfs_iget(dir->i_sb, dep->d_ino);
			break;
		}
		off += dep->d_reclen;
		dep = (tarfs_direct_t*)(((char*)dep) + dep->d_reclen);
	}
errdone: 
	if (io.io_data != NULL) {
		io.oper->bput(&io);
	}
	if (ip) {
		return d_splice_alias(ip, dentry);
	} else if (err == 0) {
		err = -ENOENT;
	}
	return ERR_PTR(err);
}

struct inode_operations tarfs_dir_inode_operations = {
	lookup:	tarfs_lookup,
};

