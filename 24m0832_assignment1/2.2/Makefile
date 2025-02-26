KDIR = /lib/modules/`uname -r`/build

TARGETS = control_station soldier

SOURCES = $(TARGETS:=.c)

MODULE = chardev.ko
MODULE_SRC = chardev.c

all: module $(TARGETS)

$(TARGETS): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $@.c

module:
	make -C $(KDIR) M=`pwd`

clean:
	rm -f $(TARGETS)
	make -C $(KDIR) M=`pwd` clean
