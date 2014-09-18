obj-m += inputdummy.o
KVERSION := $(shell uname -r)

all:
	make -C /lib/modules/${KVERSION}/build M=$(PWD) modules

clean:
	make -C /lib/modules/${KVERSION}/build M=$(PWD) clean

install:
	install -m 0644 *.ko /lib/modules/${KVERSION}/kernel/drivers/input
	install -m 0644 *.ko /lib/modules/${KVERSION}/updates/dkms/inputdummy.ko
	depmod -a
