#include "tarfs.h"

using namespace Tarfs;

TARINO InodeFactory::inodeNum;
TARBLK InodeFactory::totalDataSize;
Inode* InodeFactory::freeInode;

Inode *InodeFactory::getInode(FsIO *fsio, TARINO ino, TARINO pino)
{
	Inode *inode;
	TARBLK blkno;
	InodeFactory::freeInode->getBlock(fsio, ino, &blkno);
	inode = new Inode(ino, pino, blkno, fsio);
	if (inode && inode->isDir()) {
		delete inode;
		inode = new Dir(ino, pino, blkno, fsio);
	}
	return inode;
}
Inode *InodeFactory::getInode(FsIO *fsio, TARINO ino, TARINO pino, int ftype)
{
	Inode *inode;
	TARBLK blkno;
	InodeFactory::freeInode->getBlock(fsio, ino, &blkno);
	if (ftype == TARFS_IFDIR) {
		inode = new Dir(ino, pino, blkno, fsio);
	} else {
		inode = new Inode(ino, pino, blkno, fsio);
	}
	if (!inode) {
		fsio->rollback();
	}
	return inode;
}
Inode *InodeFactory::allocInode(FsMaker *fs, TARINO pino, int ftype)
{
	Inode *inode;
	TARINO ino = InodeFactory::inodeNum;
	TARBLK blkno;
	TARBLK oldblkno;
	SpaceManager *sp = fs->getDinodeManager();

	oldblkno = sp->getblkno();
	sp->allocBlock(fs, &blkno);
	if (oldblkno == TARBLK_NONE) {
		tarfs_dext de;
		de.de_off = ino;
		de.de_blkno = blkno;
		de.de_nblks = sp->getbulks();
		InodeFactory::freeInode->insBlock(fs, &de);
	}
	if (ftype == TARFS_IFDIR) {
		Dir *dir = new Dir(ino, pino, blkno);
		dir->dirInit(fs);
		inode = dir;
	} else {
		inode = new Inode(ino, pino, blkno, ftype);
	}
	if (inode) {
		inode->setDirty();
		InodeFactory::inodeNum++;
	} else {
		fs->rollback();
	}
	return inode;
}
Inode *InodeFactory::allocInode(FsMaker *fs, TARINO pino, File *file)
{
	Inode *inode = InodeFactory::allocInode(fs, pino, file->getFtype());
	if (inode) {
		inode->setFile(file);
		if (file->getFtype() == TARFS_IFREG) {
			InodeFactory::totalDataSize += file->getBlocks();
		}
	}
	return inode;
}

bool InodeFactory::load(FsIO *fsio, TARBLK freeInodeBlkno)
{
	Inode *inode = new Inode(TARFS_FREE_INO, TARFS_FREE_INO, freeInodeBlkno, fsio);
	if (!inode) {
		return false;
	}
	InodeFactory::freeInode = inode;
	return true;
}

bool InodeFactory::init(FsMaker *fs)
{
	tarfs_dext de;
	SpaceManager *sp = fs->getDinodeManager();

	TARBLK blkno;
	sp->allocBlock(fs, &blkno);
	de.de_off = InodeFactory::inodeNum = 0;
	de.de_blkno = blkno;
	de.de_nblks = sp->getbulks();
	Inode *inode = new Inode(TARFS_FREE_INO, TARFS_FREE_INO, blkno);
	if (inode == NULL) {
		return false;
	}
	inode->insBlock(fs, &de);
	InodeFactory::freeInode = inode;
	InodeFactory::inodeNum++;
	InodeFactory::totalDataSize = 0;
	return true;
}
void InodeFactory::fin(FsIO *fsio)
{
	InodeFactory::freeInode->sync(fsio);
	delete InodeFactory::freeInode;
	InodeFactory::freeInode = NULL;
}

