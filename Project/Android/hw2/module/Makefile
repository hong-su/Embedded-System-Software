obj-m = dev_module.o 
CC := arm-none-linux-gnueabi-gcc -Wall

KDIR    :=/work/achroimx_kernel
PWD := $(shell pwd)

defalut : 
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	
clean : 
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *.order
	rm -rf *.symvers