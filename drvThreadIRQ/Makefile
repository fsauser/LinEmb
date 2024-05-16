TARGET_MODULE:=drvThreadIRQ

CFLAGS := -DDEBUG   # enable printing of debugging messages

# If we running by kernel building system
ifneq ($(KERNELRELEASE),)
  obj-m := $(TARGET_MODULE).o # add files 
else
  PWD:=$(shell pwd)

all : 
# run kernel build system to make module
  $(MAKE) CFLAGS -C '$(LINUX_DIR)' M='$(PWD)' modules	

clean:
# run kernel build system to cleanup in current directory
  $(MAKE) -C '$(LINUX_DIR)' M='$(PWD)' clean

endif


