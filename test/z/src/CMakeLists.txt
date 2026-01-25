# Copyright (c) 2024-2026 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

cmake_minimum_required(VERSION 3.13.1)
project(ufw-zephyr-module-test)
target_sources(app PRIVATE main.c)
target_link_libraries(app PRIVATE ufw ufw-zephyr)
