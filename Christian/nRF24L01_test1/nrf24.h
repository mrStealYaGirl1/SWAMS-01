#pragma once
#include <stdint.h>
#include <stdbool.h>

void NRF24Init(void);
void NRF24InitTX(uint8_t channel, uint8_t payloadLength, const uint8_t *address);
void NRF24InitRX(uint8_t channel, uint8_t payloadLength, const uint8_t *address);

uint8_t NRF24GetStatus(void);

uint8_t NRF24ReadRegister(uint8_t reg);
void NRF24WriteRegister(uint8_t reg, uint8_t value);

void NRF24ReadRegisterMulti(uint8_t reg, uint8_t *data, uint8_t length);
void NRF24WriteRegisterMulti(uint8_t reg, const uint8_t *data, uint8_t length);

void NRF24WritePayload(const uint8_t *data, uint8_t length);
void NRF24ReadPayload(uint8_t *data, uint8_t length);

void NRF24FlushTX(void);
void NRF24FlushRX(void);
void NRF24ClearAllInterrupts(void);

bool NRF24SendPacket(const uint8_t *data, uint8_t length);
bool NRF24DataReady(void);
void NRF24ReceivePacket(uint8_t *data, uint8_t length);