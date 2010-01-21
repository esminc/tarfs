#ifndef _TARFS_H_
#define _TARFS_H_
#define _XOPEN_SOURCE 500
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <mntent.h>
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

#define FSTYPE_TARFS	"tarfs"

#define MAXPATHLEN	4096
typedef uint64_t TARBLK;  /* TAR_BLOCKSIZE */
typedef uint64_t TARINO;  /* inode number */
typedef uint64_t TAROFF;  /* byte */
typedef uint32_t TARMODE; /* mode */
typedef uint32_t TARUID;  /* uid */
typedef uint32_t TARGID;  /* gid */
typedef uint64_t TARTIME; /* time(usec) */
typedef uint64_t TARSIZE; /* file size */
typedef uint64_t TARNEXT; /* num extents */

namespace Tarfs {
	/**
	 * Utilitiy class for tarfs
	 */
	class Util {
		private:
		/** This class can not be instance.
		 * so constructor is private.
		 */
		Util() {}
		/** This class can not be instance.
		 * so destructor is private.
		 */
		~Util() {}
		public:
		static uint64_t strtoint(char *p, size_t siz);
		static int getFtype(uint32_t typeflag);
		static TARTIME getCurrentTime();
	};
	/**
	 * tar アーカイブファイルをパースした結果を
	 * ファイル単位で管理するクラス
	 */
	class File {
		private:
		std::vector<char*> *path;
		TARBLK blkno_header;
		posix_header header;
		tarfs_dext addrExtent;
		public:
		TARBLK  getBlknoHeader() { return this->blkno_header; }
		TARMODE getMode();
		TARUID  getUid();
		TARGID  getGid();
		TARTIME getMtime();
		TARSIZE getFileSize();
		TARBLK  getBlocks();
		TARNEXT getNumExtents();
		void    getExtent(tarfs_dext *extent);
		int     getFtype();
		File(std::vector<char*> *path, tarfs_dext *addrExtent, 
					TARBLK blkno_header, posix_header *header);
		~File();
		void printPath();
		void printHeader();
		inline char* operator [] (int i) { return path->at(i); }
		inline int entSize() { return this->path->size(); }
	};
	/**
	 * tar アーカイブファイルをパースするクラス
	 */
	class Parser {
		private:
		int fd;
		TARBLK curoff;
		char fname[MAXPATHLEN];
		Parser(const char *fname);
		public:
		static std::vector<char*> *parsePath(char *path);
		static Parser *create(const char *fname);
		~Parser();
		File *get();
	};
	class FsIO;
	class FsMaker;
	/**
	 * tar ファイルシステムのファイルを inode として
	 * 管理するクラス
	 */
	class Inode {
		int ftype;
		bool isMod;
		protected:
		TARINO ino;
		TARINO pino;
		TARBLK blkno;
		tarfs_dinode dinode;
		void initialize(TARINO ino, TARINO pino, TARBLK blkno);
		public:
		Inode(TARINO ino, TARINO pino, TARBLK blkno);
		Inode(TARINO ino, TARINO pino, TARBLK blkno, int ftype);
		Inode(TARINO ino, TARINO pino, TARBLK blkno, FsIO *fsio);
		~Inode() {}
		tarfs_dinode *getDinodeImage();
		inline bool isDir() { return this->ftype == TARFS_IFDIR; }
		inline bool isReg() { return this->ftype == TARFS_IFREG; }
		inline bool isSym() { return this->ftype == TARFS_IFLNK; }
		inline bool isDirty() { return this->isMod; }
		inline void setDirty() { this->isMod = true; }
		inline void setClean() { this->isMod = false; }
		inline TARINO getIno() { return this->ino; }
		void setFile(File *file);
		int getDirFtype();
		void insBlock(FsMaker *fs, tarfs_dext *de);
		void getBlock(FsIO *fsio, TARBLK off, TARBLK *blkno);
		void sync(FsIO *fsio);
		void print();
	};
	/**
	 * tar ファイルシステムのディレクトリを inode として
	 * 管理するクラス。
	 */
	class Dir : public Inode {
		private:
		struct dirFreeSpace {
			TAROFF off;
			TARBLK blkno;
			char   dirbuf[TAR_BLOCKSIZE];
		};
		void searchFreeSpace(FsMaker *fs, char *name, 
						dirFreeSpace *dfs);
		void addDirEntry(FsMaker *fs, char *name, 
					dirFreeSpace *dfs, Inode *inode);
		public:
		Dir(TARINO ino, TARINO pino, TARBLK blkno) :
			Inode(ino, pino, blkno, TARFS_IFDIR) {}
		Dir(TARINO ino, TARINO pino, TARBLK blkno, FsIO *fsio) :
			Inode(ino, pino, blkno, fsio) {}
		void dirInit(FsMaker *fs);
		Inode *lookup(FsIO *fsio, char *name);
		Dir *mkdir(FsMaker *fs, char *name);
		Inode *create(FsMaker *fs, File *file);
		void printDirEntry(FsIO *fsio);
	};
	class FsMaker;
	/**
	 * inode を管理するクラス
	 */
	class InodeFactory {
		private:
		static Inode *freeInode;
		static TARINO inodeNum;
		static TARBLK totalDataSize;
		InodeFactory() {}
		~InodeFactory() {}
		public:
		static bool init(FsMaker *fs);
		static bool load(FsIO *fsio, TARBLK freeInodeBlkno);
		static Inode *getFreeInode() { return InodeFactory::freeInode; }
		static Inode *allocInode(FsMaker *fs, TARINO pino, File *file);
		static Inode *allocInode(FsMaker *fs, TARINO pino, int ftype);
		static Inode *getInode(FsIO *fsio, TARINO ino, TARINO pino, int ftype);
		static Inode *getInode(FsIO *fsio, TARINO ino, TARINO pino);
		static TARINO getInodeNum() { return inodeNum; }
		static TARBLK getTotalDataSize() { return totalDataSize; }
		static void fin(FsIO *fsio);
	};
	/**
	 * ディスクスペースを管理するクラス
	 */
	class SpaceManager {
#define	ALLOC_DIRDATA_NUM	128
#define	ALLOC_IEXDATA_NUM	128
#define	ALLOC_DINODE_NUM	128
		private:
		TARBLK blkno;
		int bulks;
		public:
		SpaceManager(int bulks) {
			this->blkno = TARBLK_NONE;
			this->bulks = bulks;
		}
		~SpaceManager() {}
		inline TARBLK getblkno() { return this->blkno; }
		inline int getbulks() { return this->bulks; }
		void allocBlock(FsMaker *fs, TARBLK *blkno);
	};
	class FsIO {
		protected:
		char fname[MAXPATHLEN];
		int fd;
		TARBLK blks;
		tarfs_sblock sb;
		Dir *root;
		FsIO(char *fname)
		{
			this->root = NULL;
			::memset(this->fname, 0, MAXPATHLEN);
			::memcpy(this->fname, fname, strlen(fname));
			this->fd = -1;
		}
		public:
		tarfs_sblock *getSblock()
		{
			tarfs_sblock *sbp;
			sbp = (tarfs_sblock*)::malloc(sizeof(tarfs_sblock));
			if (!sbp) {
				return NULL;
			}
			::memcpy(sbp, &this->sb, sizeof(tarfs_sblock));
			return sbp;
		}
		virtual void rollback() {}
		virtual void writeBlock(char* bufp, TARBLK blkno, ssize_t nblks) = 0;
		virtual void readBlock(char* bufp, TARBLK blkno, ssize_t nblks) 
		{
			ssize_t res;
			res = ::pread(this->fd, bufp, TAR_BLOCKSIZE*nblks, 
					blkno*(TARBLK)TAR_BLOCKSIZE);
			if (res != (TAR_BLOCKSIZE*nblks)) {
				::fprintf(stderr, "pread(2) %s off=%Lu err=%d\n", 
					this->fname, this->blks, errno);
				exit(1);
			}
		}
		void printSblock()
		{
			tarfs_sblock *sbp = &this->sb;
			printf("#tarfs_magic:0x%llx\n", sbp->tarfs_magic);
			printf("#tarfs_inode:%llu\n", sbp->tarfs_inodes);
			printf("#tarfs_size:%llu\n", sbp->tarfs_size);
			printf("#tarfs_fsize:%llu\n", sbp->tarfs_fsize);
			printf("#tarfs_dsize:%llu\n", sbp->tarfs_dsize);
			printf("#tarfs_flist_regdata:%llu\n", sbp->tarfs_flist_regdata);
			printf("#tarfs_flist_dirdata:%llu\n", sbp->tarfs_flist_dirdata);
			printf("#tarfs_flist_iexdata:%llu\n", sbp->tarfs_flist_iexdata);
			printf("#tarfs_flist_indDinode:%llu\n", sbp->tarfs_flist_indDinode);
			printf("#tarfs_flist_dinode:%llu\n", sbp->tarfs_flist_dinode);
			printf("#tarfs_root:%llu\n", sbp->tarfs_root);
			printf("#tarfs_free:%llu\n", sbp->tarfs_free);
			printf("#tarfs_flags:0x%llx\n", sbp->tarfs_flags);
			printf("#tarfs_maxino:%llu\n", sbp->tarfs_maxino);
		}
	};
	/**
	 * tar アーカイブファイルをパースした結果（File）を元に、
	 * tar ファイルシステムのメタデータを構築するするクラス
	 */
	class FsMaker : public FsIO {
		private:
		TARBLK orgblks;
		SpaceManager *dirmgr;
		SpaceManager *iexmgr;
		SpaceManager *dinodemgr;
		FsMaker(char *fname) : FsIO(fname) {}
		public:
		static FsMaker *create(char *fname);
		~FsMaker();
		inline TARBLK nblks() { return this->blks; }
		int init();
		void add(File *file);
		void complete(); /* write super block */
		void rollback(); /* error case only */
		void readBlock(char* bufp, TARBLK blkno, ssize_t nblks);
		void writeBlock(char* bufp, TARBLK blkno, ssize_t nblks);
		inline SpaceManager *getDirManager() { return this->dirmgr; }
		inline SpaceManager *getIexManager() { return this->iexmgr; }
		inline SpaceManager *getDinodeManager() { return this->dinodemgr; }
	};
	class FsReader : public FsIO {
		private:
		FsReader(char *fname) : FsIO(fname) {}
		public:
		enum TarfileType {
			DEVFILE,
			TARFILE,
		};
		~FsReader();
		static FsReader *create(char *tarfile, TarfileType type);
		tarfs_dinode *getFreeDinode();
		void writeBlock(char* bufp, TARBLK blkno, ssize_t nblks);
	};
}

#endif /* _TARFS_H_ */
