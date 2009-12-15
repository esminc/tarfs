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

namespace Tarfs {
	class File {
		private:
		int numNames;
		std::vector<char*> names;
		int pathlen;
		char path[4096];
		public:
		tarfs_dext addrExtent;
		posix_header header;
		static uint64_t strtoint(char *p, size_t siz);
		static int getFtype(uint32_t typeflag);
		static uint64_t getCurrentTime();
		int getFtype();
		File(char* path, tarfs_dext *addrExtent, posix_header *header);
		~File() {}
		void printPath();
		void printHeader();
		bool parse();
		inline char* operator [] (int i) { return names[i]; }
		inline int entSize() { return this->numNames; }
	};
	class Parser {
		private:
		char fname[4096];
		int fd;
		uint64_t curblk;
		public:
		Parser(const char *fname);
		~Parser();
		bool init();
		File *get();
	};
	class FsMaker;
	class Inode {
		private:
		static Inode *freeInode;
		static uint64_t inodeNum;
		int ftype;
		bool isMod;
		protected:
		uint64_t pino;
		uint64_t blkno;
		bool isLoad;
		public:
		uint64_t ino;
		tarfs_dinode dinode;
		static bool init(FsMaker *fs);
		static void setFreeInode(Inode *inode);
		static Inode *getFreeInode();
		static Inode *allocInode(FsMaker *fs, uint64_t pino, int ftype);
		static void incInodeNum();
		static uint64_t getInodeNum();
		Inode *getInode(FsMaker *fs, uint64_t ino, uint64_t pino);
		Inode(uint64_t ino, uint64_t pino, uint64_t blkno);
		~Inode() {}
		void load(FsMaker *fs);
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
		void setFile(File *file);
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
		void searchFreeSpace(FsMaker *fs, char *name, dirFreeSpace *dfs);
		void addDirEntry(FsMaker *fs, char *name, dirFreeSpace *dfs, Inode *inode);
		public:
		Dir(uint64_t ino, uint64_t pino, uint64_t blkno) :
			Inode(ino, pino, blkno) { }
		void init(FsMaker *fs);
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
		char fname[4096];
		int fd;
		uint64_t blks;
		uint64_t orgblks;
		tarfs_sblock sb;
		Dir *root;
		SpaceManager *dirmgr;
		SpaceManager *iexmgr;
		SpaceManager *dinodemgr;
		public:
		FsMaker(char *fname);
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
