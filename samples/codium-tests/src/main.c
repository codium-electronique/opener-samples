/*
 * Copyright (c) 2026 Codium Electronique
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define SPI_FLASH_COMPAT jedec_spi_nor

#define SPI_FLASH_TEST_REGION_OFFSET 0xff000
#define SPI_FLASH_SECTOR_SIZE        4096

const uint8_t erased[] = {0xff, 0xff, 0xff, 0xff};

void single_sector_test(const struct device *flash_dev)
{
	const uint8_t expected[] = {0x55, 0xaa, 0x66, 0x99};
	const size_t len = sizeof(expected);
	uint8_t buf[sizeof(expected)];
	int rc;

	LOG_DBG("\nPerform test on single sector");
	/* Write protection needs to be disabled before each write or
	 * erase, since the flash component turns on write protection
	 * automatically after completion of write and erase
	 * operations.
	 */
	LOG_DBG("\nTest 1: Flash erase\n");

	/* Full flash erase if SPI_FLASH_TEST_REGION_OFFSET = 0 and
	 * SPI_FLASH_SECTOR_SIZE = flash size
	 */
	rc = flash_erase(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, SPI_FLASH_SECTOR_SIZE);
	if (rc != 0) {
		LOG_DBG("Flash erase failed! %d\n", rc);
	} else {
		/* Check erased pattern */
		memset(buf, 0, len);
		rc = flash_read(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, buf, len);
		if (rc != 0) {
			LOG_DBG("Flash read failed! %d\n", rc);
			return;
		}
		if (memcmp(erased, buf, len) != 0) {
			LOG_DBG("Flash erase failed at offset 0x%x got 0x%x\n",
				SPI_FLASH_TEST_REGION_OFFSET, *(uint32_t *)buf);
			return;
		}
		LOG_DBG("Flash erase succeeded!\n");
	}
	LOG_DBG("\nTest 2: Flash write\n");

	LOG_DBG("Attempting to write %zu bytes\n", len);
	rc = flash_write(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, expected, len);
	if (rc != 0) {
		LOG_DBG("Flash write failed! %d\n", rc);
		return;
	}

	memset(buf, 0, len);
	rc = flash_read(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, buf, len);
	if (rc != 0) {
		LOG_DBG("Flash read failed! %d\n", rc);
		return;
	}

	if (memcmp(expected, buf, len) == 0) {
		LOG_DBG("Data read matches data written. Good!!\n");
	} else {
		const uint8_t *wp = expected;
		const uint8_t *rp = buf;
		const uint8_t *rpe = rp + len;

		LOG_DBG("Data read does not match data written!!\n");
		while (rp < rpe) {
			LOG_DBG("%08x wrote %02x read %02x %s\n",
				(uint32_t)(SPI_FLASH_TEST_REGION_OFFSET + (rp - buf)), *wp, *rp,
				(*rp == *wp) ? "match" : "MISMATCH");
			++rp;
			++wp;
		}
	}
}

