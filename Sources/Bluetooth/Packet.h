// SPDX-License-Identifier: GPL-3.0-or-later
// Bluetooth report 0x31: four-byte prefix + nine bytes per contact.
// Transport adapter only: the verified USB decoder and gesture engine stay unchanged.
#ifndef TC_BLUETOOTH_PACKET_H
#define TC_BLUETOOTH_PACKET_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
static size_t TCBluetoothToUSB(const uint8_t*in,size_t length,uint8_t*out,size_t capacity){
 if(!in||!out||length<4||in[0]!=0x31||(length-4)%9||length>4+15*9||capacity<length+8)return 0;
 memset(out,0,12);out[0]=2;out[1]=in[1]&1;memcpy(out+12,in+4,length-4);return length+8;
}
#endif
