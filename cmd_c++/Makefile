
OBJS=parser.o file.o util.o fsmaker.o space.o inode.o inode_factory.o dir.o 
SRCS=${OBJS:.o=.cc}
CFLAGS=-g -Wall


INC=-I../include -I.

.cc.o:
	g++ $(INC) $(CFLAGS) -o $@ -c $<

all: $(OBJS)
	g++ $(INC) $(CFLAGS) -o mkfs.tar mkfs_tar.cc $(OBJS)

clean:
	rm -f $(OBJS) mkfs.tar