void multi_sector_test(const struct device *flash_dev)
{
	const uint8_t expected[] = {0x55, 0xaa, 0x66, 0x99};
	const size_t len = sizeof(expected);
	uint8_t buf[sizeof(expected)];
	int rc;

	LOG_DBG("\nPerform test on multiple consecutive sectors");

	/* Write protection needs to be disabled before each write or
	 * erase, since the flash component turns on write protection
	 * automatically after completion of write and erase
	 * operations.
	 */
	LOG_DBG("\nTest 1: Flash erase\n");

	/* Full flash erase if SPI_FLASH_TEST_REGION_OFFSET = 0 and
	 * SPI_FLASH_SECTOR_SIZE = flash size
	 * Erase 2 sectors for check for erase of consequtive sectors
	 */
	rc = flash_erase(flash_dev, SPI_FLASH_TEST_REGION_OFFSET, SPI_FLASH_SECTOR_SIZE * 2);
	if (rc != 0) {
		LOG_DBG("Flash erase failed! %d\n", rc);
	} else {
		/* Read the content and check for erased */
		memset(buf, 0, len);
		size_t offs = SPI_FLASH_TEST_REGION_OFFSET;

		while (offs < SPI_FLASH_TEST_REGION_OFFSET + 2 * SPI_FLASH_SECTOR_SIZE) {
			rc = flash_read(flash_dev, offs, buf, len);
			if (rc != 0) {
				LOG_DBG("Flash read failed! %d\n", rc);
				return;
			}
			if (memcmp(erased, buf, len) != 0) {
				LOG_DBG("Flash erase failed at offset 0x%x got 0x%x\n", offs,
					*(uint32_t *)buf);
				return;
			}
			offs += SPI_FLASH_SECTOR_SIZE;
		}
		LOG_DBG("Flash erase succeeded!\n");
	}

	LOG_DBG("\nTest 2: Flash write\n");

	size_t offs = SPI_FLASH_TEST_REGION_OFFSET;

	while (offs < SPI_FLASH_TEST_REGION_OFFSET + 2 * SPI_FLASH_SECTOR_SIZE) {
		LOG_DBG("Attempting to write %zu bytes at offset 0x%x\n", len, offs);
		rc = flash_write(flash_dev, offs, expected, len);
		if (rc != 0) {
			LOG_DBG("Flash write failed! %d\n", rc);
			return;
		}

		memset(buf, 0, len);
		rc = flash_read(flash_dev, offs, buf, len);
		if (rc != 0) {
			LOG_DBG("Flash read failed! %d\n", rc);
			return;
		}

		if (memcmp(expected, buf, len) == 0) {
			LOG_DBG("Data read matches data written. Good!!\n");
		} else {
			const uint8_t *wp = expected;
			const uint8_t *rp = buf;
			const uint8_t *rpe = rp + len;

			LOG_DBG("Data read does not match data written!!\n");
			while (rp < rpe) {
				LOG_DBG("%08x wrote %02x read %02x %s\n",
					(uint32_t)(offs + (rp - buf)), *wp, *rp,
					(*rp == *wp) ? "match" : "MISMATCH");
				++rp;
				++wp;
			}
		}
		offs += SPI_FLASH_SECTOR_SIZE;
	}
}

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

#define SPI_SLAVE DT_NODELABEL(spi2)

// SPI slave functionality
const struct device *spi_slave_dev;
static struct k_poll_signal spi_slave_done_sig = K_POLL_SIGNAL_INITIALIZER(spi_slave_done_sig);

static const struct spi_config spi_slave_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_SLAVE,
	.frequency = 4000000,
	.slave = 0,
};

static void spi_slave_init(void)
{
	spi_slave_dev = DEVICE_DT_GET(SPI_SLAVE);
	if (!device_is_ready(spi_slave_dev)) {
		LOG_DBG("SPI slave device not ready!\n");
	}
	LOG_DBG("SPI slave device ready!\n");
}

static uint32_t slave_tx_buffer[] = {0};
static uint8_t slave_rx_buffer[5] = {0};
static int spi_slave_write_test_msg(void)
{
	static uint32_t counter = 0;

	const struct spi_buf s_tx_buf = {.buf = slave_tx_buffer, .len = sizeof(slave_tx_buffer)};
	const struct spi_buf_set s_tx = {.buffers = &s_tx_buf, .count = 1};

	struct spi_buf s_rx_buf = {
		.buf = slave_rx_buffer,
		.len = sizeof(slave_rx_buffer),
	};
	const struct spi_buf_set s_rx = {.buffers = &s_rx_buf, .count = 1};

	// Update the TX buffer with a rolling counter
	slave_tx_buffer[0] = counter++;
	LOG_DBG("SPI SLAVE TX: 0x%x\n", slave_tx_buffer[0]);

	// Reset signal
	k_poll_signal_reset(&spi_slave_done_sig);

	// Start transaction
	int error = spi_transceive_signal(spi_slave_dev, &spi_slave_cfg, &s_tx, &s_rx,
					  &spi_slave_done_sig);
	if (error != 0) {
		LOG_DBG("SPI slave transceive error: %i\n", error);
		return error;
	}
	return 0;
}

