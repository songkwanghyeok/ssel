obj-m += main.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	rmmod main
	insmod main.ko

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
