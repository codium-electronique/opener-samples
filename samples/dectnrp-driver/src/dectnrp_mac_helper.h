/*
 * Copyright (c) 2026 Codium Electronique
 * Copyright (c) 2026 Deveritec GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DECTNRP helper providing access to a simplified test message and MAC
 * operations.
 *
 */

#ifndef DECTNRP_MAC_HELPER_H_
#define DECTNRP_MAC_HELPER_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/net/dectnrp_driver.h>

/** Size of the sample frame: 10 bytes PHY header and a 37 bytes PDC (TBS). */
#define SAMPLE_FRAME_SIZE 47

/** Most payload bytes the User Flow 1 IE can carry in the sample frame. */
#define SAMPLE_FRAME_PAYLOAD_MAX 32

/**
 * @brief Default payload, "Hello World!\n".
 */
extern const uint8_t sample_frame_default_payload[];

/**
 * @brief Size of sample_frame_default_payload.
 */
extern const size_t sample_frame_default_payload_size;

/**
 * @brief Build the sample frame with @p payload in the User Flow 1 IE.
 *
 * The rest of the slot is filled with a Padding IE.
 *
 * @param frame buffer receiving the frame
 * @param frame_size size of @p frame, at least SAMPLE_FRAME_SIZE
 * @param payload User Flow 1 payload, may be NULL if @p payload_size is 0
 * @param payload_size at most SAMPLE_FRAME_PAYLOAD_MAX
 * @return SAMPLE_FRAME_SIZE on success, -EINVAL on invalid arguments
 */
int sample_frame_build(uint8_t *frame, size_t frame_size, const uint8_t *payload,
		       size_t payload_size);

/**
 * @brief PHY header type of the sample frame.
 */
extern const uint8_t sample_frame_phy_header_type;

/**
 * @brief Transport parameters of the sample frame.
 */
extern const struct dectnrp_transport_parameters sample_frame_parameters;

/**
 * @brief Encode short part of given @p network_id into given @p pcc.
 *
 * @param pcc
 * @param network_id
 */
void pcc_set_short_network_id(uint8_t *pcc, uint32_t network_id);

/**
 * @brief Encode short transmitter id from given @p short_device_id into given
 * @p pcc.
 *
 * @param pcc
 * @param short_device_id
 */
void pcc_set_transmitter_short_id(uint8_t *pcc, uint16_t short_device_id);

/**
 * @brief Encode short receiver id from given @p short_device_id into given @p
 * pcc.
 *
 * @param pcc
 * @param short_device_id
 */
void pcc_set_receiver_short_id(uint8_t *pcc, uint16_t short_device_id);

/**
 * @brief Decode short transmitter id from given \p pcc.
 *
 * @attention Provided buffer @p pcc must have at least
 * DECTNRP_PHY_HEADER_TYPE1_SIZE.
 *
 * @param pcc pointer to buffer containing PCC.
 * @param len length of given @p pcc
 * @return uint16_t transmitter id
 */
uint16_t pcc_decode_transmitter_short_id(const uint8_t *pcc, size_t len);

#endif /* DECTNRP_MAC_HELPER_H_ */
