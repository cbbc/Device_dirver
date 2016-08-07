obj-m :=buddy.o
KERN_VER = $(shell uname -r)
KERN_DIR = /lib/modules/$(KERN_VER)/build
PWD = $(shell pwd)
modules:
	$(MAKE) -C $(KERN_DIR) M=$(PWD) modules
clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers modul*
