#include "tarfs.h"

using namespace Tarfs;

TARINO InodeFactory::inodeNum;
TARBLK InodeFactory::totalDataSize;
Inode* InodeFactory::freeInode;

Inode *InodeFactory::getInode(FsMaker *fs, TARINO ino, TARINO pino, int ftype)
{
	Inode *inode;
	TARBLK blkno;
	InodeFactory::freeInode->getBlock(fs, ino, &blkno);
	if (ftype == TARFS_IFDIR) {
		inode = new Dir(ino, pino, blkno, fs);
	} else {
		inode = new Inode(ino, pino, blkno, fs);
	}
	if (!inode) {
		fs->rollback();
	}
	return inode;
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
void InodeFactory::fin(FsMaker *fs)
{
	InodeFactory::freeInode->sync(fs);
	delete InodeFactory::freeInode;
	InodeFactory::freeInode = NULL;
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
		inode->setFtype(ftype);
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

void Inode::initialize(TARINO ino, TARINO pino, TARBLK blkno)
{
	this->ino = ino;
	this->pino = pino;
	this->blkno = blkno;
        memset(&this->dinode, 0, TAR_BLOCKSIZE);
        this->dinode.di_magic = TARDINODE_MAGIC;
        TARTIME now = Util::getCurrentTime();
	this->dinode.di_atime = now;
	this->dinode.di_ctime = now;
	this->dinode.di_mtime = now;
	this->dinode.di_header = TARBLK_NONE;
	this->dinode.di_uid = 0;
	this->dinode.di_gid = 0;
	this->dinode.di_mode = 0;
        this->dinode.di_nlink = 1;
}
Inode::Inode(TARINO ino, TARINO pino, TARBLK blkno)
{
	this->initialize(ino, pino, blkno);
}
Inode::Inode(TARINO ino, TARINO pino, TARBLK blkno, int ftype)
{
	this->initialize(ino, pino, blkno);
	this->ftype = ftype;
	this->dinode.di_mode = ftype | 0755;
}
Inode::Inode(TARINO ino, TARINO pino, TARBLK blkno, FsMaker *fs)
{
	this->initialize(ino, pino, blkno);
	fs->readBlock((char*)&this->dinode, this->blkno, 1);
	if ((this->dinode.di_mode & TARFS_IFMT) == TARFS_IFREG) {
		this->ftype = TARFS_IFREG;
	} else if ((this->dinode.di_mode & TARFS_IFMT) == TARFS_IFDIR) {
		this->ftype = TARFS_IFDIR;
	} else if ((this->dinode.di_mode & TARFS_IFMT) == TARFS_IFLNK) {
		this->ftype = TARFS_IFLNK;
	}
}
void Inode::setFile(File *file)
{
	this->setDirty();
	this->dinode.di_header = file->getBlknoHeader();
	this->dinode.di_mode = file->getFtype() | file->getMode();
	this->dinode.di_uid = file->getUid();
	this->dinode.di_gid = file->getGid();
	this->dinode.di_mtime = file->getMtime();
	this->dinode.di_mtime *=1000000;
	this->dinode.di_atime = this->dinode.di_ctime = this->dinode.di_mtime;
	this->dinode.di_size =file->getFileSize();
	this->dinode.di_blocks = file->getBlocks();
	this->dinode.di_nExtents = file->getNumExtents();;
	file->getExtent(&(this->dinode.di_directExtent[0]));
	return;
}
int Inode::getDirFtype()
{ 
	return TARFS_FTYPE_DINODE2DIR(this->dinode.di_mode);
}
void Inode::insBlock(FsMaker *fs, tarfs_dext *de)
{
	SpaceManager *iexmgr = fs->getIexManager();
	tarfs_indExt indExt;
	tarfs_indExt new_indExt;
	TARBLK ind_off = 0;
	TARBLK new_ind_off;
	tarfs_dext *iextp;
	int n;

	this->setDirty();
	if (this->dinode.di_nExtents < MAX_DINODE_DIRECT) {
		n = this->dinode.di_nExtents;
	} else {
		n = MAX_DINODE_DIRECT;
	}
	// search direct extents
	for (int i = 0; i < n; i++) {
		iextp = &this->dinode.di_directExtent[i];
		if ((iextp->de_blkno + iextp->de_nblks) == de->de_blkno) {
			goto done;
		}
	}
	if (this->dinode.di_nExtents == MAX_DINODE_DIRECT) {
		goto getblock;
	}
	if (this->dinode.di_nExtents < MAX_DINODE_DIRECT) {
		int n = this->dinode.di_nExtents;
		this->dinode.di_directExtent[n] = *de;
		this->dinode.di_nExtents++;
		return;
	}
	// search indirect extents
	ind_off = this->dinode.di_rootIndExt;
	while (1) {
		fs->readBlock((char*)&indExt, ind_off, 1);
		for (uint32_t i = 0; i < indExt.ie_num; i++) {
			iextp = &indExt.ie_dext[i];
			if ((iextp->de_blkno + iextp->de_nblks) == de->de_blkno) {
				goto done;
			}
		}
		if (indExt.ie_next == TARBLK_NONE) {
			break;
		}
		ind_off = indExt.ie_next;
	}
	if (indExt.ie_num < MAX_INDEXT) {
		int n = indExt.ie_num;
		indExt.ie_dext[n] = *de;
		indExt.ie_num++;
		fs->writeBlock((char*)&indExt, ind_off, 1);
		return;
	}
getblock:
	iexmgr->allocBlock(fs, &new_ind_off);
	fs->readBlock((char*)&new_indExt, new_ind_off, 1);
	new_indExt.ie_magic = TARINDE_MAGIC;
	new_indExt.ie_next = TARBLK_NONE;
	new_indExt.ie_num = 1;
	new_indExt.ie_dext[0] = *de;
	if (this->dinode.di_nExtents == MAX_DINODE_DIRECT) {
		this->dinode.di_rootIndExt = new_ind_off;
	} else {
		indExt.ie_next = new_ind_off;
		fs->writeBlock((char*)&indExt, ind_off, 1);
	}
	fs->writeBlock((char*)&new_indExt, new_ind_off, 1);
	this->dinode.di_nExtents++;
	return;
done:
	// merge!!
	iextp->de_nblks += de->de_nblks;
	if (ind_off) {
		fs->writeBlock((char*)&indExt, ind_off, 1);
	}
	return;
}
void Inode::sync(FsMaker *fs)
{
	if (!this->isDirty()) {
		return;
	}
	fs->writeBlock((char*)&this->dinode, this->blkno, 1);
	return;

}
void Inode::getBlock(FsMaker *fs, TARBLK off, TARBLK *blkno)
{
	TARBLK inx;
	tarfs_indExt_t indExt;
	TARBLK ind_off;
	tarfs_dext_t *iextp;
	int n;
	if (this->dinode.di_nExtents < MAX_DINODE_DIRECT) {
		n = this->dinode.di_nExtents;
	} else {
		n = MAX_DINODE_DIRECT;
	}
	// search direct extents
	for (int i = 0; i < n; i++) {
		iextp = &this->dinode.di_directExtent[i];
		if (iextp->de_off <= off &&
				     off < (iextp->de_off + iextp->de_nblks)) {
			goto done;
		}
	}
	if (this->dinode.di_nExtents <= MAX_DINODE_DIRECT) {
		return;
	}
	// search indirect extents
	ind_off = this->dinode.di_rootIndExt;
	while (1) {
		fs->readBlock((char*)&indExt, ind_off, 1);
		for (uint32_t i = 0; i < indExt.ie_num; i++) {
			iextp = &indExt.ie_dext[i];
			if (iextp->de_off <= off &&
					     off < (iextp->de_off + iextp->de_nblks)) {
				goto done;
			}
		}
		ind_off = indExt.ie_next;
		if (ind_off == TARBLK_NONE) {
			return;
		}
	}
done:
	inx = off - iextp->de_off;
	*blkno = iextp->de_blkno + inx;
	return;
}

