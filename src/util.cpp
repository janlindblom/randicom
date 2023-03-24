// Copyright (c) 2022 Jan Lindblom <jan@namnlos.co>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "util.h"

const char *macToString(uint8_t mac[6]) {
    static char s[20];
    sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return s;
}

const char *encToString(uint8_t enc) {
    switch (enc) {
        case ENC_TYPE_NONE:
            return "NONE";
        case ENC_TYPE_TKIP:
            return "WPA";
        case ENC_TYPE_CCMP:
            return "WPA2";
        case ENC_TYPE_AUTO:
            return "AUTO";
    }
    return "UNKN";
}
