#ifndef _HT16K33_I2C_H
#define _HT16K33_I2C_H

#include "ch32v003fun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  

#define SYSTEM_CORE_CLOCK 48000000
#define APB_CLOCK SYSTEM_CORE_CLOCK 
#define HT16K33_I2C_ADDR 0x70  
#define HT16K33_I2C_CLKRATE 1000000 
#define HT16K33_I2C_PRERATE 2000000
#define TIMEOUT_MAX 100000

#define I2C_EVENT_MASTER_MODE_SELECT ((uint32_t)0x00030001)  /* BUSY, MSL and SB flag */
#define I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ((uint32_t)0x00070082)  /* BUSY, MSL, ADDR, TXE and TRA flags */
#define I2C_EVENT_MASTER_BYTE_TRANSMITTED ((uint32_t)0x00070084)  /* TRA, BUSY, MSL, TXE and BTF flags */

#ifndef _BV
  #define _BV(bit) (1<<(bit))
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

#define LED_ON 1
#define LED_OFF 0

#define LED_RED 1
#define LED_YELLOW 2
#define LED_GREEN 3 

#define HT16K33_BLINK_CMD 0x80
#define HT16K33_BLINK_DISPLAYON 0x01
#define HT16K33_BLINK_OFF 0
#define HT16K33_BLINK_2HZ  1
#define HT16K33_BLINK_1HZ  2
#define HT16K33_BLINK_HALFHZ  3

#define HT16K33_CMD_BRIGHTNESS 0xE0

#define SEVENSEG_DIGITS 5

static const uint8_t numbertable[] = {
	0x3F, /* 0 */
	0x06, /* 1 */
	0x5B, /* 2 */
	0x4F, /* 3 */
	0x66, /* 4 */
	0x6D, /* 5 */
	0x7D, /* 6 */
	0x07, /* 7 */
	0x7F, /* 8 */
	0x6F, /* 9 */
	0x77, /* a */
	0x7C, /* b */
	0x39, /* C */
	0x5E, /* d */
	0x79, /* E */
	0x71, /* F */
};

uint16_t displaybuffer[8] = {0}; 

int i2c_init(void) 
{ 
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
	RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

    // PC1 is SDA, 10MHz Output, alt func, open-drain
	GPIOC->CFGLR &= ~(0xf<<(4*1));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF)<<(4*1);
	
	// PC2 is SCL, 10MHz Output, alt func, open-drain
	GPIOC->CFGLR &= ~(0xf<<(4*2));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF)<<(4*2); 
  
	RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1; 

    uint32_t prerate = 2000000; 
    I2C1->CTLR2 |= (APB_CLOCK/prerate) & I2C_CTLR2_FREQ;

    uint32_t clockrate = 1000000;  
    I2C1->CKCFGR = ((APB_CLOCK/(3*clockrate))&I2C_CKCFGR_CCR) | I2C_CKCFGR_FS; 
    I2C1->CTLR1 |= I2C_CTLR1_PE; 
	I2C1->CTLR1 |= I2C_CTLR1_ACK; 

    return 1;
}

char *errstr[] =
{
	"not busy",
	"master mode",
	"transmit mode",
	"tx empty",
	"transmit complete",
};

uint8_t i2c_error(uint8_t err)
{
	printf("i2c_error - timeout waiting for %s\n\r", errstr[err]);
	
	i2c_init();

	return 1;
}

uint8_t i2c_chk_evt(uint32_t event_mask)
{
	uint32_t status = I2C1->STAR1 | (I2C1->STAR2<<16);
	return (status & event_mask) == event_mask;
}

