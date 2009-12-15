
OBJS=tarfs_common.o
SRCS=${OBJS:.o=.c}

TARGET_1=mkfs.tarfs
TARGET_2=checkTarfs
TARGET_3=tarfsmount

INC=-I../include

.c.o:
	gcc -g $(INC) -Wall -o $@ -std=c99 -c $<

all: $(OBJS) mkfs_tar.c checkTarfs.c
	gcc -g -o $(TARGET_1) $(INC) -Wall mkfs_tar.c $(OBJS)
	gcc -g -o $(TARGET_2) $(INC) -Wall checkTarfs.c $(OBJS)
	gcc -g -o $(TARGET_3) $(INC) -Wall tarfsmount.c $(OBJS)
install:
	cp $(TARGET_1) /sbin/
	cp $(TARGET_3) /sbin/
	cp mount.tarfs /sbin/

clean:
	rm -f $(TARGET_1) $(TARGET_2) $(TARGET_3) $(OBJS)
