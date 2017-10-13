#ifndef ION_H_
#define ION_H_

#include "wiring.h"
#include "rf24.h"

void ionStart(uint8_t *databuff);
void ionSend();
void ionTxOK();
void ionTxFail();
void ionRx();

uint8_t *databuff_ptr;

void ionStart(uint8_t *databuff){
	databuff_ptr = databuff;
	radioBegin();
	setAutoAck(false);
	powerUp();
	openReadingPipe(0, 0xbb, 0x00, 0x00, 0x00, 0x30);
	openWritingPipe(0xbb, 0x00, 0x00, 0x00, 0x30);
	startListening();
	interrupt_control_rfirq_enable();
	interrupts();
}

void ionSend(){
	stopListening();
	write(databuff_ptr, 32, true);
	startListening();
}

ISR(INTERRUPT_VECTOR_RFIRQ){
	bool tx_ok, tx_fail, rx_ready;

	whatHappened(&tx_ok, &tx_fail, &rx_ready);
	if (rx_ready) 
	{
		read(databuff_ptr, 32);
		ionRx();
	}

	if (tx_ok) 
	{
		ionTxOK();
	}

	if (tx_fail) 
	{
		ionTxFail();
	}

	interrupt_clear_rfirq();
}
#endif