#include "tarfs.h"

using namespace Tarfs;

uint64_t Util::strtoint(char *p, size_t siz)
{
	char buf[TAR_BLOCKSIZE];
	uint64_t val;
	uint32_t i;
	for(i = 0; i < siz; i++){
	if (!isspace(p[i]))	/* '\0' is control character */
		break;
	}
	memcpy(buf, p+i, siz-i);
	buf[siz-i]='\0';
	val=strtoull(buf, NULL, 8);
	return val;
}
int Util::getFtype(uint32_t typeflag)
{
	uint32_t ftype = -1;
	switch(typeflag){
	case TARFS_REGTYPE:
		ftype = TARFS_IFREG;
		break;
	case TARFS_DIRTYPE:
		ftype = TARFS_IFDIR;
		break;
	case TARFS_SYMTYPE:
		ftype = TARFS_IFLNK;
		break;
	default:
		break;
	}
	return ftype;
}

TARTIME Util::getCurrentTime()
{
	TARTIME now;
	struct timeval tv;
	(void)::gettimeofday(&tv, NULL);
	now = (((TARTIME)tv.tv_sec)*(TARTIME)1000000) + (TARTIME)tv.tv_usec;
	return now;
}
