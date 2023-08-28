#ifndef CRC_H
#define CRC_H

#include <stdint.h>

uint16_t crc_checksum(const char *data, size_t length) {
    uint16_t crc = 0;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

int verify_checksum(const char *sentence, size_t length) {
    uint16_t calculated_crc = crc_checksum(sentence, length - 3);

    uint16_t expected_crc;
    sscanf(sentence + length - 2, "*%2hx", &expected_crc);

    if (calculated_crc == expected_crc) {
        return 1;
    } else {
        return 0;
    }
}

#endif
