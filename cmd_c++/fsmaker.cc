#include "tarfs.h"

Tarfs::FsMaker::FsMaker(char *fname)
{
	::memset(this->fname, 0, 4096);
	::memcpy(this->fname, fname, strlen(fname));
	this->fd = -1;
}
int Tarfs::FsMaker::init()
{
	int err;
	struct stat buf_stat;

	this->dirmgr = NULL;
	this->iexmgr = NULL;
	this->dinodemgr = NULL;
	this->fd = ::open(this->fname, O_RDWR);
	if (this->fd < 0) {
		return -1;
	}
	err = ::fstat(fd, &buf_stat);
	if (err < 0) {
		::fprintf(stderr, "init_mkfs:fstat err=%d\n", errno);
		return -1;
	}
	this->blks = buf_stat.st_size / TAR_BLOCKSIZE;
	this->orgblks = buf_stat.st_size / TAR_BLOCKSIZE;

	ssize_t res;
        int64_t off = (buf_stat.st_size / TAR_BLOCKSIZE) - 1;
	res = pread(this->fd, &this->sb, TAR_BLOCKSIZE, off*TAR_BLOCKSIZE);
	if (res != TAR_BLOCKSIZE){
		::fprintf(stderr, "init_mkfs:pread err=%d\n", err);
		return -1;
	}
	if (sb.tarfs_magic == TARSBLOCK_MAGIC) {
		if (sb.tarfs_flags == TARSBLOCK_FLAG_OK) {
			return EEXIST;
		} else {
			return -1;
		}
	}
	// super block
	this->sb.tarfs_magic = TARSBLOCK_MAGIC;
	this->sb.tarfs_size = 0;
	this->sb.tarfs_dsize = 0;
	this->sb.tarfs_flist_regdata = TARBLK_NONE;
	this->sb.tarfs_flist_dirdata = TARBLK_NONE;
	this->sb.tarfs_flist_iexdata = TARBLK_NONE;
	this->sb.tarfs_flist_indDinode = TARBLK_NONE;
	this->sb.tarfs_flist_dinode = TARBLK_NONE;
	this->sb.tarfs_free = (buf_stat.st_size/TAR_BLOCKSIZE);
	this->sb.tarfs_root = (buf_stat.st_size/TAR_BLOCKSIZE)+1;
	this->sb.tarfs_flags = TARSBLOCK_FLAG_OK;

	// init space managers
	this->dirmgr = new SpaceManager(ALLOC_DIRDATA_NUM);
	this->iexmgr = new SpaceManager(ALLOC_IEXDATA_NUM);
	this->dinodemgr = new SpaceManager(ALLOC_DINODE_NUM);
	if (!this->dirmgr || !this->iexmgr || !this->dinodemgr) {
		return false;
	}
	// free dinode
	bool ret = Inode::init(this);
	if (ret == false) {
		return false;
	}

	// root dinode
	Inode *inode = Inode::allocInode(this, TARFS_ROOT_INO, TARFS_IFDIR);
	if (inode == NULL) {
		return false;
	}
	this->root = static_cast<Dir*>(inode);
	this->root->init(this);
	return true;
}

Tarfs::FsMaker::~FsMaker()
{
	if (this->fd >= 0) {
		::close(this->fd);
		this->fd = -1;
	}
	if (this->dirmgr) {
		delete this->dirmgr;
	}
	if (this->iexmgr) {
		delete this->iexmgr;
	}
	if (this->dinodemgr) {
		delete this->dinodemgr;
	}
}

void Tarfs::FsMaker::add(File *file)
{
	int i;
	Dir *dir;
	Dir *parent = this->root;
	Inode *found;

	for (i = 0; i < file->entSize() - 1; i++, parent = dir) {
		dir = NULL;
		char *name = (*file)[i]; /* name in path */
		found = parent->lookup(this, name);
		if (!found) { 
			/* create directory */
			dir = parent->mkdir(this, name);
		} else if (found->isDir()){ 
			dir = static_cast<Dir*>(found);
		}
		if (dir == NULL) {
			this->rollback();
		}
		if (dir != this->root) {
			delete parent;
		}
	}
	Inode *inode = NULL;
	/* last name */
	found = parent->lookup(this, (*file)[file->entSize()-1]);
	if (!found) {
		inode = parent->create(this, file);
	} else {
		inode = found;
		inode->setFile(file);
	}
	if (!inode) {
		this->rollback();
	}
	inode->sync(this);
	delete inode;
	return;
}
void Tarfs::FsMaker::rollback()
{
	(void)::ftruncate(this->fd, this->orgblks*TAR_BLOCKSIZE);
	::exit(1);
	return;
}
void Tarfs::FsMaker::readBlock(char *bufp, uint64_t blkno, ssize_t nblks)
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
void Tarfs::FsMaker::writeBlock(char *bufp, uint64_t blkno, ssize_t nblks)
{
	int err = 0;
	ssize_t res;
	res = ::pwrite(this->fd, bufp, TAR_BLOCKSIZE*nblks, 
				blkno*(uint64_t)TAR_BLOCKSIZE);
	if (res != (TAR_BLOCKSIZE*nblks)) {
		err = errno;
		::fprintf(stderr, "readData:blkno=%lld err = %d\n", blkno, err);
		this->rollback();
	}
	if ((blkno + nblks) > this->blks) {
		this->blks = blkno + nblks;
		this->sb.tarfs_size = (this->blks - this->orgblks);
	}
	return;
}

