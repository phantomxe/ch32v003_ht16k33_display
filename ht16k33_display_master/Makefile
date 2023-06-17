all : flash

TARGET:=ht16k33_display
CH32V003FUN:=../ch32v003fun/ch32v003fun
MINICHLINK=../ch32v003fun/minichlink

CFLAGS+=-DSTDOUT_UART
CFLAGS+=-DTINYVECTOR 

include ../ch32v003fun/ch32v003fun/ch32v003fun.mk

flash : cv_flash
clean : cv_clean