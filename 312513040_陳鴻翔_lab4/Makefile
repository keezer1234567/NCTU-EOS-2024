ccflags-y := -std=gnu99 -Wno-declaration-after-statement
obj-m := my_dev.o

KERNEL_VERSION := $(shell uname -r)

all:
	$(MAKE) -C /lib/modules/$(KERNEL_VERSION)/build M=$(shell pwd) modules

clean:
	$(MAKE) -C /lib/modules/$(KERNEL_VERSION)/build M=$(shell pwd) clean

