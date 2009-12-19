#include "tarfs.h"
using namespace Tarfs;

FsMaker::FsMaker(char *fname)
{
	::memset(this->fname, 0, 4096);
	::memcpy(this->fname, fname, strlen(fname));
	this->fd = -1;
}
FsMaker *FsMaker::create(char *fname)
{
	int err;
	struct stat buf_stat;

	FsMaker *fsmaker = new FsMaker(fname);
	if (!fsmaker) {
		return NULL;
	}
	fsmaker->dirmgr = NULL;
	fsmaker->iexmgr = NULL;
	fsmaker->dinodemgr = NULL;
	fsmaker->fd = ::open(fsmaker->fname, O_RDWR);
	if (fsmaker->fd < 0) {
		delete fsmaker;
		return NULL;
	}
	err = ::fstat(fsmaker->fd, &buf_stat);
	if (err < 0) {
		::fprintf(stderr, "init_mkfs:fstat err=%d\n", errno);
		delete fsmaker;
		return NULL;
	}
	fsmaker->blks = buf_stat.st_size / TAR_BLOCKSIZE;
	fsmaker->orgblks = buf_stat.st_size / TAR_BLOCKSIZE;

	ssize_t res;
        int64_t off = (buf_stat.st_size / TAR_BLOCKSIZE) - 1;
	res = pread(fsmaker->fd, &fsmaker->sb, TAR_BLOCKSIZE, off*TAR_BLOCKSIZE);
	if (res != TAR_BLOCKSIZE){
		::fprintf(stderr, "init_mkfs:pread err=%d\n", err);
		delete fsmaker;
		return NULL;
	}
	if (fsmaker->sb.tarfs_magic == TARSBLOCK_MAGIC) {
		delete fsmaker;
		return NULL;
	}
	// super block
	fsmaker->sb.tarfs_magic = TARSBLOCK_MAGIC;
	fsmaker->sb.tarfs_size = 0;
	fsmaker->sb.tarfs_dsize = 0;
	fsmaker->sb.tarfs_flist_regdata = TARBLK_NONE;
	fsmaker->sb.tarfs_flist_dirdata = TARBLK_NONE;
	fsmaker->sb.tarfs_flist_iexdata = TARBLK_NONE;
	fsmaker->sb.tarfs_flist_indDinode = TARBLK_NONE;
	fsmaker->sb.tarfs_flist_dinode = TARBLK_NONE;
	fsmaker->sb.tarfs_free = (buf_stat.st_size/TAR_BLOCKSIZE);
	fsmaker->sb.tarfs_root = (buf_stat.st_size/TAR_BLOCKSIZE)+1;
	fsmaker->sb.tarfs_flags = TARSBLOCK_FLAG_OK;

	// init space managers
	fsmaker->dirmgr = new SpaceManager(ALLOC_DIRDATA_NUM);
	fsmaker->iexmgr = new SpaceManager(ALLOC_IEXDATA_NUM);
	fsmaker->dinodemgr = new SpaceManager(ALLOC_DINODE_NUM);
	if (!fsmaker->dirmgr || !fsmaker->iexmgr || !fsmaker->dinodemgr) {
		delete fsmaker;
		return NULL;
	}
	// free dinode
	bool ret = InodeFactory::init(fsmaker);
	if (ret == false) {
		delete fsmaker;
		return NULL;
	}

	// root dinode
	Inode *inode = InodeFactory::allocInode(fsmaker, TARFS_ROOT_INO, TARFS_IFDIR);
	if (inode == NULL) {
		delete fsmaker;
		return false;
	}
	fsmaker->root = static_cast<Dir*>(inode);
	fsmaker->root->init(fsmaker);
	return fsmaker;
}

FsMaker::~FsMaker()
{
	if (this->fd >= 0) {
		::close(this->fd);
		this->fd = -1;
	}
	if (this->dirmgr) {
		delete this->dirmgr;
		this->dirmgr = NULL;
	}
	if (this->iexmgr) {
		delete this->iexmgr;
		this->iexmgr = NULL;
	}
	if (this->dinodemgr) {
		delete this->dinodemgr;
		this->dinodemgr = NULL;
	}
}

void FsMaker::add(File *file)
{
	int i;
	Dir *dir;
	Dir *nextDir;
	Inode *inode;

	for (dir = this->root, i = 0; i < file->entSize(); i++, dir = nextDir) {
		char *name = (*file)[i]; /* name in path */
		inode = dir->lookup(this, name);
		if (i < (file->entSize() - 1)) { /* create directory */
			if (!inode) {
				nextDir = dir->mkdir(this, name);
			} else if (inode->isDir()) { 
				nextDir = static_cast<Dir*>(inode);
			} else { /* assert check */
				this->rollback();
			}
			nextDir->sync(this);
		} else { /* create last file */
			if (!inode) {
				inode = dir->create(this, file);
			} else {
				inode->setFile(file);
			}
			inode->sync(this);
		}
		if (dir != this->root) {
			dir->sync(this);
			delete dir;
		}
	}
	delete inode;
	return;
}
void FsMaker::complete()
{
	this->sb.tarfs_dsize = InodeFactory::getTotalDataSize();
	this->sb.tarfs_flist_dirdata = this->dirmgr->getblkno();
	this->sb.tarfs_flist_iexdata = this->iexmgr->getblkno();
	this->sb.tarfs_flist_dinode = this->dinodemgr->getblkno();
	this->sb.tarfs_inodes = InodeFactory::getInodeNum();
	this->sb.tarfs_maxino = InodeFactory::getInodeNum();
	this->root->sync(this);
	InodeFactory::fin(this);
	this->writeBlock((char*)&(this->sb), this->blks, 1);
}
void FsMaker::rollback()
{
	(void)::ftruncate(this->fd, this->orgblks*TAR_BLOCKSIZE);
	::exit(1);
	return;
}
void FsMaker::readBlock(char *bufp, uint64_t blkno, ssize_t nblks)
{
	int err = 0;
	ssize_t res;
	res = ::pread(this->fd, bufp, TAR_BLOCKSIZE*nblks, 
				blkno*(uint64_t)TAR_BLOCKSIZE);
	if (res != (TAR_BLOCKSIZE*nblks)) {
		err = errno;
		::fprintf(stderr, "readData:blkno=%lld err = %d\n", blkno, err);
		this->rollback();
	}
	return;
}
void FsMaker::writeBlock(char *bufp, uint64_t blkno, ssize_t nblks)
{
	int err = 0;
	ssize_t res;
	if ((blkno + nblks) > this->blks) {
		this->blks = blkno + nblks;
		this->sb.tarfs_size = (this->blks - this->orgblks);
	}
	res = ::pwrite(this->fd, bufp, TAR_BLOCKSIZE*nblks, 
				blkno*(uint64_t)TAR_BLOCKSIZE);
	if (res != (TAR_BLOCKSIZE*nblks)) {
		err = errno;
		::fprintf(stderr, "readData:blkno=%lld err = %d\n", blkno, err);
		this->rollback();
	}
	return;
}

