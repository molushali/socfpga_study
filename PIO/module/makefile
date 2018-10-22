ifeq ($(KERNELRELEASE),)

#KERNELDIR ?= /your/target/source/directory/
KERNELDIR ?= ~/work/linux-socfpga
PWD := $(shell pwd)


modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions  Module* module*

.PHONY: modules modules_install clean

else
    obj-m := button.o
endif
