#include "tarfs.h"

uint64_t Tarfs::Inode::inodeNum;
Tarfs::Inode* Tarfs::Inode::freeInode;

void Tarfs::Inode::incInodeNum()
{
	inodeNum++;
}
uint64_t Tarfs::Inode::getInodeNum()
{
	return inodeNum;
}
void Tarfs::Inode::setFreeInode(Inode *inode)
{
	freeInode = inode;
}
Tarfs::Inode *Tarfs::Inode::getFreeInode()
{
	return freeInode;
}
Tarfs::Inode *getInode(Tarfs::FsMaker *fs, uint64_t ino, uint64_t pino)
{
	uint64_t blkno;
	Tarfs::Inode::getFreeInode()->getBlock(fs, ino, &blkno);
	Tarfs::Inode *inode =  new Tarfs::Inode(ino, pino, blkno);
	if (inode == NULL) {
		return NULL;
	}
	inode->load(fs);
	return inode;
}
bool Tarfs::Inode::init(FsMaker *fs)
{
	tarfs_dext de;
	SpaceManager *sp = fs->getDinodeManager();

	uint64_t blkno;
	sp->allocBlock(fs, &blkno);
	de.de_off = Inode::getInodeNum();
	de.de_blkno = blkno;
	de.de_nblks = sp->getbulks();
	Inode *inode = new Inode(TARFS_FREE_INO, TARFS_FREE_INO, blkno);
	if (inode == NULL) {
		return false;
	}
	inode->insBlock(fs, &de);
	Inode::setFreeInode(inode);
	Inode::incInodeNum();
	return true;
}
Tarfs::Inode *Tarfs::Inode::allocInode(Tarfs::FsMaker *fs, uint64_t pino, int ftype)
{
	tarfs_dext de;
	uint64_t ino = Inode::getInodeNum();
	uint64_t blkno;
	uint64_t oldblkno;
	SpaceManager *sp = fs->getDinodeManager();

	oldblkno = sp->getblkno();
	sp->allocBlock(fs, &blkno);
	if (oldblkno == TARBLK_NONE) {
		de.de_off = ino;
		de.de_blkno = blkno;
		de.de_nblks = sp->getbulks();
		Inode::freeInode->insBlock(fs, &de);
	}
	Inode *inode;
	if (ftype == TARFS_IFDIR) {
		inode = new Dir(ino, pino, blkno);
	} else {
		inode = new Inode(ino, pino, blkno);
	}
	if (inode == NULL) {
		return NULL;
	}
	inode->load(fs);
	inode->setFtype(ftype);
	Inode::incInodeNum();
	return inode;
}

Tarfs::Inode::Inode(uint64_t ino, uint64_t pino, uint64_t blkno)
 {
	this->isLoad = false;
	this->ino = ino;
	this->pino = pino;
	this->blkno = blkno;
        memset(&this->dinode, 0, TAR_BLOCKSIZE);
        this->dinode.di_magic = TARDINODE_MAGIC;
        uint64_t now = File::getCurrentTime();
	this->dinode.di_atime = now;
	this->dinode.di_ctime = now;
	this->dinode.di_mtime = now;
	this->dinode.di_header = TARBLK_NONE;
	this->dinode.di_uid = 0;
	this->dinode.di_gid = 0;
	this->dinode.di_mode = 0;
        this->dinode.di_nlink = 1;
}
void Tarfs::Inode::setFile(File *file)
{
	this->setDirty();
	int ftype = file->getFtype();
	this->dinode.di_mode = ftype;
	this->dinode.di_mode |= File::strtoint(file->header.mode, sizeof(file->header.mode));
	this->dinode.di_uid = File::strtoint(file->header.uid, sizeof(file->header.uid));
	this->dinode.di_gid = File::strtoint(file->header.gid, sizeof(file->header.gid));
	this->dinode.di_mtime = File::strtoint(file->header.mtime, sizeof(file->header.mtime));
	this->dinode.di_mtime *=1000000;
	this->dinode.di_atime = this->dinode.di_ctime = this->dinode.di_mtime;
	if (ftype == TARFS_IFREG) {
		this->dinode.di_size =File::strtoint(file->header.size, sizeof(file->header.size));
		this->dinode.di_blocks = TARFS_BLOCKS(this->dinode.di_size);
		if (this->dinode.di_blocks > 0) {
			this->dinode.di_nExtents = 1;
			this->dinode.di_directExtent[0] = file->addrExtent;
		}
	} else if (ftype == TARFS_IFLNK) {
		this->dinode.di_size = strlen(file->header.linkname);
		this->dinode.di_blocks = 0;
	}
	return;
}
void Tarfs::Inode::load(FsMaker *fs)
{
	fs->readBlock((char*)&this->dinode, this->blkno, 1);
	this->isLoad = true;
	if ((this->dinode.di_mode & TARFS_IFMT) == TARFS_IFREG) {
		this->ftype = TARFS_IFREG;
	} else if ((this->dinode.di_mode & TARFS_IFMT) == TARFS_IFDIR) {
		this->ftype = TARFS_IFDIR;
	} else if ((this->dinode.di_mode & TARFS_IFMT) == TARFS_IFLNK) {
		this->ftype = TARFS_IFLNK;
	}
	return;
}
void Tarfs::Inode::insBlock(FsMaker *fs, tarfs_dext *de)
{
	SpaceManager *iexmgr = fs->getIexManager();
	tarfs_indExt indExt;
	tarfs_indExt new_indExt;
	uint64_t ind_off = 0;
	uint64_t new_ind_off;
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
void Tarfs::Inode::sync(FsMaker *fs)
{
	if (!this->isDirty()) {
		return;
	}
	fs->writeBlock((char*)&this->dinode, this->blkno, 1);
	return;

}
void Tarfs::Inode::getBlock(FsMaker *fs, uint64_t off, uint64_t *blkno)
{
	uint64_t inx;
	tarfs_indExt_t indExt;
	uint64_t ind_off;
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

