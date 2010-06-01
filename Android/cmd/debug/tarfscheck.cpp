#include "tarfs.h"

using namespace Tarfs;

int
main(int argc, const char* argv[])
{
	char *fname;
	TARINO ino;

	if (argc != 3 && argc != 4) {
		fprintf(stderr, "Usage: %s tarfile ino [s]\n", argv[0]);
		return 1;
	}
	fname = (char*)argv[1];
	ino = atoi(argv[2]);
	FsReader *fsreader = FsReader::create(fname, FsReader::TARFILE);
	if (!fsreader) {
		return 1;
	}
	if (argc == 4) {
		printf("******* superblock ********\n");
		fsreader->printSblock();
		printf("****************************\n");
	}

	Inode *inode = InodeFactory::getInode(fsreader, ino, -1);
	if (!inode) {
		delete fsreader;
		fprintf(stderr, "can not get inode\n");
		return 1;
	}
	printf("*******  dinode(%llu) ********\n", inode->getIno());
	inode->print();
	printf("****************************\n");

	if (inode->isDir()) {
		Dir *dir = static_cast<Dir*>(inode);
		printf("*******  dir data(%llu) ********\n", inode->getIno());
		dir->printDirEntry(fsreader);
		printf("****************************\n");
	}

	delete fsreader;
	delete inode;
	return 0;
}
