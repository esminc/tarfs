
OBJS=parser.o fsmaker.o space.o inode.o dir.o
SRCS=${OBJS:.o=.cc}


INC=-I../include -I.

.cc.o:
	g++ -g $(INC) -Wall -o $@ -c $<

all: $(OBJS)
	g++ -g $(INC) -Wall -o main main.cc $(OBJS)

clean:
	rm -f $(OBJS) main
