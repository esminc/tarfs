#include "tarfs.h"

void Tarfs::Dir::init(FsMaker *fs)
{
	tarfs_dext de;
	uint64_t blkno;
	char buf[TAR_BLOCKSIZE];
	SpaceManager *sp = fs->getDirManager();

	// get new entry
	sp->allocBlock(fs, &blkno);
	de.de_off = this->dinode.di_blocks;
	de.de_blkno = blkno;
	de.de_nblks = 1;
	this->insBlock(fs, &de);

	int dot_len;
	tarfs_direct *direct = (tarfs_direct*)buf;
	direct->d_ino = this->ino;
	direct->d_name[0] = '.';
	direct->d_namelen = 1;
	direct->d_ftype = TARFS_FTYPE_DIR;
	dot_len = direct->d_reclen = DIRENT_SIZE(1);
	direct = (tarfs_direct *)((char*)direct + direct->d_reclen);
	direct->d_ino = this->pino;
	direct->d_name[0] = '.';
	direct->d_name[1] = '.';
	direct->d_namelen = 2;
	direct->d_ftype = TARFS_FTYPE_DIR;
	direct->d_reclen = TAR_BLOCKSIZE - dot_len;

	fs->writeBlock(buf, blkno, 1);
	this->dinode.di_nlink++;
	this->dinode.di_blocks=1;
	this->dinode.di_size = TAR_BLOCKSIZE;
	this->setDirty();
	return;
}
Tarfs::Inode* Tarfs::Dir::lookup(FsMaker *fs, char *name)
{
	uint64_t off = 0;
	uint64_t endoff = this->dinode.di_size;
	uint64_t dirblkno;
	char 	 dirbuf[TAR_BLOCKSIZE];
	tarfs_direct *direct;
	int len = ::strlen(name);

	while (off < endoff) {
		if ((off & (TAR_BLOCKSIZE -1)) == 0) { // 512 align
			this->getBlock(fs, off/TAR_BLOCKSIZE, &dirblkno);
			fs->readBlock(dirbuf, dirblkno, 1);
			direct = (tarfs_direct*)dirbuf;
		}
		/* exist entry check */
		if (direct->d_ino != TARBLK_NONE && direct->d_namelen == len &&
			!strncmp(direct->d_name, name, len)) {
			goto done;
		}
		off += direct->d_reclen;
		direct = (tarfs_direct_t*)(((char*)direct) + direct->d_reclen);
	}
	return NULL;
done:
	Inode *inode;
	int ftype = TARFS_FTYPE_DIR2DINODE(direct->d_ftype);
	if (ftype == TARFS_IFDIR) {
		inode = new Dir(this->ino, direct->d_ino, ftype);
	} else {
		inode = new Inode(this->ino, direct->d_ino, ftype);
	}
	if (inode) {
		inode->load(fs);
	}
	return inode;
}
void Tarfs::Dir::searchFreeSpace(FsMaker *fs, char *name, dirFreeSpace *dfs)
{
	uint64_t off = 0;
	uint64_t endoff = this->dinode.di_size;
	uint64_t dirblkno;
	char 	 dirbuf[TAR_BLOCKSIZE];
	tarfs_direct *direct;
	int len = ::strlen(name);
	int needspace = DIRENT_SIZE(len);
	int freesize;

	while (off < endoff) {
		if ((off & (TAR_BLOCKSIZE -1)) == 0) { // 512 align
			this->getBlock(fs, off/TAR_BLOCKSIZE, &dirblkno);
			fs->readBlock(dirbuf, dirblkno, 1);
			direct = (tarfs_direct*)dirbuf;
		}
		/* free space check */
		freesize = direct->d_reclen;
		if (direct->d_ino != TARBLK_NONE) {
			freesize -= (int)DIRECT_SIZE(direct);
		}
		if (freesize > 0 && freesize >= needspace) {
			dfs->off = off;
			dfs->blkno = dirblkno;
			::memcpy(dfs->dirbuf, dirbuf, TAR_BLOCKSIZE);
			return;
		}
		off += direct->d_reclen;
		direct = (tarfs_direct_t*)(((char*)direct) + direct->d_reclen);
	}
	return;
}
void Tarfs::Dir::addDirEntry(FsMaker *fs, char *name, dirFreeSpace *dfs, Inode *inode)
{
	tarfs_direct *direct;
	tarfs_direct *ep;
	int len = strlen(name);

	if (inode->isDir()) {
		this->dinode.di_nlink++;
	}
	this->setDirty();
	// existing entry
	if (dfs->off != TARBLK_NONE) {
		int inx = (int)(dfs->off & (uint64_t)(TAR_BLOCKSIZE -1));
		direct = (tarfs_direct_t *)&dfs->dirbuf[inx];
		if (direct->d_ino == TARBLK_NONE) {
			direct->d_ino = inode->ino;
			direct->d_ftype = TARFS_FTYPE_DINODE2DIR(inode->dinode.di_mode);
			direct->d_namelen = len;
			memcpy(direct->d_name, name, len);
		} else {
			int org_reclen;
			org_reclen = direct->d_reclen;
			direct->d_reclen = DIRENT_SIZE(direct->d_namelen);
			ep = (tarfs_direct *)((char*)direct + direct->d_reclen);
			ep->d_ino = this->ino;
			ep->d_ftype = TARFS_FTYPE_DINODE2DIR(this->dinode.di_mode);
			ep->d_namelen = len;
			ep->d_reclen = org_reclen - DIRENT_SIZE(direct->d_namelen);
			memcpy(ep->d_name, name, len);
		}
		fs->writeBlock((char*)dfs->dirbuf, dfs->blkno, 1);
		return;
	}
	// get new entry
	SpaceManager *sp = fs->getDirManager();
	sp->allocBlock(fs, &dfs->blkno);
	fs->readBlock((char*)&dfs->dirbuf, dfs->blkno, 1);
	tarfs_dext de;
	de.de_off = this->dinode.di_blocks;
	de.de_blkno = dfs->blkno;
	de.de_nblks = 1;
	this->insBlock(fs, &de);
	ep = (tarfs_direct *)dfs->dirbuf;
	ep->d_reclen = TAR_BLOCKSIZE;
	ep->d_ino = inode->ino;
	ep->d_ftype = TARFS_FTYPE_DINODE2DIR(inode->dinode.di_mode);
	ep->d_namelen = len;
	memcpy(ep->d_name, name, len);

	this->dinode.di_blocks++;
	this->dinode.di_size += TAR_BLOCKSIZE;
	fs->writeBlock((char*)dfs->dirbuf, dfs->blkno, 1);
	return;
}

Tarfs::Dir* Tarfs::Dir::mkdir(FsMaker *fs, char *name)
{
	Dir *dir = static_cast<Dir*>(Inode::allocInode(fs, this->ino, TARFS_IFDIR));
	if (dir == NULL) {
		return NULL;
	}
	dir->init(fs);
	dirFreeSpace dfs;
	dfs.off = TARBLK_NONE;
	this->searchFreeSpace(fs, name, &dfs);
	this->addDirEntry(fs, name, &dfs, dir);
	dir->sync(fs);
	return dir;
}
Tarfs::Inode* Tarfs::Dir::create(FsMaker *fs, File *file)
{
	char *name = (*file)[file->entSize()-1];
	Inode *inode = Inode::allocInode(fs, this->ino, file->getFtype());
	if (inode == NULL) {
		return NULL;
	}
	if (inode->isDir()) {
		Dir *dir = static_cast<Dir*>(inode);
		dir->init(fs);
	}
	dirFreeSpace dfs;
	dfs.off = TARBLK_NONE;
	this->searchFreeSpace(fs, name, &dfs);
	this->addDirEntry(fs, name, &dfs, inode);
	inode->sync(fs);
	return inode;
}
