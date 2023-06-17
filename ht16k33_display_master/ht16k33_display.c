#include "ch32v003fun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ht16k33_i2c.h"
#include "onewire.h"

int main()
{ 
	SystemInit48HSI();
 
	SetupUART( UART_BRR );
	Delay_Ms( 100 ); 

	printf("Ht16K33 DS18B20 Temperature Display Example\r\n");

	onewire_init(); 
  
	if(i2c_init()) {  
		printf("I2C Ok!...\r\n"); 

        ht16k33_display_begin();

		while(1) {  
			uint16_t t = onewire_read_temperature();
			char buf[20];
			mini_snprintf(buf, 20, "%u.%lu", t >> 4, ((t & 0x0F)*100)>>4);

			printf("Temp: %s\r\n", buf);
 			ht16k33_print_float(buf); 

			Delay_Ms(1000);
		} 
	}
	else {
        printf("i2c device not found\n\r");
    }  
}
