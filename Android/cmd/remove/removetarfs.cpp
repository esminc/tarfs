#include "tarfs.h"

using namespace Tarfs;

int
main(int argc, const char* argv[])
{
	int err;
	char *tarfile;
	FsReader *fsreader;
	tarfs_sblock *sbp;
	TARBLK orgblk;

	if (argc != 2) {
		fprintf(stderr, "Usage:%s tarfile\n", argv[0]);
		return 1;
	}
	tarfile = (char*)argv[1];

	fsreader = FsReader::create(tarfile, FsReader::TARFILE);
	if (!fsreader) {
		return 1;
	}
	sbp = fsreader->getSblock();
	orgblk = fsreader->getFileSize() - sbp->tarfs_size;
	::free(sbp);
	delete fsreader;

    int fd = ::open(tarfile, O_RDWR);
    if (fd >= 0) {
    	err = ::ftruncate(fd, orgblk*TAR_BLOCKSIZE);
        if (err < 0) {
            err = errno;
        }
        ::close(fd);
    } else {
        err = errno;
    }
	if (err != 0) {
		fprintf(stderr, "truncate(2) err=%d\n", errno);
		return 1;
	}
	return 0;
}

