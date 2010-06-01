#include "tarfs.h"

using namespace Tarfs;

File::File(std::vector<char*> *path, tarfs_dext *addr, 
		TARBLK blkno_header, posix_header *header)
{
	this->path = path;
	this->addrExtent = *addr;
	this->blkno_header = blkno_header;
	this->header = *header;
}
File::~File()
{
	if (this->path) {
		this->path->clear();
		delete this->path;
		this->path = NULL;
	}
}

void File::printPath()
{
	for (uint32_t i = 0; i < this->path->size(); i++) {
		printf("%s", (*(this->path))[i]);
		if (i == (this->path->size()-1)) {
			printf("\n");
		} else {
			printf("/");
		}
	}
}
void File::printHeader()
{
	printf("addrExtent.de_blkno=%Lu\n", this->addrExtent.de_blkno);
	printf("addrExtent.de_nblks=%Lu\n", this->addrExtent.de_nblks);
}

int File::getFtype()
{
	return Util::getFtype(this->header.typeflag);
}
TARMODE File::getMode()
{
	return Util::strtoint(this->header.mode, sizeof(this->header.mode));
}
TARUID File::getUid()
{
	return Util::strtoint(this->header.uid, sizeof(this->header.gid));
}
TARGID File::getGid()
{
	return Util::strtoint(this->header.gid, sizeof(this->header.gid));
}
TARTIME File::getMtime()
{
	return Util::strtoint(this->header.mtime, sizeof(this->header.mtime));
}
TARSIZE File::getFileSize()
{
	if (this->getFtype() == TARFS_IFLNK) {
		return ::strlen(this->header.linkname);
	} else {
		return Util::strtoint(this->header.size, sizeof(this->header.size));
	}
}
TARBLK File::getBlocks()
{
	if (this->getFtype() == TARFS_IFREG) {
		return TARFS_BLOCKS(this->getFileSize());
	} else {
		return 0;
	}
}
TARNEXT File::getNumExtents()
{
	if (this->getFtype() == TARFS_IFREG &&
		this->getFileSize() > 0) {
		return 1;
	}
	return 0;
}
void File::getExtent(tarfs_dext *extent)
{
	*extent = this->addrExtent;
}
