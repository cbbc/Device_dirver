ifneq ($(KERNELRELEASE),)
obj-m :=buddy_at24c.o
else
	KDIR :=/home/buddy/buddy_tmp/project/kernel/linux-2.6.38
all:
	     make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-
clean:
	     rm -rf *.ko *.o *.mod.o *.mod.c *.symvers modul* .at* .bud* .tm*
endif
