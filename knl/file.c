#include "tarfs_common_lnx.h"

struct file_operations tarfs_file_operations = {
	llseek:		generic_file_llseek,
	read:		do_sync_read,
	aio_read:	generic_file_aio_read,
	mmap:		generic_file_mmap,
	open:		generic_file_open,
};

