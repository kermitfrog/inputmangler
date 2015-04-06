obj-m += inputdummy.o
KVERSION := $(shell uname -r)

all:
	make -C /lib/modules/${KVERSION}/build M=$(PWD) modules

clean:
	make -C /lib/modules/${KVERSION}/build M=$(PWD) clean

install:
	install -m 0644 inputdummy.ko /lib/modules/${KVERSION}/kernel/drivers/input
	install -m 0644 inputdummy.ko /lib/modules/${KVERSION}/updates/dkms/inputdummy.ko
	install -m 0644 98-inputdummy.rules /etc/udev/rules.d/98-inputdummy.rules
	depmod -a
