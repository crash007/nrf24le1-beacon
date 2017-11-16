/*
 * Copyright (C) 2015 Justin Decker (original 2013 Florian Echtler <floe@butterbrot.org>)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
*/
#ifndef BTLE_H_
#define BTLE_H_
#include "rf24.h"

bool advertise( void* buf, uint8_t buflen );
bool createAdvertisePackage( void* buf, uint8_t buflen, void* out, uint8_t* outbufSize );
void crc( uint8_t len, uint8_t* dst );
void whiten( uint8_t len );
void btleBegin();
void btleHopChannel();
bool btleAdvertise( void* buf, uint8_t buflen );
bool btleListen(int timeout);

// advertisement PDU
struct btle_adv_pdu {

	// packet header
	uint8_t pdu_type; // PDU type
	uint8_t pl_size;  // payload size

	// MAC address
	uint8_t mac[6];

	uint8_t payload[24];
} buffer;


// payload chunk in advertisement PDU payload
struct btle_pdu_chunk {
	uint8_t size;
	uint8_t type;
	uint8_t data[];
};

uint8_t current;   // current channel index

const uint8_t channel[3]   = {37,38,39};  // logical BTLE channel number (37-39)
const uint8_t frequency[3] = { 2,26,80};  // physical frequency (2400+x MHz)

// This is a rather convoluted hack to extract the month number from the build date in
// the __DATE__ macro using a small hash function + lookup table. Since all inputs are
// const, this can be fully resolved by the compiler and saves over 200 bytes of code.
#define month(m) month_lookup[ (( ((( (m[0] % 24) * 13) + m[1]) % 24) * 13) + m[2]) % 24 ]
const uint8_t month_lookup[24] = { 0,6,0,4,0,1,0,17,0,8,0,0,3,0,0,0,18,2,16,5,9,0,1,7 };

// change buffer contents to "wire bit order"
void swapbuf( uint8_t len ) {

	uint8_t* buf = (uint8_t*)&buffer;

	while (len--) {

		uint8_t a = *buf;
		uint8_t v = 0;

		if (a & 0x80) v |= 0x01;
		if (a & 0x40) v |= 0x02;
		if (a & 0x20) v |= 0x04;
		if (a & 0x10) v |= 0x08;
		if (a & 0x08) v |= 0x10;
		if (a & 0x04) v |= 0x20;
		if (a & 0x02) v |= 0x40;
		if (a & 0x01) v |= 0x80;

		*(buf++) = v;
	}
}

void btleBegin(){
	current = 0;
	radioBegin();

	// set standard parameters
	setAutoAck(false);
	setDataRate(RF24_1MBPS);
	setCRCLength(RF24_CRC_DISABLED);
	setChannel( frequency[current] );
	setRetries(0,0);
	setPALevel(RF24_PA_MAX);

	// set advertisement address: 0x8E89BED6 (bit-reversed -> 0x6B7D9171)
	setAddressWidth(4);
	openReadingPipe(0, 0, 0x6B, 0x7D, 0x91, 0x71);
	openWritingPipe(  0, 0x6B, 0x7D, 0x91, 0x71);

	powerUp();
}

// set the current channel (from 37 to 39)
void btlesetChannel( uint8_t num ) {
	current = min(2,max(0,num-37));
	setChannel( frequency[current] );
}

// hop to the next channel
void btleHopChannel() {
	current++;
	if (current >= sizeof(channel)) current = 0;
	setChannel( frequency[current] );
}

// Broadcast an advertisement packet with optional payload
// Data type will be 0xFF (Manufacturer Specific Data)
bool btleAdvertise( void* buf, uint8_t buflen ) {
	return advertise(buf, buflen);
}

// Broadcast an advertisement packet with a specific data type
// Standardized data types can be seen here: 
// https://www.bluetooth.org/en-us/specification/assigned-numbers/generic-access-profile
bool advertise( void* buf, uint8_t buflen ) {
	
    uint8_t outbuf[32];
        uint8_t outbufSize=0;
        
        createAdvertisePackage( buf, buflen, &outbuf, &outbufSize);
	// flush buffers and send
	stopListening();
	write( outbuf, outbufSize, false );

	return true;
}