static int spi_slave_check_for_message(void)
{
	int signaled, result;
	k_poll_signal_check(&spi_slave_done_sig, &signaled, &result);
	if (signaled != 0) {
		LOG_DBG("Message waiting, signal set\n");
		return 0;
	} else {
		return -1;
	}
}

static const struct gpio_dt_spec linux_isr = GPIO_DT_SPEC_GET(DT_NODELABEL(nrp_isr), gpios);
static const struct gpio_dt_spec linux_busy = GPIO_DT_SPEC_GET(DT_NODELABEL(nrp_busy), gpios);

int outputs_init(void)
{
	int rc;

	if (!gpio_is_ready_dt(&linux_isr)) {
		LOG_ERR("Linux ISR GPIO device not ready\n");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&linux_busy)) {
		LOG_ERR("Linux BUSY GPIO device not ready\n");
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&linux_isr, GPIO_OUTPUT_ACTIVE);
	if (rc != 0) {
		LOG_ERR("Failed to configure Linux ISR pin: %d\n", rc);
		return rc;
	}

	rc = gpio_pin_configure_dt(&linux_busy, GPIO_OUTPUT_ACTIVE);
	if (rc != 0) {
		LOG_ERR("Failed to configure Linux BUSY pin: %d\n", rc);
		return rc;
	}

	rc = gpio_pin_set_dt(&linux_isr, 1);
	if (rc != 0) {
		LOG_ERR("Failed to set hall sensor power pin: %d\n", rc);
		return rc;
	}

	rc = gpio_pin_set_dt(&linux_busy, 1);
	if (rc != 0) {
		LOG_ERR("Failed to set hall sensor power pin: %d\n", rc);
		return rc;
	}

	return 0;
}

static int linux_isr_value = 1;
static int linux_busy_value = 1;

static int cmd_isr_toggle(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	linux_isr_value ^= 0x01;

	int rc = gpio_pin_set_dt(&linux_isr, linux_isr_value);

	if (rc != 0) {
		shell_error(sh, "Failed to set ISR line: %d", rc);
		return rc;
	}

	shell_print(sh, "ISR line set to %d", linux_isr_value);
	return 0;
}

static int cmd_busy_toggle(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	linux_busy_value ^= 0x01;

	int rc = gpio_pin_set_dt(&linux_busy, linux_busy_value);

	if (rc != 0) {
		shell_error(sh, "Failed to set BUSY line: %d", rc);
		return rc;
	}

	shell_print(sh, "BUSY line set to %d", linux_busy_value);
	return 0;
}

SHELL_CMD_REGISTER(isr_toggle, NULL, "Toggle the Linux ISR GPIO line", cmd_isr_toggle);
SHELL_CMD_REGISTER(busy_toggle, NULL, "Toggle the Linux BUSY GPIO line", cmd_busy_toggle);

int main(void)
{
	int rc;

	const struct device *flash_dev = DEVICE_DT_GET_ONE(SPI_FLASH_COMPAT);

	LOG_DBG("Codium tests starting...\n");

	if (!device_is_ready(flash_dev)) {
		LOG_ERR("%s: device not ready.\n", flash_dev->name);
		return 0;
	}

	rc = outputs_init();
	if (rc != 0) {
		LOG_ERR("Failed to init Linux outputs: %d\n", rc);
		return 0;
	}

	LOG_DBG("\n%s SPI flash testing\n", flash_dev->name);
	LOG_DBG("==========================\n");

	single_sector_test(flash_dev);
	multi_sector_test(flash_dev);

	spi_slave_init();
	spi_slave_write_test_msg();

	while (1) {
		if (spi_slave_check_for_message() == 0) {
			// Print the last received data
			LOG_DBG("SPI SLAVE RX: %s\n", (char *)slave_rx_buffer);

			// Prepare the next SPI slave transaction
			spi_slave_write_test_msg();
		}

		k_sleep(K_MSEC(100));
	}

	return 0;
}

/* Stub needed to build */
void dectnrp_l2_init(struct net_if *iface)
{
}