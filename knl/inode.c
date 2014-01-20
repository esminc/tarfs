#include "tarfs_common_lnx.h"

static void* tarfs_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	int err;
	tarfs_io_t io;
	tarfs_dinode_t *dp;
	struct posix_header *ph;

	dp = (tarfs_dinode_t*)dentry->d_inode->i_private;
	tarfs_io_oper.init(dentry->d_inode->i_sb, &io);
	err = io.oper->bread(&io, dp->di_header);
	if (err < 0) {
		return ERR_PTR(err);
	}
	ph = (struct posix_header*)io.io_data;
	nd_set_link(nd, ph->linkname);
	return io.io_bh;
}
void tarfs_put_link(struct dentry *dentry, struct nameidata *nd, void *cookie)
{
	brelse ((struct buffer_head*)cookie);
}

struct inode_operations tarfs_symlink_inode_operations = {
	readlink:       generic_readlink,
	follow_link:    tarfs_follow_link,
	put_link:    	tarfs_put_link,
};

static int tarfs_get_block(struct inode *inode, sector_t iblock, 
			   struct buffer_head *bh_result, int create)
{
	int err;
	uint64_t blkno;
	uint64_t off = iblock;
	tarfs_dinode_t *ip;
	tarfs_io_t io;

	tarfs_io_oper.init(inode->i_sb, &io);
	ip = (tarfs_dinode_t *)inode->i_private;
	err = tarfs_getDataBlock(&io, ip, off, &blkno);
	if (err != 0) {
		return err;
	}
	clear_buffer_new(bh_result);
	map_bh(bh_result, inode->i_sb, blkno);
	return 0;
}

int tarfs_readpage(struct file *file, struct page *page)
{
	return block_read_full_page(page, tarfs_get_block);
}

struct address_space_operations tarfs_aops = {
	readpage: tarfs_readpage,
};

#define TARSB(sb) ((struct tarfs_sblock*)((sb)->s_fs_info))

int tarfs_read_inode (struct inode * inode)
{
	int err;
	uint64_t blkno;
	struct super_block *sb = inode->i_sb;
	tarfs_io_t io;
	struct tarfs_dinode *dp;
	tarfs_dinode_t *free_dinode;
	struct tarfs_incore_sblock *i_sb;
	uint64_t atime;
	uint64_t mtime;
	uint64_t ctime;

	dp = kmem_cache_alloc(tarfs_inode_cachep, GFP_NOFS);
	if (dp == NULL) {
		err = -ENOMEM;
		goto errdone;
	}
	DBG(("tarfs_dinode_t:alloc=%p\n", dp));
	i_sb = (struct tarfs_incore_sblock*)(sb->s_fs_info);
	free_dinode = (tarfs_dinode_t*)&i_sb->free_dinode;

	tarfs_io_oper.init(inode->i_sb, &io);
	err = tarfs_getDataBlock(&io, free_dinode, inode->i_ino, &blkno);
	if (err != 0) {
		goto errdone;
	}
	err = io.oper->bread(&io, blkno);
	if (err != 0) {
		goto errdone;
	}
	memcpy(dp, io.io_data, TAR_BLOCKSIZE);
	io.oper->bput(&io);

	inode->i_mode = ((tarfs_dinode_t*)(dp))->di_mode;
	inode->i_uid = ((tarfs_dinode_t*)(dp))->di_uid;
	inode->i_gid = ((tarfs_dinode_t*)(dp))->di_gid;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
	inode->i_nlink = ((tarfs_dinode_t*)(dp))->di_nlink;
#else
	set_nlink(inode, ((tarfs_dinode_t*)(dp))->di_nlink);
#endif
	inode->i_size = ((tarfs_dinode_t*)(dp))->di_size;

	atime = dp->di_atime;
	mtime = dp->di_mtime;
	ctime = dp->di_ctime;
	do_div(atime, 1000000LL);
	do_div(mtime, 1000000LL);
	do_div(ctime, 1000000LL);
	inode->i_atime.tv_sec = atime;
	inode->i_ctime.tv_sec = ctime;
	inode->i_mtime.tv_sec = mtime;
	inode->i_private = ((tarfs_dinode_t*)(dp));

	if (S_ISREG(inode->i_mode)) {
		inode->i_fop = &tarfs_file_operations;
		inode->i_mapping->a_ops = &tarfs_aops;
	} else if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &tarfs_dir_inode_operations;
		inode->i_fop = &tarfs_dir_operations;
	} else if (S_ISLNK(inode->i_mode)) {
		inode->i_op = &tarfs_symlink_inode_operations;
	} else {
		;
	}
	return 0;
errdone:
	if (io.io_data != NULL) {
		io.oper->bput(&io);
	}
	return err;
}
struct inode *
tarfs_iget(struct super_block *sb, uint64_t ino)
{
	int err = 0;
	struct inode *ip;

	ip = iget_locked(sb, ino);
	if (!ip) {
		err = -ENOMEM;
		goto done;
	}
	if (!(ip->i_state & I_NEW)) {
                return ip;
	}
	err = tarfs_read_inode(ip);
done:
	unlock_new_inode(ip);
	if (err) {
		return ERR_PTR(err);
	}
	return ip;
}
