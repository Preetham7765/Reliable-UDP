CC           = gcc
CFLAGS       = -O2 -pthread
#CFLAGS       = -O0 -g

#set the following variables for custom libraries and/or other objects
EXTOBJS      =
LIBS         = -lm
LIBPATHS     = 
INCLUDEPATHS = 

PROGRAM     = netster
OBJS        = $(PROGRAM).o a2.o queue.o# a3.o a4.o

$(PROGRAM):$(OBJS)
	$(CC) -o $(PROGRAM) $(LIBPATHS) $(CFLAGS) $(OBJS) $(EXTOBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(TUNEFLAGS) $(INCLUDEPATHS) -c $<

clean:
	rm -f $(OBJS) $(PROGRAM) *~