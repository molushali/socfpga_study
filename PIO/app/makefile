#

TARGET = button_test

#

CROSS_COMPILE = arm-linux-gnueabihf-

#CFLAGS = -static -g -Wall  -I ~/work/linux-socfpga/usr/hwlib/include 

LDFLAGS =  -g -Wall  

CC = $(CROSS_COMPILE)gcc

ARCH= arm

#LDFLAGS =  -g -Wall  -Iteraisc_pcie_qsys.so -ldl

#-ldl must be placed after the file calling lpxxxx funciton





build: $(TARGET)



#-lmpeg2 --> link libmpeg2.a (lib___.a)

$(TARGET): button_test.o 

	$(CC) $(LDFLAGS)   $^ -o $@  

#	$(CC) $(LDFLAGS)  $^ -o $@  -ldl -lmpeg2  -lmpeg2convert -lpthread

%.o : %.c

	$(CC) $(CFLAGS) -c $< -o $@



.PHONY: clean

clean:

	rm -f $(TARGET) *.a *.o *~ 
        #rm -f $(TARGET) *.a *.o *~ *.bmp
