# Copyright (c) 2026 Codium Electronique
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dectnrp_driver_frame)

target_include_directories(app PRIVATE ../src)
target_sources(app PRIVATE
    src/test_frame.c
    src/test_sensor_payload.c
    ../src/dectnrp_mac_helper.c
    ../src/sensor_payload.c
)
