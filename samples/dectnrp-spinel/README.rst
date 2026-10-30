Opener DECT NR+ Spinel radio co-processor
#########################################

Overview
********

This sample turns an nRF91x1 into a DECT NR+ radio co-processor controlled by
a host over Spinel. The firmware runs only the Opener ``dectnrp`` driver and
its Spinel interface (``CONFIG_DECTNRP_SPINEL``). The host schedules the radio
operations (TX, RX, RSSI1), reads the modem time and receives the driver
events; all the protocol logic runs on the host.

The interface is described in the Opener documentation, ``doc/spinel.rst``.

``main()`` only logs a banner: the Spinel interface starts on its own at boot,
and the radio stays stopped until the host enables it.

Requirements
************

* An nRF9151 DK, an nRF9131 EK or a Codium NR+Gate.
* The DECT NR+ PHY modem firmware programmed on the nRF91x1.

Building and running
********************

Build and flash for the nRF9151 DK:

.. code-block:: bash

    west build -p -b nrf9151dk/nrf9151/ns -d build/nrf9151dk/nrf9151/ns/dectnrp-spinel samples/dectnrp-spinel
    west flash -d build/nrf9151dk/nrf9151/ns/dectnrp-spinel

For the nRF9131 EK, use the board ``nrf9131ek/nrf9131/ns``.

Spinel runs on ``uart0`` at 1 000 000 baud, with HDLC-lite framing and
hardware flow control: on the DK and the EK, it is the first virtual COM port
of the on-board debugger, which follows RTS. Without flow control, the UARTE
loses bytes from the host when its interrupt is served late. The console and
the logs go to SEGGER RTT.

The boot notification is sent before any host can open the port, so a host
starts with a Spinel ``CMD_RESET`` and waits for the ``RESET_SOFTWARE``
status.

On the Codium NR+Gate, build for ``cod_nrp_gw/nrf9151/ns``:

.. code-block:: bash

    west build -p -b cod_nrp_gw/nrf9151/ns -d build/cod_nrp_gw/nrf9151/ns/dectnrp-spinel samples/dectnrp-spinel

Spinel then runs over SPI to the gateway's Linux host, with the Spinel SPI
framing: ``spi2`` is the SPI slave, P0.09 the IRQ line and P0.26 the BUSY
line, both active high (``boards/cod_nrp_gw_nrf9151_ns.overlay``). The console
and the logs stay on ``uart0``, which the gateway reads on ``/dev/ttyS3``.

Configuration
*************

``prj.conf`` sizes the network buffers so that a whole frame, physical header
plus ``DECTNRP_MTU`` payload, fits in one buffer: the nRF91 driver sends the
first buffer of a TX packet only. It allows one TX packet per Spinel
operation slot.
