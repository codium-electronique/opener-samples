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

#include <errno.h>
#include <string.h>

#include "dectnrp_mac_helper.h"

/** Offset of the MAC mux header of the User Flow 1 IE in the frame. */
#define MUX_OFFSET 13

/**
 * @brief Start of the test frame: PHY header, MAC header type and MAC data header.
 */
static const uint8_t frame_head[MUX_OFFSET] = {
	/* PHY header */
	0x10, /* Header format=0, packetlength-type=SLOTS, packet length=1 slot */
	0x78, /* Short Network ID */
	0xee,
	0xff, /* Transmitter ID (short id)*/
	0x71, /* Transmit Power=7(0dBm), DF MCS=1 */
	0xff,
	0xff, /* Receiver Id */
	0x00,
	0x00,
	0x00, /* Feedback Format0 - no feedback */

	/* MAC header type */
	0x00, /* Data header, without security */
	/* MAC data header */
	0x00,
	0x17, /* No reset, sequence number=0x17 */
};

const uint8_t sample_frame_default_payload[] = "Hello World!\n";
const size_t sample_frame_default_payload_size = sizeof(sample_frame_default_payload) - 1;

int sample_frame_build(uint8_t *frame, size_t frame_size, const uint8_t *payload,
		       size_t payload_size)
{
	if (frame == NULL || frame_size < SAMPLE_FRAME_SIZE ||
	    payload_size > SAMPLE_FRAME_PAYLOAD_MAX || (payload == NULL && payload_size != 0)) {
		return -EINVAL;
	}

	memset(frame, 0, SAMPLE_FRAME_SIZE);
	memcpy(frame, frame_head, sizeof(frame_head));

	/* MAC mux header: MAC_Ext 01 (8-bit length), IE type 000011 (User plane data,
	 * flow 1), then the payload length (ETSI TS 103 636-4 V2.2.1 Tables 6.3.4-1
	 * and 6.3.4-2).
	 */
	size_t offset = MUX_OFFSET;

	frame[offset++] = 0x43;
	frame[offset++] = (uint8_t)payload_size;
	if (payload_size > 0) {
		memcpy(&frame[offset], payload, payload_size);
		offset += payload_size;
	}

	/* The slot keeps its size, so the rest of the 32 bytes after the MAC header is a
	 * Padding IE (ETSI TS 103 636-4 V2.2.1 §6.4.3.8, Tables 6.3.4-1, 6.3.4-3 and
	 * 6.3.4-4): one byte is the mux header 0xC0 alone (MAC_Ext 11, length bit 0),
	 * two bytes are the mux header 0xE0 (MAC_Ext 11, length bit 1) and one padding byte,
	 * more are the mux header 0x40 (MAC_Ext 01) with a length byte, then zeros.
	 */
	size_t remaining = SAMPLE_FRAME_PAYLOAD_MAX - payload_size;

	if (remaining == 1) {
		frame[offset] = 0xC0;
	} else if (remaining == 2) {
		frame[offset] = 0xE0;
	} else if (remaining >= 3) {
		frame[offset] = 0x40;
		frame[offset + 1] = (uint8_t)(remaining - 2);
	}

	return SAMPLE_FRAME_SIZE;
}

const uint8_t sample_frame_phy_header_type = 1;
const struct dectnrp_transport_parameters sample_frame_parameters = {
	.beta = 1,
	.mu = 1,
	.tbs = 37,
	.mcs = 1,
	.slots = 1,
};

void pcc_set_short_network_id(uint8_t *pcc, uint32_t network_id)
{
	pcc[1] = network_id & 0xff;
}

void pcc_set_transmitter_short_id(uint8_t *pcc, uint16_t short_device_id)
{
	UNALIGNED_PUT(sys_cpu_to_be16(short_device_id), (uint16_t *)(&pcc[2]));
}

void pcc_set_receiver_short_id(uint8_t *pcc, uint16_t short_device_id)
{
	UNALIGNED_PUT(sys_cpu_to_be16(short_device_id), (uint16_t *)(&pcc[5]));
}

uint16_t pcc_decode_transmitter_short_id(const uint8_t *pcc, size_t len)
{

	__ASSERT(len >= DECTNRP_PHY_HEADER_TYPE1_SIZE,
		 "buffer len < DECTNRP_PHY_HEADER_TYPE1_SIZE");
	uint16_t raw = *(uint16_t *)(&pcc[2]);
	return sys_be16_to_cpu(raw);
}
