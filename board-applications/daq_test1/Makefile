# Makefile to build user mode data acquisition application using the 
# dt78xx USB module
##############################################################################

# Name of executable
EXAMPLE = daq-test1

# IMPORTANT : path to gcc cross compiler for the ARM
CROSS-TOOLS-DIR = /usr/local/gcc-linaro-arm-linux-gnueabihf-4.7-2013.03-20130313_linux
ARM-GCC = $(CROSS-TOOLS-DIR)/bin/arm-linux-gnueabihf-gcc
#ARM-INC = $(CROSS-TOOLS-DIR)/arm-linux-gnueabihf/libc/usr/include/arm-linux-gnueabi

CFLAGS= -I$(ARM-INC) -I. -DDEBUG
DEPS = 
OBJ = main.o xcrc32.o

%.o: %.c $(DEPS)  Makefile
	$(ARM-GCC) $(CFLAGS) -c -o $@ $< 

$(EXAMPLE): $(OBJ)
	$(ARM-GCC) $(CFLAGS) -o $@ $^ -lpthread

clean:
	rm -rf *.o $(EXAMPLE)