uint8_t ht16k33_i2c_send(uint8_t addr, uint8_t *data, uint8_t sz)
{
	int32_t timeout;
	
	// wait for not busy
	timeout = TIMEOUT_MAX;
	while((I2C1->STAR2 & I2C_STAR2_BUSY) && (timeout--));
	if(timeout==-1)
		return i2c_error(0);

	// Set START condition
	I2C1->CTLR1 |= I2C_CTLR1_START;
	
	// wait for master mode select
	timeout = TIMEOUT_MAX;
	while((!i2c_chk_evt(I2C_EVENT_MASTER_MODE_SELECT)) && (timeout--));
	if(timeout==-1)
		return i2c_error(1);
	
	// send 7-bit address + write flag
	I2C1->DATAR = addr<<1;

	// wait for transmit condition
	timeout = TIMEOUT_MAX;
	while((!i2c_chk_evt(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) && (timeout--));
	if(timeout==-1)
		return i2c_error(2);

	// send data one byte at a time
	while(sz--)
	{
		// wait for TX Empty
		timeout = TIMEOUT_MAX;
		while(!(I2C1->STAR1 & I2C_STAR1_TXE) && (timeout--));
		if(timeout==-1)
			return i2c_error(3);
		
		// send command
		I2C1->DATAR = *data++;
	}

	// wait for tx complete
	timeout = TIMEOUT_MAX;
	while((!i2c_chk_evt(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) && (timeout--));
	if(timeout==-1)
		return i2c_error(4);

	// set STOP condition
	I2C1->CTLR1 |= I2C_CTLR1_STOP;
	
	// we're happy
	return 0;
}

uint8_t ht16k33_cmd_transfer(uint8_t cmd)
{
	uint8_t data[] = {cmd};
	return ht16k33_i2c_send(HT16K33_I2C_ADDR, data, 1);
} 

uint8_t ht16k33_data_transfer(uint8_t *data, uint8_t sz)
{ 
	return ht16k33_i2c_send(HT16K33_I2C_ADDR, data, sz);
}

void ht16k33_set_brightness(uint8_t b)
{
    if (b > 15) b = 15; 
	ht16k33_cmd_transfer(HT16K33_CMD_BRIGHTNESS | b);
}

void ht16k33_blink_rate(uint8_t b) { 
    if (b > 3) b = 0;  
    ht16k33_cmd_transfer(HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (b << 1));  
}

void ht16k33_display_begin(void) {
    ht16k33_cmd_transfer(0x21);
    ht16k33_blink_rate(HT16K33_BLINK_OFF);
    ht16k33_set_brightness(15);
	ht16k33_write_display();
}

void ht16k33_write_display(void) {
    uint8_t buf[17];

    buf[0] = 0x00;

    for(uint8_t i=0; i<8; i++) {
        buf[(2*i)+1] = displaybuffer[i] & 0xFF;    
        buf[(2*i)+2] = displaybuffer[i] >> 8;    
    } 

    ht16k33_data_transfer(buf, sizeof(buf));
}

void ht16k33_clear_buffer(void) {
    for(uint8_t i = 0; i < 8; i++) {
        displaybuffer[i] = 0;
    }
}

void ht16k33_write_digit_num(uint8_t d, uint8_t num, uint8_t dot) { 
  displaybuffer[d] = numbertable[num] | (dot << 7);
}

uint8_t waiting_float_dot = 0;

char *dtostrf (float val, signed char width, unsigned char prec, char *sout) {
  char fmt[20];
  snprintf(fmt, 20, "%%%d.%df", width, prec);
  snprintf(sout, 20, fmt, val);
  return sout;
}

void ht16k33_print_float(char usd[]) {  
    char string_usd[10], string_usd_decimal[10]; 

	uint8_t max_i = 0, len = strlen(usd);
	char c = usd[0];
	while(c != '.' && max_i != len) { 
		max_i++;
		c = usd[max_i];
	}

	mini_snprintf(string_usd, max_i+1, "%s", usd); 
	mini_snprintf(string_usd_decimal, len-max_i, "%s", (usd + max_i+ 1)); 

	max_i = 0;
	for(uint8_t i = 0; i < strlen(string_usd); i++) {
		max_i++;
		if(i+1 == strlen(string_usd)) {
			ht16k33_write_digit_num(i, (uint8_t)(string_usd[i] - '0'), 1);
		} else {
			ht16k33_write_digit_num(i, (uint8_t)(string_usd[i] - '0'), 0);
		}
	} 

	for(uint8_t i = 0; i < strlen(string_usd_decimal) && i < 7-strlen(string_usd);  i++) { 
		max_i++;
		ht16k33_write_digit_num(i+strlen(string_usd), (uint8_t)(string_usd_decimal[i] - '0'), 0); 
	}
	
	for(uint8_t i = max_i; i < 6; i++) {
        displaybuffer[i] = 0;
    }

 	ht16k33_write_display();
}  

#endif