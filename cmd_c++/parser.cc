#include "tarfs.h"

using namespace Tarfs;
Parser::Parser(const char *fname)
{
	::memset(this->fname, 0, MAXPATHLEN);
	::memcpy(this->fname, fname, strlen(fname));
	this->fd = -1;
	this->curoff = 0;
}
Parser *Parser::create(const char *fname)
{
	Parser *parser = new Parser(fname);
	if (parser) {
		parser->fd = ::open(parser->fname, O_RDONLY);
		if (parser->fd < 0) {
			delete parser;
			parser = NULL;
		}
	}
	return parser;
}
Parser::~Parser()
{
	if (this->fd >= 0) {
		::close(this->fd);
		this->fd = -1;
	}
}
File* Parser::get()
{
	char path[MAXPATHLEN];
	ssize_t res;
	struct posix_header header;
	TARBLK data_blks;
	tarfs_dext addr;

	/* read tar headdr */
	res = ::pread(this->fd, (void*)&header, 
			TAR_BLOCKSIZE, this->curoff*TAR_BLOCKSIZE);
	if (res != TAR_BLOCKSIZE) {
		fprintf(stderr, "parse_tar:pread(2) err=%d\n", errno);
		return NULL;
	}
	if (header.name[0] == '\0') { /* reach eof */
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
		path_size = Util::strtoint(header.size, 
						sizeof(header.size));
		path_blks = TARFS_BLOCKS(path_size);
		/* read path */
		this->curoff++;
		res = ::pread(this->fd, path, path_blks*TAR_BLOCKSIZE, 
					(this->curoff)*TAR_BLOCKSIZE);
		if (res != (path_blks*TAR_BLOCKSIZE)) {
			fprintf(stderr, "tarfs: failed to read headers\n");
			return NULL;
		}
		/* read real header */
		this->curoff += path_blks;
		res = ::pread(this->fd, (void*)&header, TAR_BLOCKSIZE, 
					(this->curoff)*TAR_BLOCKSIZE);
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
	std::vector<char*> *names = Parser::parsePath(path);
	if (!names) {
		return NULL;
	}
	/* get data size */
	data_blks = 0;
	if (header.typeflag == TARFS_REGTYPE) {
		uint64_t data_size = Util::strtoint(header.size, 
						sizeof(header.size));
		data_blks = TARFS_BLOCKS(data_size);
	}
	addr.de_off = 0;
	addr.de_blkno = this->curoff + 1; /* data offset */
	addr.de_nblks = data_blks; /* data size */
	File *file =  new File(names, &addr, this->curoff, &header);
	if (!file) {
		names->clear();
		delete names;
	}
	this->curoff += (addr.de_nblks + 1);
	return file;
}
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
std::vector<char*> *Parser::parsePath(char *path)
{
	std::vector<char*> *names = new std::vector<char*>;
	if (!names) {
		return NULL;
	}
	char *entry;
	char *p;
	char *s = path;
	char *end = &path[strlen(path)];
	if (*s == '/') s++;
	p = s;
	while (p < end) {
		if (*p == '\0' || *p == '/') { /* skip '\0', '/' */
			p++;
			continue;
		}
		for (p = s; ((*p != '/') && (*p != '\0')) ; p++){} /* search last */
		*p = '\0'; /* change '/' to '\0' */
		int len = strlen(s);
		if ((len == 1 && *s == '.') /* . is invalid filename */ ||
			(len == 2 && !strcmp(s, "..")) /* .. is invalid filename */ ||
			(!(entry = (char*)::malloc(strlen(s)+1)))) {
			names->clear();
			delete names;
			return NULL;
		}
		memcpy(entry, s, len);
		entry[len] = '\0';
		names->push_back(s);
		s += len + 1;
	}
	if (names->size() == 0) {
		delete names;
		names = NULL;
	}
	return names;
}

void File::printPath()
{
	for (uint32_t i = 0; i < this->path->size(); i++) {
		printf("%s", this->path->at(i));
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
