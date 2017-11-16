#include "btle.h"
#include "main.h"
#include "ds18b20.h"
#include "rtc2.h"
#include "pwr_clk_mgmt.h"

#if DEBUG
#define EN_UART 1                  // use UART for debugging
#define UARTTXPIN GPIO_PIN_ID_P0_3 // P0.3 - UART TX
#define UARTRXPIN GPIO_PIN_ID_P0_4 // P0.4	- UART RX
#else
#define EN_UART 0 // use UART for debugging
#endif

#if EN_UART
#include "uart.h"
#endif

int temp;
uint8_t ret;

struct NodeConfig nc;
struct AdvertisePackage package; //packetet att sända

uint8_t beacon[17] = {
    0x02, // size
    0x01, // type, LE General Discoverable Mode
    0x05, // data ??

    0x05, // size
    0x09, // complete name
    0x62, 0x6f, 0x6a, 0x33,

    0x07, //size
    0x16, //Service data
    0x09, 0x18, //UUID in reverse 0x1809 Health Thermometer
    0x00, 0x00, 0x00, 0xff
    //0xa4, 0x08, 0x00, 0xfe //temperature in nrf format
};

void setup() {

    interrupts();

    CLKLFCTRL = 1;
    //rtc2_configure(RTC2_CONFIG_OPTION_COMPARE_MODE_0_RESET_AT_IRQ ,6130); //187ms, 65535=2 сек, 32767=1 сек
    rtc2_configure(RTC2_CONFIG_OPTION_COMPARE_MODE_0_RESET_AT_IRQ, RTC2_SLEEP); //65535=2 сек, 32767=1 сек
    rtc2_run();
    pwr_clk_mgmt_wakeup_sources_configure(PWR_CLK_MGMT_CLKLF_CONFIG_OPTION_CLK_SRC_XOSC32K);


#if EN_UART

    gpio_pin_configure(UARTTXPIN,
            GPIO_PIN_CONFIG_OPTION_DIR_OUTPUT |
            GPIO_PIN_CONFIG_OPTION_OUTPUT_VAL_SET |
            GPIO_PIN_CONFIG_OPTION_PIN_MODE_OUTPUT_BUFFER_NORMAL_DRIVE_STRENGTH);

    gpio_pin_configure(UARTRXPIN,
            GPIO_PIN_CONFIG_OPTION_DIR_INPUT |
            GPIO_PIN_CONFIG_OPTION_PIN_MODE_INPUT_BUFFER_ON_NO_RESISTORS);

    uart_configure_8_n_1_19200();
    delay_ms(100);

#endif

    nc.count = TEMP_COUNTER;
}

void loop() {

    if (nc.count >= TEMP_COUNTER) {

#if !DEBUG
        mcu_init();
#endif
        ret = ds18b20_read(&temp);

#if DEBUG
        printf("Temperature: %d, status=%d .\n", temp, ret);
#endif
        if (nc.temp != temp) {

#if DEBUG
            printf("Uppdaterar temperastur i paketet. gammal temp: %d\n,nc.temp");
#endif
            nc.temp = temp;

            //Sätter temperaturen i paketet
            beacon[13] = nc.temp & 0xff;
            beacon[14] = (nc.temp >> 8) & 0xff;
            createAdvertisePackage(&beacon, sizeof (beacon), &(package.outbuf), &(package.outbufSize));

        } else {
#if DEBUG
            printf("Samma temperatur\n");
#endif
        }

        nc.count = 0;
    }

    btleBegin();
    //RF24 commands
    stopListening();
    write(package.outbuf, package.outbufSize, false);
    powerDown();

#if DEBUG
    printf("Sending package: ");

    for (i = 0; i < package.outbufSize; i++) {
        printf("0x%x, ", package.outbuf[i]);
    }

    printf("\n");
#endif

#if !DEBUG
    disconnectGPIO();
#endif
    nc.count++;
    pwr_clk_mgmt_enter_pwr_mode_register_ret();

}

void mcu_init(void) {
    // XOSC32K input
    P0DIR = 0x00;
    P1DIR = 0x00;
    P2DIR = 0x00;
    P3DIR = 0x00;

    P0 = 0x00;
    P1 = 0x00;
    P2 = 0x00;
    P3 = 0x00;

    // Open latch
    OPMCON = 0x00;
}

void disconnectGPIO() {

    uint8_t i;

    P0DIR = 0xFF;
    P1DIR = 0xFF;
    P2DIR = 0xFF;
    P3DIR = 0xFF;

    for (i = 0; i < 8; i++) {
        P0CON = 0x70 | i;
        P1CON = 0x70 | i;
        P2CON = 0x70 | i;
        P3CON = 0x70 | i;
    }
    OPMCON |= 0x02;
}
