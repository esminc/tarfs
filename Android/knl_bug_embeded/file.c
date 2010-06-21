#include "tarfs_common_lnx.h"

static ssize_t tarfs_sync_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos);

struct file_operations tarfs_file_operations = {
	llseek:		generic_file_llseek,
	read:		tarfs_sync_read,
	aio_read:	generic_file_aio_read,
	mmap:		generic_file_mmap,
	open:		generic_file_open,
};

// for debug
static ssize_t tarfs_sync_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
	return do_sync_read(filp, buf, len, ppos);
}
