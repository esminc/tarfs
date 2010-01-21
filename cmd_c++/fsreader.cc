#include "tarfs.h"
using namespace Tarfs;

FsReader::~FsReader()
{
	InodeFactory::fin(this);
	if (this->fd >= 0) {
		::close(this->fd);
		this->fd = -1;
	}
	if (this->root) {
		delete this->root;
		this->root = NULL;
	}
}
FsReader *FsReader::create(char *fname, TarfileType type)
{
	int err;
	struct stat buf_stat;

	FsReader *fsreader = new FsReader(fname);
	if (!fsreader) {
		::fprintf(stderr, "can not new FsReader Instance.\n");
		return NULL;
	}
	fsreader->fd = ::open(fsreader->fname, O_RDONLY|O_LARGEFILE);
	if (fsreader->fd < 0) {
		::fprintf(stderr, "open(2) %s err=%d\n", fsreader->fname, errno);
		delete fsreader;
		return NULL;
	}
	if (type == TARFILE) {
		err = ::fstat(fsreader->fd, &buf_stat);
		fsreader->blks = buf_stat.st_size / TAR_BLOCKSIZE;
	} else { /* DEVFILE */
		TARBLK blksz;
		err = ::ioctl(fsreader->fd, BLKGETSIZE64, &blksz);
		fsreader->blks = blksz / TAR_BLOCKSIZE;
	}
	if (err < 0) {
		::fprintf(stderr, "can not get filesize of %s err=%d\n", fsreader->fname, errno);
		delete fsreader;
		return NULL;
	}

	fsreader->readBlock((char*)&fsreader->sb, (fsreader->blks - 1), 1);
	if (fsreader->sb.tarfs_magic != TARSBLOCK_MAGIC) {
		/* not completed mkfs_tar */
		::fprintf(stderr, "please do mkfs.tarfs\n");
		delete fsreader;
		return NULL;
	}
	bool ret = InodeFactory::load(fsreader, fsreader->sb.tarfs_free);
	if (ret == false) {
		delete fsreader;
		return NULL;
	}
	fsreader->root = static_cast<Dir*>(InodeFactory::getInode(fsreader, TARFS_ROOT_INO, TARFS_ROOT_INO, TARFS_IFDIR));
	if (!fsreader->root) {
		delete fsreader;
		return NULL;
	}
	return fsreader;
}

tarfs_dinode *FsReader::getFreeDinode()
{
	Inode *freeInode = InodeFactory::getFreeInode();
	return freeInode->getDinodeImage();
}
void FsReader::writeBlock(char* bufp, TARBLK blkno, ssize_t nblks)
{
	return;
}

