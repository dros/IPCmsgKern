
all:
	mkdir -p Lib
	$(CC) $(CFLAGS)  -fPIC -c IPCmsgKernTest.c -o Lib/IPCmsgKernTest.o
	$(CC) $(LDFLAGS) -o Lib/IPCmsgKernTest Lib/IPCmsgKernTest.o ../IPCkern/Lib/IPCmsgKern.o

clean:
	rm -r Lib
