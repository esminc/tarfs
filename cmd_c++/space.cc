#include "tarfs.h"

void Tarfs::SpaceManager::allocBlock(FsMaker *fs, uint64_t *blkno)
{
	tarfs_freeData *space = (tarfs_freeData*)::malloc(this->bulks*TAR_BLOCKSIZE);
	if (space == NULL) {
		fs->rollback();
	}
	if (this->blkno == TARBLK_NONE) {
		*blkno = fs->nblks(); /* first block is returned for use */
		this->blkno = fs->nblks() + 1; /* next block is first free */
		/* setup free blocks */
		uint64_t blk = *blkno;
		for (int i = 0; i < this->bulks; i++, blk++) {
			space[i].f_nextblkno = (blk+1);
			space[i].f_magic = TARFREE_MAGIC;
		}
		space[this->bulks-1].f_nextblkno = TARBLK_NONE;
		/* commit */
		fs->writeBlock((char*)space, *blkno, this->bulks);
	} else {
		*blkno = this->blkno; /* this block is returned for use */
		fs->readBlock((char*)space, this->blkno, 1);
		this->blkno = space->f_nextblkno;
	}
	::free(space);
	return;
}
