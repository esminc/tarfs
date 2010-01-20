
OBJS=parser.o file.o util.o fsmaker.o space.o inode.o inode_factory.o dir.o fsreader.o
SRCS=${OBJS:.o=.cc}
CFLAGS=-g -Wall


INC=-I../include -I.

.cc.o:
	g++ $(INC) $(CFLAGS) -o $@ -c $<

all: $(OBJS)
	g++ $(INC) $(CFLAGS) -o mkfs.tarfs mkfs_tarfs.cc $(OBJS)
	g++ $(INC) $(CFLAGS) -o tarfsmount tarfsmount.cc $(OBJS)
	g++ $(INC) $(CFLAGS) -o tarfscheck tarfscheck.cc $(OBJS)

install:
	cp mkfs.tarfs /sbin/
	cp tarfsmount /sbin/
	cp mount.tarfs /sbin/

clean:
	rm -f $(OBJS) mkfs.tarfs tarfsmount tarfscheck
