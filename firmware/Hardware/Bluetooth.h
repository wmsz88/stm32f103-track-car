#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "stm32f10x.h"

void Bluetooth_Init(void);
void Serial_SendByte(u8 byte);
void Serial_SendString(char *str);

u8 Serial_GetRxFlag(void);
u8 Serial_GetRxData(void);

#endif
