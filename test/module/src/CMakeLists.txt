cmake_minimum_required(VERSION 3.13.1)
project(ufw-zephyr-module-test)
target_sources(app PRIVATE main.c)
target_link_libraries(app PRIVATE ufw ufw-zephyr)
