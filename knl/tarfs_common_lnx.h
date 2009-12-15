#ifndef _TARFS_COMMON_LNX_H_
#define _TARFS_COMMON_LNX_H_

#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/mpage.h>
#include <linux/writeback.h>
#include <linux/highmem.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/blkdev.h>
#include <linux/parser.h>
#include <linux/random.h>
#include <linux/buffer_head.h>
#include <linux/smp_lock.h>
#include <linux/vfs.h>
#include <linux/time.h>
#include <linux/namei.h>
#include <linux/delay.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/wait.h>
#include <net/sock.h>
#include <linux/tcp.h>
#include <linux/syscalls.h>
#include <linux/proc_fs.h>
#include <linux/pagevec.h>

#include "tarfs_common.h"

struct tarfs_incore_sblock {
	struct tarfs_sblock sb;
	tarfs_dinode_t 	    free_dinode;
};
extern struct kmem_cache *tarfs_inode_cachep;
extern void init_tarfs_io(void* dev, uint64_t blkno, tarfs_io_t * iop);
extern struct inode_operations tarfs_dir_inode_operations;
extern struct file_operations tarfs_dir_operations;
extern struct inode_operations tarfs_file_inode_operations;
extern struct file_operations tarfs_file_operations;
extern struct inode *tarfs_iget(struct super_block *sb, uint64_t ino);


#endif /* _TARFS_COMMON_LNX_H_ */
