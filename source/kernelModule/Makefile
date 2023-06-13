obj-m += IPCmsgKernMod.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

replug:
	rm -f /dev/IPCmsgKern
	/usr/sbin/rmmod -f IPCmsgKern
	/usr/sbin/insmod IPCmsgKernMod.ko
	mknod /dev/IPCmsgKern c 244 0
	chmod 666 /dev/IPCmsgKern

plug:
	/usr/sbin/insmod IPCmsgKernMod.ko
	chmod 666 /dev/IPCmsgKern

unplug:
	/usr/sbin/rmmod -f IPCmsgKernMod
