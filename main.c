#include "wiring.h"
#include "btle.h"
#include "main.h"
#include "ds18b20.h"
#include "memory.h"


#define TEMP_MEM_BUFFER_SIZE MEMORY_FLASH_NV_EXT_END_PAGE_SIZE
#define NODE_CONFIGURATION_ADDRESS MEMORY_FLASH_NV_EXT_END_START_ADDRESS



#if DEBUG
#define EN_UART			1	// use UART for debugging
#define UARTTXPIN		GPIO_PIN_ID_P0_3		// P0.3 - UART TX
#define UARTRXPIN		GPIO_PIN_ID_P0_4		// P0.4	- UART RX
#else
#define EN_UART			0	// use UART for debugging
#endif


#if EN_UART
#include "uart.h"
#endif
        
uint8_t beacon[17] =
{
  0x02,			// size
  0x01,			// type, LE General Discoverable Mode
  0x05,			// data ??

  0x05,			// size
  0x09,   // complete name
  0x62,0x6f,0x6a,0x33,

  0x07,  //size
  0x16,  //Service data
  0x09, 0x18,  //UUID in reverse 0x1809 Health Thermometer
  0x00, 0x00, 0x00, 0xff
  //0xa4, 0x08, 0x00, 0xfe //temperature in nrf format
 
};


int temp;
uint8_t ret;
unsigned long time;
uint8_t tmp;

struct NodeConfig
{
	uint8_t count; 
	int temp;
};

struct NodeConfig nc;
uint8_t temp_buf[TEMP_MEM_BUFFER_SIZE];

void setup(){
	interrupts();
	memory_flash_read_bytes(NODE_CONFIGURATION_ADDRESS,sizeof(nc),(uint8_t*)nc);

#if EN_UART
        
	gpio_pin_configure(UARTTXPIN,
                                           GPIO_PIN_CONFIG_OPTION_DIR_OUTPUT |
                                           GPIO_PIN_CONFIG_OPTION_OUTPUT_VAL_SET |
                                           GPIO_PIN_CONFIG_OPTION_PIN_MODE_OUTPUT_BUFFER_NORMAL_DRIVE_STRENGTH);

    	gpio_pin_configure(UARTRXPIN,
                                       GPIO_PIN_CONFIG_OPTION_DIR_INPUT |
                                       GPIO_PIN_CONFIG_OPTION_PIN_MODE_INPUT_BUFFER_ON_NO_RESISTORS);

    	uart_configure_8_n_1_19200();
#endif

								// turn interrupts on
        
        if(nc.count == 0 || nc.count == TEMP_INTERVALL){
	
            ret = ds18b20_read(&temp);
#if DEBUG
            printf("Temperature: %d, status=%d .\n", temp, ret);
#endif
            nc.temp = temp;
            nc.count = 0;
		
	}
       
        beacon[13] = nc.temp & 0xff;
        beacon[14] =  (nc.temp >> 8) & 0xff;

	btleBegin();	
        btleAdvertise(&beacon, sizeof(beacon));		// start advertising the packet above
        powerDown();
        
	nc.count++;
	memory_flash_write_bytes_smart(NODE_CONFIGURATION_ADDRESS,sizeof(nc),(uint8_t*)nc,temp_buf);
	
	watchdogRun(SLEEP_TIME_MS);						// start watchdog and reset at 700 ms
	sleep(MEMORY_TIMER_OFF);				// switch to lowest mode that can wakeup from wdt
        
}

void loop(){
	
}
