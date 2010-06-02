#include "tarfs.h"

using namespace Tarfs;

int
main(int argc, const char* argv[])
{
	int err;
	tarfs_mnt_arg arg;
	char *tarfile;
	char *ramdisk;
	char *mntpoint;
	FsReader *fsreader;
	tarfs_sblock *sbp;
	tarfs_dinode *freeDinodep;

	if (argc != 4) {
		fprintf(stderr, "Usage:%s tarfile ramdisk mntpoint\n", argv[0]);
		return 1;
	}

	tarfile = (char*)argv[1];
	ramdisk = (char*)argv[2];
	mntpoint = (char*)argv[3];

	fsreader = FsReader::create(tarfile, ramdisk);
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

	err = mount(ramdisk, mntpoint, FSTYPE_TARFS,  MS_RDONLY, &arg);
	if (err != 0) {
		fprintf(stderr, "mount(2) err=%d\n", errno);
		return 1;
	}
	return 0;
}

