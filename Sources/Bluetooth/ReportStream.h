// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef TC_BLUETOOTH_REPORT_STREAM_H
#define TC_BLUETOOTH_REPORT_STREAM_H
#include "Packet.h"
// Reference: Linux hid-magicmouse.c DOUBLE_REPORT_ID and TRACKPAD2_BT_REPORT_ID.
// Caller supplies a complete HID report, or a complete HIDP DATA(input) SDU.
// ACL/L2CAP fragments must be reassembled by the transport before this function.
// Validate the entire packet before delivery: a malformed second frame must not
// leave the gesture engine with the first half of an invalid combined packet.
typedef void (*TCBluetoothFrameSink)(void *, const uint8_t *, size_t);
static int TCBluetoothDeliverReport(const uint8_t *data, size_t length,
                                   int hidp, TCBluetoothFrameSink sink, void *context) {
    if (!data || !sink || !length) return 0;
    if (hidp) {
        if (length < 2 || data[0] != 0xa1) return 0;
        data++; length--;
    }
    uint8_t first[147], second[147];
    size_t nfirst=0, nsecond=0;
    if (data[0] == 0xf7) {
        if (length < 2) return 0;
        size_t split=data[1];
        if (split == 0 || split >= length-2) return 0;
        nfirst=TCBluetoothToUSB(data+2,split,first,sizeof(first));
        nsecond=TCBluetoothToUSB(data+2+split,length-2-split,second,sizeof(second));
        if (!nfirst || !nsecond) return 0;
    } else {
        nfirst=TCBluetoothToUSB(data,length,first,sizeof(first));
        if (!nfirst) return 0;
    }
    sink(context,first,nfirst);
    if (nsecond) sink(context,second,nsecond);
    return nsecond ? 2 : 1;
}
#endif
