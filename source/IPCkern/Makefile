
all:
	mkdir -p Lib
	$(CC) $(CFLAGS)  -fPIC -c IPCmsgKern.c -o Lib/IPCmsgKern.o
	$(CC) $(LDFLAGS) -shared -o Lib/libIPCmsgKern.so Lib/IPCmsgKern.o

cleanBHawC:
	rm -r Lib
