#include "tarfs.h"

Tarfs::Parser::Parser(const char *fname)
{
	::memset(this->fname, 0, 4096);
	::memcpy(this->fname, fname, strlen(fname));
	this->fd = -1;
	this->curblk = 0;
}
bool Tarfs::Parser::init()
{
	this->fd = ::open(this->fname, O_RDONLY);
	if (this->fd < 0) {
		return false;
	}
	return true;
}
Tarfs::Parser::~Parser()
{
	if (this->fd >= 0) {
		::close(this->fd);
		this->fd = -1;
	}
}
Tarfs::File* Tarfs::Parser::get()
{
	char path[4096];
	ssize_t res;
	struct posix_header header;
	uint64_t data_blks;
	tarfs_dext addr;

	/* read tar headdr */
	res = ::pread(this->fd, (void*)&header, 
			TAR_BLOCKSIZE, this->curblk*TAR_BLOCKSIZE);
	if (res != TAR_BLOCKSIZE) {
		fprintf(stderr, "parse_tar:pread(2) err=%d\n", errno);
		return NULL;
	}
	if (header.name[0] == '\0') {
		/* reach eof */
		return NULL;
	}
	switch (header.typeflag) {
	case TARFS_REGTYPE:
	case TARFS_AREGTYPE:
	case TARFS_LNKTYPE:
	case TARFS_SYMTYPE:
	case TARFS_CHRTYPE:
	case TARFS_BLKTYPE:
	case TARFS_DIRTYPE:
	case TARFS_FIFOTYPE:
	case TARFS_CONTTYPE:
		/* Posix header only */
		::memcpy(path, header.name, strlen(header.name));
		path[strlen(header.name)] = '\0';
		break;
	case TARFS_GNU_LONGNAME:
		ssize_t path_size;
		ssize_t path_blks;
		/* get path size */
		path_size = Tarfs::File::strtoint(header.size, 
						sizeof(header.size));
		path_blks = TARFS_BLOCKS(path_size);
		/* read path */
		this->curblk++;
		res = ::pread(this->fd, path, path_blks*TAR_BLOCKSIZE, 
					(this->curblk)*TAR_BLOCKSIZE);
		if (res != (path_blks*TAR_BLOCKSIZE)) {
			fprintf(stderr, "tarfs: failed to read headers\n");
			return NULL;
		}
		/* read real header */
		this->curblk += path_blks;
		res = ::pread(this->fd, (void*)&header, TAR_BLOCKSIZE, 
					(this->curblk)*TAR_BLOCKSIZE);
		if (res != TAR_BLOCKSIZE) {
			fprintf(stderr, "tarfs: failed to read headers\n");
			return NULL;
		}
		break;
	default:
		/* TARFS_GNU_LONGLINK */
		fprintf(stderr, "tarfs: invalid or unsupported typeflag(%c). "
			"Headers can not be skipped correctly.\n", 
			header.typeflag);
		return NULL;
	}
	/* get data size */
	data_blks = 0;
	if (header.typeflag == TARFS_REGTYPE) {
		uint64_t data_size = Tarfs::File::strtoint(header.size, 
						sizeof(header.size));
		data_blks = TARFS_BLOCKS(data_size);
	}
	addr.de_off = 0;
	addr.de_blkno = this->curblk + 1; /* data offset */
	addr.de_nblks = data_blks; /* data size */
	this->curblk += (addr.de_nblks + 1);
	return  new File(path, &addr, &header);
}
Tarfs::File::File(char* path, tarfs_dext *addr, posix_header *header)
{
	this->numNames = 0;
	this->pathlen = strlen(path);
	::memcpy(this->path, path, this->pathlen);
	this->path[this->pathlen] = '\0';
	this->addrExtent = *addr;
	this->header = *header;
}
bool Tarfs::File::parse()
{
	char *p = this->path;
	char *s = this->path;
	char *end = &this->path[this->pathlen-1];
	if (*s == '/') s++;
	while (p < end) {
		for (p = s; ((*p != '/') && (*p != '\0')) ; p++){}
		*p = '\0';
		int len = strlen(s);
		if (len == 1 && *s == '.') {
			/* . is invalid filename */
			return false;
		} else if (len == 2 && !strcmp(s, "..")) {
			/* .. is invalid filename */
			return false;
		}
		this->names.push_back(s);
		this->numNames++;
		s += len + 1;
	}
	return true;
}

void Tarfs::File::printPath()
{
	printf("%s\n", this->path);
}
void Tarfs::File::printHeader()
{
	printf("addr.de_blkno=%Lu\n", this->addrExtent.de_blkno);
	printf("addr.de_nblks=%Lu\n", this->addrExtent.de_nblks);
}

uint64_t Tarfs::File::strtoint(char *p, size_t siz)
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
int Tarfs::File::getFtype()
{
	return Tarfs::File::getFtype(this->header.typeflag);
}
int Tarfs::File::getFtype(uint32_t typeflag)
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

uint64_t Tarfs::File::getCurrentTime()
{
	uint64_t now;
	struct timeval tv;
	(void)::gettimeofday(&tv, NULL);
	now = (((uint64_t)tv.tv_sec)*(uint64_t)1000000) + (uint64_t)tv.tv_usec;
	return now;
}
