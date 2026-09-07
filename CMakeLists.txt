cmake_minimum_required(VERSION 3.16)

# Captive-portal DNS sinkhole from ESP-IDF examples.
set(EXTRA_COMPONENT_DIRS
    "$ENV{IDF_PATH}/examples/protocols/http_server/captive_portal/components/dns_server")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(umurmur-esp)
