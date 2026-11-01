/*
 * Copyright (c) 2026 Codium Electronique
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Tests of the dectnrp-driver sample frame builder.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/ztest.h>

#include "dectnrp_mac_helper.h"

/* The sample's previous fixed frame, the oracle for the default payload. */
static const uint8_t expected[SAMPLE_FRAME_SIZE] = {
	/* PHY header */
	0x10, 0x78, 0xee, 0xff, 0x71, 0xff, 0xff, 0x00, 0x00, 0x00,
	/* MAC header type, MAC data header */
	0x00, 0x00, 0x17,
	/* MAC mux header: User Flow 1 IE, 13 bytes payload */
	0x43, 0x0d,
	/* Hello World!\n */
	0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x0a,
	/* MAC mux header: Padding IE, 17 bytes, then the padding */
	0x40, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00};

static uint8_t payload[SAMPLE_FRAME_PAYLOAD_MAX + 1];
static uint8_t frame[SAMPLE_FRAME_SIZE];

static void *setup(void)
{
	for (size_t i = 0; i < sizeof(payload); i++) {
		payload[i] = (uint8_t)(0xa0 + i);
	}
	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	memset(frame, 0xee, sizeof(frame));
}

ZTEST_SUITE(frame, NULL, setup, before, NULL, NULL);

ZTEST(frame, test_build_default)
{
	int ret;

	ret = sample_frame_build(frame, sizeof(frame), sample_frame_default_payload,
				 sample_frame_default_payload_size);
	zassert_equal(ret, SAMPLE_FRAME_SIZE);
	zassert_equal(sample_frame_default_payload_size, 13);
	zassert_mem_equal(frame, expected, sizeof(expected));
}

ZTEST(frame, test_build_padding_boundaries)
{
	int ret;

	ret = sample_frame_build(frame, sizeof(frame), payload, 32);
	zassert_equal(ret, SAMPLE_FRAME_SIZE);
	zassert_equal(frame[13], 0x43);
	zassert_equal(frame[14], 32);
	zassert_mem_equal(&frame[15], payload, 32);

	ret = sample_frame_build(frame, sizeof(frame), payload, 31);
	zassert_equal(ret, SAMPLE_FRAME_SIZE);
	zassert_equal(frame[14], 31);
	zassert_mem_equal(&frame[15], payload, 31);
	zassert_equal(frame[46], 0xC0);

	ret = sample_frame_build(frame, sizeof(frame), payload, 30);
	zassert_equal(ret, SAMPLE_FRAME_SIZE);
	zassert_equal(frame[45], 0xE0);
	zassert_equal(frame[46], 0x00);

	ret = sample_frame_build(frame, sizeof(frame), payload, 29);
	zassert_equal(ret, SAMPLE_FRAME_SIZE);
	zassert_equal(frame[44], 0x40);
	zassert_equal(frame[45], 0x01);
	zassert_equal(frame[46], 0x00);
}

ZTEST(frame, test_build_empty)
{
	static const uint8_t mux[] = {0x43, 0x00, 0x40, 0x1e};
	int ret;

	ret = sample_frame_build(frame, sizeof(frame), NULL, 0);
	zassert_equal(ret, SAMPLE_FRAME_SIZE);
	zassert_mem_equal(&frame[13], mux, sizeof(mux));
	for (size_t i = 17; i < SAMPLE_FRAME_SIZE; i++) {
		zassert_equal(frame[i], 0x00, "byte %zu", i);
	}
}

ZTEST(frame, test_build_rejects)
{
	int ret;

	ret = sample_frame_build(frame, sizeof(frame), payload, 33);
	zassert_equal(ret, -EINVAL);

	ret = sample_frame_build(frame, SAMPLE_FRAME_SIZE - 1, payload, 13);
	zassert_equal(ret, -EINVAL);

	ret = sample_frame_build(frame, sizeof(frame), NULL, 1);
	zassert_equal(ret, -EINVAL);
}
