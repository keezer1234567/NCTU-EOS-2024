ccflags-y := -std=gnu99 -Wno-declaration-after-statement
obj-m += led_driver.o
obj-m += seg_driver.o

KDIR = /lib/modules/$(shell uname -r)/build
all:
	make -C $(KDIR) M=$(shell pwd) modules
clean:
	make -C $(KDIR) M=$(shell pwd) clean