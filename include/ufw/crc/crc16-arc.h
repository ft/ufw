/*
 * Copyright (c) 2023-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_CRC_CRC16_ARC_H_19c5e461
#define INC_UFW_CRC_CRC16_ARC_H_19c5e461

/**
 * @addtogroup checksums Checksum Algorithms
 *
 * Implementation of common checksum functions
 *
 * @{
 *
 * @file ufw/crc/crc16-arc.h
 * @brief CRC16 ARC API
 *
 * @}
 */

#include <stddef.h>
#include <stdint.h>

#define CRC16_ARC_INITIAL 0x0000u

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

uint16_t ufw_crc16_arc(uint16_t, const void*, size_t);
uint16_t ufw_buffer_crc16_arc(const void*, size_t);

uint16_t ufw_crc16_arc_u16(uint16_t, const uint16_t*, size_t);
uint16_t ufw_buffer_crc16_arc_u16(const uint16_t*, size_t);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_CRC_CRC16_ARC_H_19c5e461 */
