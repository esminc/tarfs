#include "tarfs_common.h"
#include "tarfs_common_lnx.h"

struct kmem_cache *tarfs_inode_cachep;

static void
init_once(void *foo)
{
	return;
}
static int 
init_tarfs_inodecache(void)
{
	tarfs_inode_cachep = kmem_cache_create("tarfs_inode_cache",
				sizeof(tarfs_dinode_t),
				0, (SLAB_RECLAIM_ACCOUNT|
					SLAB_MEM_SPREAD),
					init_once);
	if(tarfs_inode_cachep == NULL) {
		return -ENOMEM;
	}
	return 0;
}
static void
destroy_inodecache(void)
{
	kmem_cache_destroy(tarfs_inode_cachep);
}

void tarfs_put_super(struct super_block *sb)
{
	if (sb->s_fs_info) {
		kfree(sb->s_fs_info);
		sb->s_fs_info = NULL;
	}
	return;
}

int tarfs_statfs (struct dentry * dentry, struct kstatfs * buf)
{
	struct super_block *sb = dentry->d_inode->i_sb;
	struct tarfs_sblock *t_sb= NULL;
	struct tarfs_incore_sblock *i_sb= NULL;
	i_sb = sb->s_fs_info;
	t_sb = &i_sb->sb;
	buf->f_type = TARSBLOCK_MAGIC;
	buf->f_bsize = TAR_BLOCKSIZE;
	buf->f_blocks = t_sb->tarfs_dsize;
	buf->f_bfree = 0;
	buf->f_bavail = 0;
	buf->f_files = t_sb->tarfs_inodes;
	buf->f_ffree = 0;
	buf->f_namelen = MAXNAMLEN;
	return 0;
}

int tarfs_remount (struct super_block * sb, int * flags, char * data)
{
	return -EINVAL;
}

static void tarfs_clear_inode(struct inode *inode) 
{
	DBG(("tarfs_dinode_t:freep=%p\n", inode->i_private));
	if (inode->i_private) {
		kmem_cache_free(tarfs_inode_cachep, inode->i_private);
		inode->i_private = NULL;
	}
	return;
}	

extern void tarfs_read_inode(struct inode* ip);

static struct super_operations tarfs_sops = {
	put_super:      tarfs_put_super,
	statfs:         tarfs_statfs,
	remount_fs:     tarfs_remount,
	clear_inode:    tarfs_clear_inode,
};

static int tarfs_fill_super (struct super_block *sb, void *data, int silent)
{
	int i;
	struct inode *ip;
	struct block_device *bdev = sb->s_bdev;
	struct tarfs_sblock *t_sb= NULL;
	tarfs_dinode_t *t_dp;
	tarfs_dinode_t *s_dp;
	struct tarfs_incore_sblock *i_sb= NULL;
	struct tarfs_mnt_arg *arg=(struct tarfs_mnt_arg *)data;

	i_sb = kmalloc(sizeof(struct tarfs_incore_sblock), GFP_KERNEL);	
	if (!i_sb) {
		printk("%s err can not allocate\n",__FUNCTION__);
		return -ENOMEM; 
	}
	t_sb = &i_sb->sb;
	t_dp = &i_sb->free_dinode;
	s_dp = &arg->mnt_dinode;

	// set super block
	t_sb->tarfs_magic = arg->mnt_sb.tarfs_magic;
        t_sb->tarfs_inodes = arg->mnt_sb.tarfs_inodes;
        t_sb->tarfs_size = arg->mnt_sb.tarfs_size;
        t_sb->tarfs_dsize = arg->mnt_sb.tarfs_dsize;
        t_sb->tarfs_fsize = arg->mnt_sb.tarfs_fsize;
        t_sb->tarfs_flist_regdata = arg->mnt_sb.tarfs_flist_regdata;
        t_sb->tarfs_flist_dirdata = arg->mnt_sb.tarfs_flist_dirdata;
        t_sb->tarfs_flist_iexdata = arg->mnt_sb.tarfs_flist_iexdata;
        t_sb->tarfs_flist_indDinode = arg->mnt_sb.tarfs_flist_indDinode;
        t_sb->tarfs_flist_dinode = arg->mnt_sb.tarfs_flist_dinode;
        t_sb->tarfs_root = arg->mnt_sb.tarfs_root;
        t_sb->tarfs_free = arg->mnt_sb.tarfs_root;
        t_sb->tarfs_flags = arg->mnt_sb.tarfs_flags;
        t_sb->tarfs_maxino = arg->mnt_sb.tarfs_maxino;

	sb->s_fs_info = i_sb;
	set_blocksize(bdev, TAR_BLOCKSIZE);
	sb->s_blocksize_bits = TAR_BLOCKBITS;
	sb->s_blocksize = TAR_BLOCKSIZE;
	sb->s_op = &tarfs_sops;
	sb->s_time_gran = 1000;
	sb->s_flags = MS_RDONLY;
	sb->s_dirt = 0;

	// set free dinode 
	t_dp->di_magic = s_dp->di_magic;
	t_dp->di_mode = s_dp->di_mode;
	t_dp->di_uid = s_dp->di_uid;
	t_dp->di_gid = s_dp->di_gid;
	t_dp->di_nlink = s_dp->di_nlink;
	t_dp->di_size = s_dp->di_size;
	t_dp->di_blocks = s_dp->di_blocks;
	t_dp->di_atime = s_dp->di_atime;
	t_dp->di_mtime = s_dp->di_mtime;
	t_dp->di_ctime = s_dp->di_ctime;
	t_dp->di_header = TARBLK_NONE;
	t_dp->di_nExtents = s_dp->di_nExtents;
	for (i = 0; i < t_dp->di_nExtents; i++) {
		t_dp->di_directExtent[i].de_off = s_dp->di_directExtent[i].de_off;
		t_dp->di_directExtent[i].de_blkno = s_dp->di_directExtent[i].de_blkno;
		t_dp->di_directExtent[i].de_nblks = s_dp->di_directExtent[i].de_nblks;
	}
	t_dp->di_rootIndExt = s_dp->di_rootIndExt;
	t_dp->di_flags = s_dp->di_flags;

	// get root inode
	ip = tarfs_iget(sb, TARFS_ROOT_INO);
	if (!ip) {
		kfree(i_sb);
		printk("%s err can not allocate\n",__FUNCTION__);
		return -ENOMEM; 
	}
	sb->s_root = d_alloc_root(ip);
	return 0;
}

static int tarfs_get_sb(struct file_system_type *fs_type,
			int flags, const char *dev_name, void *data, struct vfsmount *mnt)
{
	return get_sb_bdev(fs_type, flags, dev_name, data, tarfs_fill_super, mnt);
}

static struct file_system_type tar_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "tarfs",
	.get_sb		= tarfs_get_sb,
	.kill_sb	= kill_block_super,
	.fs_flags	= FS_REQUIRES_DEV,
};

static int __init 
init_tarfs(void)
{
	int err;
	err = init_tarfs_inodecache();
	if (err != 0) {
		return err;
	}
	err = register_filesystem(&tar_fs_type);
	if (err != 0) {
		destroy_inodecache();
		return err;
	}
	return 0;
}

static void __exit
exit_tarfs(void)
{
	destroy_inodecache();
	unregister_filesystem(&tar_fs_type);
}


module_init(init_tarfs)
module_exit(exit_tarfs)

MODULE_LICENSE("GPL");
