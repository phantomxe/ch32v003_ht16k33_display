#ifndef _ONEWIRE_H
#define _ONEWIRE_H

#include "ch32v003fun.h"
#include "ch32v003_GPIO_branchless.h"
#include <stdio.h>
 
#define ONEWIRE_READ_ROM	0x33
#define ONEWIRE_MATCH_ROM	0x55
#define ONEWIRE_SKIP_ROM	0xCC
#define ONEWIRE_CONVERT_T   0x44 
#define ONEWIRE_READ_SC_PAD 0XBE

#define ONEWIRE_PIN_INPUT()		(GPIO_pinMode(GPIO_port_C, 0, GPIO_pinMode_I_floating, GPIO_Speed_10MHz))
#define ONEWIRE_PIN_OUTPUT()    (GPIO_pinMode(GPIO_port_C, 0, GPIO_pinMode_O_openDrain, GPIO_Speed_10MHz))
#define ONEWIRE_PIN_LOW()		(GPIO_digitalWrite(GPIO_port_C, 0, low))
#define ONEWIRE_PIN_HIGH()		(GPIO_digitalWrite(GPIO_port_C, 0, high))
#define ONEWIRE_PIN_READ()		(GPIO_digitalRead(GPIO_port_C, 0))


union foo {
  uint8_t DataBytes[9];
  uint16_t DataWords[4];
} foo;

void onewire_init(void) 
{
    GPIO_portEnable(GPIO_port_C);
    ONEWIRE_PIN_OUTPUT();

	printf("onewire init\r\n");
}

void onewire_drive_low_release(int low_time, int high_time) {
	ONEWIRE_PIN_OUTPUT();
	ONEWIRE_PIN_LOW();
	Delay_Us(low_time);
	ONEWIRE_PIN_INPUT();
	Delay_Us(high_time);
}

uint8_t onewire_reset(void) 
{
    uint8_t data = 1;
	onewire_drive_low_release(480, 70); 
	data = ONEWIRE_PIN_READ();
	Delay_Us(410);
	return data;                         // 0 = device present
} 

uint8_t onewire_write(uint8_t data)
{
    int del;
	for (int i = 0; i<8; i++) {
		if ((data & 1) == 1) del = 6; else del = 60;
		onewire_drive_low_release(del, 70 - del);
		data = data >> 1;
	}
}

uint8_t onewire_read(void)
{
    uint8_t data = 0;
	for (int i = 0; i<8; i++) {
		onewire_drive_low_release(6, 9);
		ONEWIRE_PIN_INPUT();
		data = data | ONEWIRE_PIN_READ()<<i;
		Delay_Us(55);
	}
	return data;
}

void onewire_read_bytes(uint8_t bytes) { 
	for (uint8_t i = 0; i < bytes; i++) {
		foo.DataBytes[i] = onewire_read();
	}
}

uint8_t crc(uint8_t bytes) {
  uint8_t crc = 0;
  for (int j = 0; j < bytes; j++) {
    crc = crc ^ foo.DataBytes[j];
    for (int i=0; i<8; i++) crc = crc>>1 ^ ((crc & 1) ? 0x8c : 0);
  }
  return crc;
}

uint16_t onewire_read_temperature(void) { 
  if (onewire_reset() != 0) { 
	printf("Onewire error\r\n");
    return 0;
  } else {
    onewire_write(ONEWIRE_SKIP_ROM); 
    onewire_write(ONEWIRE_CONVERT_T); 
	ONEWIRE_PIN_INPUT();
	uint8_t data;
    while ((data = onewire_read()) != 0xFF) {  }; 
    onewire_reset(); 
    onewire_write(ONEWIRE_SKIP_ROM);
    onewire_write(ONEWIRE_READ_SC_PAD);
	ONEWIRE_PIN_INPUT();
    onewire_read_bytes(9);  
    if (crc(9) == 0) { 
		printf("complete\r\n");
		return foo.DataWords[0];
    } else {
		printf("CRC error\r\n"); 
		return 0;
	}
  } 
}

#endif