#ifndef MAIN_H_
#define MAIN_H_

#define DEBUG 1
#define TEMP_COUNTER 5
#define RTC2_SLEEP 65535 //65535 = 2 sek.

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