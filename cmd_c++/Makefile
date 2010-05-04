
TARGETS=mkfs.tarfs tarfsmount tarfscheck detachtarfs
OBJS=parser.o file.o util.o fsmaker.o space.o inode.o inode_factory.o dir.o fsreader.o
SRCS=${OBJS:.o=.cc}
CFLAGS=-g -Wall


INC=-I../include -I.

.cc.o:
	g++ $(INC) $(CFLAGS) -o $@ -c $<

all: $(TARGETS)

mkfs.tarfs: $(OBJS)
	g++ $(INC) $(CFLAGS) -o mkfs.tarfs mkfs_tarfs.cc $(OBJS)

tarfsmount: $(OBJS)
	g++ $(INC) $(CFLAGS) -o tarfsmount tarfsmount.cc $(OBJS)

tarfscheck: $(OBJS)
	g++ $(INC) $(CFLAGS) -o tarfscheck tarfscheck.cc $(OBJS)

detachtarfs: $(OBJS)
	g++ $(INC) $(CFLAGS) -o detachtarfs detachtarfs.cc $(OBJS)


install: $(TARGETS)
	cp $(TARGETS) /sbin/

uninstall:
	rm -f /sbin/mkfs.tarfs
	rm -f /sbin/tarfsmount
	rm -f /sbin/mount.tarfs
	rm -f /sbin/detachtarfs

clean:
	rm -f $(OBJS) $(TARGETS)
