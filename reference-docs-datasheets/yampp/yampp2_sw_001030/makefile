# Simple Makefile
# Volker Oth (c) 1999

include $(AVR)/include/make1

########### change this lines according to your project ##################

#put the name of the target mcu here (at90s8515, at90s8535, attiny22, atmega603 etc.)
    MCU = at90s8515

#put the name of the target file here (without extension)
	TRG	= yampp2

#put your C sourcefiles here 
	SRC	= rec80.c mem.c dac.c mas.c i2c.c spi.c lcd.c fat.c ata_if.c delay.c uart.c $(TRG).c

#put additional assembler source file here
	ASRC    =

#additional libraries and object files to link
	LIB = $(AVR)/avr/lib/libc.a

#additional includes to compile
	INC	= 

#compiler flags
	CPFLAGS	=  -g -O3 -Wall -Wstrict-prototypes -Wa,-ahlms=$(<:.c=.lst)

#linker flags
	LDFLAGS = -Wl,-Map=$(TRG).map,--cref

########### you should not need to change the following line #############
include $(AVR)/include/make2
		  
###### dependecies, add any dependencies you need here ###################

rec80.o		: rec80.c rec80.h
mem.o		: mem.c mem.h
dac.o		: dac.c dac.h
mas.o		: mas.c mas.h
i2c.o		: i2c.c i2c.h
spi.o		: spi.c spi.h
lcd.o		: lcd.c lcd.h
ata_if.o	: ata_if.c ata_if.h
fat.o		: fat.c fat.h
uart.o 		: uart.c uart.h
delay.o		: delay.c delay.h
$(TRG).o	: $(TRG).c
