#include "tarfs.h"

using namespace Tarfs;

int
main(int argc, const char* argv[])
{
	int err;
	tarfs_mnt_arg arg;
	char *devname;
	char *tarfile;
	char *mntpoint;
	FsReader *fsreader;
	tarfs_sblock *sbp;
	tarfs_dinode *freeDinodep;

	if (argc != 4) {
		fprintf(stderr, "Usage:%s devname tarfile mntpoint\n", argv[0]);
		return 1;
	}

	devname = (char*)argv[1];
	tarfile = (char*)argv[2];
	mntpoint = (char*)argv[3];

	fsreader = FsReader::create(tarfile, FsReader::TARFILE);
	if (!fsreader) {
		return 1;
	}
	sbp = fsreader->getSblock();
	freeDinodep = fsreader->getFreeDinode();
	arg.mnt_sb = *sbp;
	arg.mnt_dinode = *freeDinodep;
	::free(sbp);
	::free(freeDinodep);
	delete fsreader;

	err = mount(devname, mntpoint, FSTYPE_TARFS,  MS_RDONLY, &arg);
	if (err != 0) {
		fprintf(stderr, "mount(2) err=%d\n", errno);
		return 1;
	}
	return 0;
}

