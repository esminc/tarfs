#include "tarfs.h"

using namespace Tarfs;

int
main(int argc, const char* argv[])
{
	int err;
	tarfs_mnt_arg arg;
	char *devname;
	char *mntpoint;
	char mntopts[10240];
	FsReader *fsreader;
	tarfs_sblock *sbp;
	tarfs_dinode *freeDinodep;
	mntent tabent;

	if (argc != 3) {
		fprintf(stderr, "Usage:%s devname mntpoint\n", argv[0]);
		return 1;
	}

	devname = (char*)argv[1];
	mntpoint = (char*)argv[2];

	fsreader = FsReader::create(devname, FsReader::DEVFILE);
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
	sprintf(mntopts, "%s,loop=%s", MNTOPT_RO, devname);
	tabent.mnt_fsname = devname;
	tabent.mnt_dir = mntpoint;
	tabent.mnt_type = (char*)FSTYPE_TARFS;
	tabent.mnt_opts = mntopts;
	tabent.mnt_freq = 0;
	tabent.mnt_passno = 0;
	FILE *fp = setmntent(MOUNTED, "a+");
	if (fp) {
		addmntent(fp, &tabent);
		endmntent(fp);
	}
	return 0;
}

