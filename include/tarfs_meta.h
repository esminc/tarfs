#ifndef _TARFS_META_H_
#define _TARFS_META_H_

/* Information in tar archive */

#define TAR_BLOCKSIZE	512
#define TAR_BLOCKBITS	9

/**
 * typeflag
 */
#define TARFS_REGTYPE  '0'	/* regular file */
#define TARFS_SYMTYPE  '2'	/* symlink file */
#define TARFS_DIRTYPE  '5'	/* directory file */
#define TARFS_GNU_LONGNAME 'L'	/* file name len > 100 */
struct posix_header
{				/* dec:oct : */
	char name[100];		/*   0:00000: file name */
	char mode[8];		/* 100:00144: file mode */
	char uid[8];		/* 108:00154: uid */
	char gid[8];		/* 116:00164: gid */
	char size[12];		/* 124:00174: file size */
	char mtime[12];		/* 136:00210: mtime */
	char chksum[8];		/* 148:00224: chksum */
	char typeflag;		/* 156:00234: typeflag */
  	char linkname[100];	/* 157:00235: linkname */
	char magic[6];		/* 257:00401: magic */
	char version[2];	/* 263:00407: version */
	char uname[32];		/* 265:00409 user name */
	char gname[32];		/* 297:00451: group name */
	char devmajor[8];	/* 329:00511: dev major */
	char devminor[8];	/* 337:00521: dev minor */
	char padding[167];	/* 345:00531: padding */
};				/* 512:01000: */

/**
 * not support typeflag
 */
#define TARFS_AREGTYPE '\0'
#define TARFS_LNKTYPE  '1'
#define TARFS_CHRTYPE  '3'
#define TARFS_BLKTYPE  '4'
#define TARFS_FIFOTYPE '6'
#define TARFS_CONTTYPE '7'
#define TARFS_GNU_DUMPDIR  'D'
#define TARFS_GNU_LONGLINK 'K'	/* symlink name len > 100 */
#define TARFS_GNU_MULTIVOL 'M'
#define TARFS_GNU_NAMES    'N'
#define TARFS_GNU_SPARSE   'S'
#define TARFS_GNU_VOLHDR   'V'

/* Mode field */
#define TARFS_SUID     04000
#define TARFS_SGID     02000
#define TARFS_SVTX     01000
#define TARFS_MODEMASK 00777

#define	TARSBLOCK_MAGIC	((uint64_t)0xbeafcafe)
#define	TARDINODE_MAGIC	((uint64_t)0xdacbdacb)
#define	TARFREE_MAGIC	((uint64_t)0xdeaddead)
#define	TARINDE_MAGIC	((uint64_t)0xbeebbeeb)
#define	TARBLK_NONE	((uint64_t)-1)
#define TARFS_FREE_INO  0
#define TARFS_ROOT_INO  1


struct tarfs_dext {
	uint64_t	de_off;
	uint64_t	de_blkno;
	uint64_t	de_nblks;
};
typedef struct tarfs_dext tarfs_dext_t;

#define MAX_INDEXT	20
struct tarfs_indExt {
	uint64_t	ie_magic;
	uint64_t	ie_next;
	uint32_t	ie_num;
	char		ie_padding[12];
	tarfs_dext_t	ie_dext[MAX_INDEXT];
};
typedef struct tarfs_indExt tarfs_indExt_t;

#define TARSBLOCK_FLAG_NG	1
#define TARSBLOCK_FLAG_OK	2
#define TARSBLOCK_FLAG_DIRTY	3
struct tarfs_sblock {
	uint64_t	tarfs_magic;
	uint64_t	tarfs_inodes;
	uint64_t	tarfs_dsize;
	uint64_t	tarfs_size;
	uint64_t	tarfs_fsize;
	uint64_t	tarfs_flist_regdata;
	uint64_t	tarfs_flist_dirdata;
	uint64_t	tarfs_flist_iexdata;
	uint64_t	tarfs_flist_indDinode;
	uint64_t	tarfs_flist_dinode;
	uint64_t	tarfs_root;
	uint64_t	tarfs_free;
	uint64_t	tarfs_flags;
	uint64_t	tarfs_maxino;
	char		tarfs_padding[400];
};
typedef struct tarfs_sblock tarfs_sblock_t;

#define TARFS_IFMT  00170000
#define TARFS_IFSOCK 0140000
#define TARFS_IFLNK  0120000
#define TARFS_IFREG  0100000
#define TARFS_IFBLK  0060000
#define TARFS_IFDIR  0040000
#define TARFS_IFCHR  0020000
#define TARFS_IFIFO  0010000
#define TARFS_ISUID  0004000
#define TARFS_ISGID  0002000
#define TARFS_ISVTX  0001000

#define MAXNAMLEN 255
#define	TARFS_FTYPE_NON	0
#define	TARFS_FTYPE_REG	1
#define	TARFS_FTYPE_DIR	2
#define	TARFS_FTYPE_SYM	3
#define TARFS_FTYPE_DIR2DINODE(dtype)					\
	((dtype) == TARFS_FTYPE_REG) ? TARFS_IFREG : 			\
	((dtype) == TARFS_FTYPE_DIR) ? TARFS_IFDIR :			\
	((dtype) == TARFS_FTYPE_SYM) ? TARFS_IFLNK :	0

#define TARFS_FTYPE_DINODE2DIR(ftype)						\
	((ftype & S_IFMT) == TARFS_IFREG) ? TARFS_FTYPE_REG : 			\
	((ftype & S_IFMT) == TARFS_IFDIR) ? TARFS_FTYPE_DIR :			\
	((ftype & S_IFMT) == TARFS_IFLNK) ? TARFS_FTYPE_SYM : TARFS_FTYPE_NON	

struct tarfs_direct {
	uint64_t	d_ino;
	uint16_t	d_reclen;
	uint16_t	d_namelen;
	char		d_ftype;
	char		d_name[MAXNAMLEN+1];
};
typedef struct tarfs_direct tarfs_direct_t;
#define DIRENT_SIZE(namelen)								\
	(((int)&((struct tarfs_direct *)0)->d_name +					\
	  ((namelen)+1)*sizeof(((struct tarfs_direct *)0)->d_name[0]) + 3) & ~3)

#define DIRECT_SIZE(dp)									\
	((sizeof (struct tarfs_direct) - (MAXNAMLEN+1)) + (((dp)->d_namelen+1+3) &~ 3))

struct tarfs_freeData {
	uint64_t	f_magic;
	uint64_t	f_nextblkno;
	char		f_padding[496];
};
typedef struct tarfs_freeData tarfs_freeData_t;

#define MAX_DINODE_DIRECT	17
struct tarfs_dinode {
	uint64_t	di_magic;
	uint32_t	di_mode;
	uint32_t	di_uid;
	uint32_t	di_gid;
	uint32_t	di_nlink;
	uint64_t	di_size;
	uint64_t	di_blocks;
	uint64_t	di_atime;
	uint64_t	di_ctime;
	uint64_t	di_mtime;
	uint64_t	di_header;
	uint64_t	di_nExtents;
	tarfs_dext_t	di_directExtent[MAX_DINODE_DIRECT];
	uint64_t	di_rootIndExt;
	uint32_t	di_flags;
	char		di_pading[12];
};
typedef struct tarfs_dinode tarfs_dinode_t;

struct tarfs_mnt_arg {
	struct tarfs_sblock mnt_sb;
	struct tarfs_dinode mnt_dinode;
};
#define TARFS_BLOCKS(sz) (((sz) + TAR_BLOCKSIZE -1)/TAR_BLOCKSIZE)

#endif /* _TARFS_META_H_ */
