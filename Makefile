# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
	obj-m := prime_finder_driver.o
# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif


clean:
	rm prime_finder_driver.ko .prime_finder_driver.ko.cmd prime_finder_driver.mod prime_finder_driver.mod.c \
		 .prime_finder_driver.mod.cmd prime_finder_driver.mod.o .prime_finder_driver.mod.o.cmd \
		 prime_finder_driver.o .prime_finder_driver.o.cmd Module.symvers modules.order