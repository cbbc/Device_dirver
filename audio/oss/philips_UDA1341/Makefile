
include ./Rules.mak
dirs = basic microwindows minigui

all: 
	for i in $(dirs) ; do  make -C $$i || exit $? ; done
clean:
	-for i in $(dirs) ; do \
		[ ! -d $$i ] || make -C $$i clean; \
	done
	rm -f bin/*

install:
	-for i in $(dirs) ; do \
		[ ! -d $$i ] || make -C $$i install; \
	done
