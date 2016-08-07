ifneq ($(KERNELRELEASE),)
obj-m :=tank.o
else
	KERNEL_VERSION := $(shell uname -r)
	KDIR :=/lib/modules/$(KERNEL_VERSION)/build
all:
	     make -C $(KDIR) M=$(PWD) modules
clean:
	     rm -rf *.ko *.o *.mod.o *.mod.c *.symvers modul* .budd* .tm*
endif