bool createAdvertisePackage( void* buf, uint8_t buflen, void* out, uint8_t* outbufSize ) {
    // name & total payload size
        uint8_t pls = 0;
	uint8_t i;
	uint8_t* outbuf;

	// insert pseudo-random MAC address
	buffer.mac[0] = 0x06; //((__TIME__[6]-0x30) << 4) | (__TIME__[7]-0x30);
	buffer.mac[1] = 0x05; //((__TIME__[3]-0x30) << 4) | (__TIME__[4]-0x30);
	buffer.mac[2] = 0x04; //((__TIME__[0]-0x30) << 4) | (__TIME__[1]-0x30);
	buffer.mac[3] = 0x03; //((__DATE__[4]-0x30) << 4) | (__DATE__[5]-0x30);
	buffer.mac[4] = 0x02; //month(__DATE__);
	buffer.mac[5] = 0x02;
	//buffer.mac[5] = ((__DATE__[9]-0x30) << 4) | (__DATE__[10]-0x30);

	for (i = 0; i < buflen; i++)
		buffer.payload[i] = ((uint8_t*)buf)[i];
	pls += buflen;

	// total payload size must be 21 bytes or less
	if (pls > 21)
		return false;

	// assemble header
	buffer.pdu_type = 0x42;    // PDU type: ADV_NONCONN_IND, TX address is random
	buffer.pl_size = pls+6;    // set final payload size in header incl. MAC

	// calculate CRC over header+MAC+payload, append after payload
	outbuf = (uint8_t*)&buffer;
	crc( pls+8, outbuf+pls+8);

	// whiten header+MAC+payload+CRC, swap bit order
	whiten( pls+11 );
	swapbuf( pls+11 );
        
        //kopiera array
        for(i=0; i<pls+11; i++){
            ((uint8_t *) out)[i] = ((uint8_t*)outbuf)[i];
        }  
        *outbufSize = pls+11;
        return true;
}

// see BT Core Spec 4.0, Section 6.B.3.1.1
void crc( uint8_t len, uint8_t* dst ) {

	uint8_t* buf = (uint8_t*)&buffer;
	uint8_t i;

	// initialize 24-bit shift register in "wire bit order"
	// dst[0] = bits 23-16, dst[1] = bits 15-8, dst[2] = bits 7-0
	dst[0] = 0xAA;
	dst[1] = 0xAA;
	dst[2] = 0xAA;

	while (len--) {

		uint8_t d = *(buf++);

		for (i = 1; i; i <<= 1, d >>= 1) {

			// save bit 23 (highest-value), left-shift the entire register by one
			uint8_t t = dst[0] & 0x01;         dst[0] >>= 1;
			if (dst[1] & 0x01) dst[0] |= 0x80; dst[1] >>= 1;
			if (dst[2] & 0x01) dst[1] |= 0x80; dst[2] >>= 1;

			// if the bit just shifted out (former bit 23) and the incoming data
			// bit are not equal (i.e. bit_out ^ bit_in == 1) => toggle tap bits
			if (t != (d & 1)) {
				// toggle register tap bits (=XOR with 1) according to CRC polynom
				dst[2] ^= 0xDA; // 0b11011010 inv. = 0b01011011 ^= x^6+x^4+x^3+x+1
				dst[1] ^= 0x60; // 0b01100000 inv. = 0b00000110 ^= x^10+x^9
			}
		}
	}
}

void whiten( uint8_t len ) {

	uint8_t* buf = (uint8_t*)&buffer;
	uint8_t i;

	// initialize LFSR with current channel, set bit 6
	uint8_t lfsr = channel[current] | 0x40;

	while (len--) {
		uint8_t res = 0;
		// LFSR in "wire bit order"
		for (i = 1; i; i <<= 1) {
			if (lfsr & 0x01) {
				lfsr ^= 0x88;
				res |= i;
			}
			lfsr >>= 1;
		}
		*(buf++) ^= res;
	}
}
#endif /* BTLE_H_ */