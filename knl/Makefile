#
# tarfs 用 make ファイル
#

# obj-mには作成したいモジュール名を指定する。
# 拡張子は*.koではなく*.oなので注意。
# ここで指定したファイルが最終的にモジュール(*.ko)に変換される。
obj-m		:= tarfs.o

# モジュールを構成するオブジェクトを指定する。
# TARGET名としては、<モジュール名>-objs
tarfs-objs	:= super.o \
		   inode.o \
		   dir.o \
		   file.o \
		   namei.o \
	   	   tarfs_common.o \
		   tarfs_common_lnx.o

# モジュールを構成するファイルを指定する。
SRC		:= Makefile \
		super.c \
		inode.c \
		dir.c \
		file.c \
		namei.c \
	   	tarfs_common.c \
	   	tarfs_common_lnx.c

all:$(SRC)
	make -C /lib/modules/`uname -r`/build M=`pwd` modules

clean:
	make -C /lib/modules/`uname -r`/build M=`pwd` clean
	$(RM) *.o
	$(RM) Module.symvers

