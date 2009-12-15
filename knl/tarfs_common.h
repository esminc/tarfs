#ifndef _TARFS_COMMON_H_
#define _TARFS_COMMON_H_

#include "tarfs_osdep.h"
#include "../include/tarfs_meta.h"

//#define DEBUG
#ifdef DEBUG
#define DBG(arg)	printk arg
#else
#define DBG(arg)
#endif
#define ASSERT(EXP)						\
	if (!(EXP))	{ printk("ASSERTION FAILED:%s:%d:%s\n", \
			__FUNCTION__, __LINE__, #EXP); }
/*
 * I/O 要求用のデータ
 * (要求できるI/O 個数は１個だけで、サイズは 512byte)
 */
struct tarfs_io;
struct tarfs_io_operation;
struct tarfs_io {
	uint64_t io_blkno; 	/* IN:512 byte 単位 */
	void*	 io_data;	/* OUT: I/O 完了したデータ */
	void*	 io_bh;		/* OS 依存データ(linux の場合は buffer_head) */
	void*	 io_dev;	/* OS 依存データ(linux の場合は super_block) */
	const struct tarfs_io_operation* oper;
};
typedef struct tarfs_io tarfs_io_t;

struct tarfs_io_operation {
	int32_t	 (*init) (void* , tarfs_io_t *);
	int32_t	 (*bget) (struct tarfs_io *, uint64_t); /* OS 依存関数(getblk)に相当する */
	int32_t	 (*bread) (struct tarfs_io *, uint64_t); /* OS 依存関数(sb_bread)に相当する */
	int32_t	 (*bput) (struct tarfs_io *); /* OS 依存関数(brelse)に相当する */
};
typedef struct tarfs_io_operation tarfs_io_operation_t;
extern const tarfs_io_operation_t tarfs_io_oper;
extern int tarfs_getDataBlock(tarfs_io_t *iop, tarfs_dinode_t *ip, uint64_t off, uint64_t *blkno);


#endif /* _TARFS_COMMON_H_ */
