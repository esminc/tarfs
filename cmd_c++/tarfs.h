#ifndef _TARFS_H_
#define _TARFS_H_
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <vector>
#include "tarfs_meta.h"

#define MAXPATHLEN	4096
typedef uint64_t TARBLK;

namespace Tarfs {
	class Util {
		private:
		Util() {}
		~Util() {}
		public:
		static uint64_t strtoint(char *p, size_t siz);
		static int getFtype(uint32_t typeflag);
		static uint64_t getCurrentTime();
	};
	class File {
		private:
		std::vector<char*> *path;
		TARBLK blkno_header;
		posix_header header;
		tarfs_dext addrExtent;
		public:
		TARBLK   getBlknoHeader() { return this->blkno_header; }
		uint64_t getMode();
		uint64_t getUid();
		uint64_t getGid();
		uint64_t getMtime();
		uint64_t getFileSize();
		uint64_t getBlocks();
		uint64_t getNumExtents();
		void     getExtent(tarfs_dext *extent);
		int      getFtype();
		File(std::vector<char*> *path, tarfs_dext *addrExtent, 
					TARBLK blkno_header, posix_header *header);
		~File();
		void printPath();
		void printHeader();
		inline char* operator [] (int i) { return path->at(i); }
		inline int entSize() { return this->path->size(); }
	};
	class Parser {
		private:
		int fd;
		TARBLK searchBlkno;
		char fname[MAXPATHLEN];
		Parser(const char *fname);
		public:
		static std::vector<char*> *parsePath(char *path);
		static Parser *create(const char *fname);
		~Parser();
		File *get();
	};
	class FsMaker;
	class Inode;
	class Dir;
	class InodeFactory {
		private:
		static Inode *freeInode;
		static uint64_t inodeNum;
		static TARBLK totalDataSize;
		InodeFactory() {}
		~InodeFactory() {}
		public:
		static bool init(FsMaker *fs);
		static Inode *allocInode(FsMaker *fs, uint64_t pino, File *file);
		static Inode *allocInode(FsMaker *fs, uint64_t pino, int ftype);
		static Inode *getInode(FsMaker *fs, uint64_t ino, uint64_t pino, int ftype);
		static uint64_t getInodeNum() { return inodeNum; }
		static TARBLK getTotalDataSize() { return totalDataSize; }
		static void fin(Tarfs::FsMaker *fs);
	};
	class Inode {
		int ftype;
		bool isMod;
		protected:
		uint64_t ino;
		uint64_t pino;
		uint64_t blkno;
		tarfs_dinode dinode;
		void initialize(uint64_t ino, uint64_t pino, uint64_t blkno);
		public:
		Inode(uint64_t ino, uint64_t pino, uint64_t blkno);
		Inode(uint64_t ino, uint64_t pino, uint64_t blkno, int ftype);
		Inode(uint64_t ino, uint64_t pino, uint64_t blkno, FsMaker *fs);
		~Inode() {}
		inline void setFtype(int ftype) {
			this->ftype = ftype;
			this->dinode.di_mode = (ftype|0755);
		}
		inline bool isDir() { return this->ftype == TARFS_IFDIR; }
		inline bool isReg() { return this->ftype == TARFS_IFREG; }
		inline bool isSym() { return this->ftype == TARFS_IFLNK; }
		inline bool isDirty() { return this->isMod; }
		inline void setDirty() { this->isMod = true; }
		inline void setClean() { this->isMod = false; }
		inline uint64_t getIno() { return this->ino; }
		void setFile(File *file);
		uint64_t getDirFtype();
		void insBlock(FsMaker *fs, tarfs_dext *de);
		void getBlock(FsMaker *fs, uint64_t off, uint64_t *blkno);
		void sync(FsMaker *fs);
	};
	class Dir : public Inode {
		private:
		struct dirFreeSpace {
			uint64_t off;
			uint64_t blkno;
			char	 dirbuf[TAR_BLOCKSIZE];
		};
		void searchFreeSpace(FsMaker *fs, char *name, 
						dirFreeSpace *dfs);
		void addDirEntry(FsMaker *fs, char *name, 
					dirFreeSpace *dfs, Inode *inode);
		public:
		Dir(uint64_t ino, uint64_t pino, uint64_t blkno) :
			Inode(ino, pino, blkno, TARFS_IFDIR) {}
		Dir(uint64_t ino, uint64_t pino, uint64_t blkno, FsMaker *fs) :
			Inode(ino, pino, blkno, fs) {}
		void dirInit(FsMaker *fs);
		Inode *lookup(FsMaker *fs, char *name);
		Dir *mkdir(FsMaker *fs, char *name);
		Inode *create(FsMaker *fs, File *file);
	};
#define	ALLOC_DIRDATA_NUM	128
#define	ALLOC_IEXDATA_NUM	128
#define	ALLOC_DINODE_NUM	128
	class SpaceManager {
		private:
		uint64_t blkno;
		int bulks;
		public:
		SpaceManager(uint64_t bulks) {
			this->blkno = TARBLK_NONE;
			this->bulks = bulks;
		}
		~SpaceManager() {}
		inline uint64_t getblkno() { return this->blkno; }
		inline int getbulks() { return this->bulks; }
		void allocBlock(FsMaker *fs, uint64_t *blkno);
	};
	class FsMaker {
		private:
		char fname[MAXPATHLEN];
		int fd;
		uint64_t blks;
		uint64_t orgblks;
		tarfs_sblock sb;
		Dir *root;
		SpaceManager *dirmgr;
		SpaceManager *iexmgr;
		SpaceManager *dinodemgr;
		FsMaker(char *fname);
		public:
		static FsMaker *create(char *fname);
		~FsMaker();
		inline uint64_t nblks() { return this->blks; }
		int init();
		void add(File *file);
		void complete(); /* write super block */
		void rollback(); /* error case only */
		void createBlock(uint64_t *blkno, int nblks);
		void readBlock(char* bufp, uint64_t blkno, ssize_t nblks);
		void writeBlock(char* bufp, uint64_t blkno, int nblks);
		inline SpaceManager *getDirManager() { return this->dirmgr; }
		inline SpaceManager *getIexManager() { return this->iexmgr; }
		inline SpaceManager *getDinodeManager() { return this->dinodemgr; }
	};
}

#endif /* _TARFS_H_ */
