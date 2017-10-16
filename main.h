#ifndef MAIN_H_
#define MAIN_H_

#define DEBUG	0
#define DSPIN	GPIO_PIN_ID_P0_0
#define SLEEP_TIME_MS 700
#define TEMP_INTERVALL 10 // tiden mellan mätningar blir SLEEP_TIME_MS*TEMP_INTERVALL

#define TEMP_MEM_BUFFER_SIZE MEMORY_FLASH_NV_EXT_END_PAGE_SIZE
#define NODE_CONFIGURATION_ADDRESS MEMORY_FLASH_NV_EXT_END_START_ADDRESS

//void mcu_init(void);
//void disconnectGPIO();

struct NodeConfig {
    uint8_t count;
    int temp;
};

//Paketet som sänds via rf24.
struct AdvertisePackage {
    uint8_t outbuf[32];
    uint8_t outbufSize;
};

#endif /* MAIN_H_ */