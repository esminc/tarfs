#ifndef _TARFS_COMMON_H_
#define _TARFS_COMMON_H_

#include <stdint.h>
#include "tarfs_meta.h"

#ifdef DEBUG
#define DBG(arg)	printf arg
#else
#define DBG(arg)
#endif
#define ASSERT(EXP)						\
	if (!(EXP))	{ printf("ASSERTION FAILED:%s:%d:%s\n", \
			__FUNCTION__, __LINE__, #EXP); exit(1);}
struct mkfs_manager;
struct incore_dinode {
	struct mkfs_manager 	*i_mp;
	struct posix_header 	*i_ph;
	tarfs_dext_t 		i_de;
	uint64_t		i_ino;
	uint64_t		i_pino;
	uint64_t		i_blkno;
	tarfs_dinode_t 		i_dinode;
};
typedef struct incore_dinode incore_dinode_t;

struct mkfs_manager {
	int 			m_fd;
	uint64_t		m_size;
	uint64_t		m_orgsize;
	tarfs_sblock_t		m_sb;
	incore_dinode_t		m_free;
	incore_dinode_t		m_root;
};
typedef struct mkfs_manager mkfs_mgr_t;

#define	ALLOC_REGDATA_NUM	128 // 通常ファイル
#define	ALLOC_DIRDATA_NUM	128 // ディレクトリ
#define	ALLOC_IEXDATA_NUM	128 // 間接エクステント(データ)
#define	ALLOC_INDDINODE_NUM	128 // 間接エクステント(dinode)
#define	ALLOC_DINODE_NUM	128 // dinode

// debug
extern void printDinode(uint64_t ino, tarfs_dinode_t *dp);
extern void printSblock(tarfs_sblock_t *sbp);
// dinode 
extern int allocDinode(mkfs_mgr_t *mp, incore_dinode_t *icp, int ftype);
extern int getDinode(mkfs_mgr_t *mp, incore_dinode_t *ip);
extern int allocFreeDinode(mkfs_mgr_t *mp, incore_dinode_t *icp);
extern int getDataBlock(mkfs_mgr_t *mp, tarfs_dinode_t *ip, uint64_t off, uint64_t *blkno);
// filesystem operation
extern int tarfs_readdir(mkfs_mgr_t *mp, tarfs_dinode_t *parentp);
extern int tarfs_create(mkfs_mgr_t *mp, incore_dinode_t *parentp,
		char *fname, incore_dinode_t *icp);
extern int tarfs_link(mkfs_mgr_t *mp, incore_dinode_t *parentp,
		char *fname, incore_dinode_t *icp);
// io
extern void do_rollback(mkfs_mgr_t *mp);
extern void writeData(mkfs_mgr_t *mp, char *p, uint64_t blkno, ssize_t nblks);
extern void readData(mkfs_mgr_t *mp, char *p, uint64_t blkno, ssize_t nblks);

// others
extern uint64_t getval8(char *p, size_t siz);
extern uint32_t getFtype(uint32_t typeflag);
extern uint64_t getCurrentTime(void);
#endif /* _TARFS_COMMON_H_ */
