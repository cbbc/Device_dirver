ifneq ($(KERNELRELEASE),)
obj-m :=buddy.o
else
	KDIR :=/home/buddy/buddy_tmp/project/kernel/linux-2.6.38
all:
	     make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-
clean:
	     rm -f *.ko *.o *.mod.o *.mod.c *.symvers modul*
endif
