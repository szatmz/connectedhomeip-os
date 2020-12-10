#
#    Copyright (c) 2020 Project CHIP Authors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
cmake_minimum_required(VERSION 3.13.1)

get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/connectedhomeip REALPATH)
get_filename_component(NRFCONNECT_COMMON ${CHIP_ROOT}/examples/platform/nrfconnect REALPATH)

set(CONF_FILE ${CHIP_ROOT}/config/nrfconnect/sample-defaults.conf prj.conf)
set(CHIP_OPENTHREAD_CONFIG ${NRFCONNECT_COMMON}/project_include/OpenThreadConfig.h)

if(${BOARD} STREQUAL "nrf52840dongle_nrf52840")
list(INSERT OVERLAY_CONFIG 0 ${CHIP_ROOT}/config/nrfconnect/overlay-usb_support.conf)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CHIP_ROOT}/config/nrfconnect/)
set(PIGWEED_ROOT "${CHIP_ROOT}/third_party/pigweed/repo")
include(nrfconnect-app)

include(${PIGWEED_ROOT}/pw_build/pigweed.cmake)
include(${PIGWEED_ROOT}/pw_protobuf_compiler/proto.cmake)

pw_set_backend(pw_log pw_log_basic)
pw_set_backend(pw_assert pw_assert_log)
pw_set_backend(pw_sys_io pw_sys_io.nrfconnect)
set(dir_pw_third_party_nanopb PRESENT CACHE STRING ${CHIP_ROOT}/third_party/nanopb/repo FORCE)

add_subdirectory(third_party/connectedhomeip/examples/platform/nrfconnect/pw_sys_io)
add_subdirectory(third_party/connectedhomeip/third_party/nanopb/repo)
add_subdirectory(third_party/connectedhomeip/third_party/pigweed/repo)

project(chip-nrf52840-pigweed-example)
target_include_directories(app PRIVATE main/include
                           ${NRFCONNECT_COMMON}/util/include
                           ${NRFCONNECT_COMMON}/app/include
                           ${CHIP_ROOT}/src/inet/tests
                           ${CHIP_ROOT}/third_party/nlfaultinjection/repo/include
                           ${PIGWEED_ROOT}/pw_hdlc_lite/public
                           ${PIGWEED_ROOT}/pw_sys_io/public
                           ${PIGWEED_ROOT}/pw_span/public_overrides
                           ${PIGWEED_ROOT}/pw_span/public
                           ${PIGWEED_ROOT}/pw_polyfill/public
                           ${PIGWEED_ROOT}/pw_polyfill/standard_library_public
                           ${PIGWEED_ROOT}/pw_polyfill/public_overrides
                           ${PIGWEED_ROOT}/pw_status/public
                           ${PIGWEED_ROOT}/pw_stream/public
                           ${PIGWEED_ROOT}/pw_preprocessor/public
                           ${NRFCONNECT_COMMON}/pw_sys_io/include)

pw_proto_library(pigweed_test_protolib
  SOURCES
    main/pigweed_test.proto
)

target_sources(app PRIVATE
  main/main.cpp
  main/PwNetworkTestHelpers.cpp
  ${NRFCONNECT_COMMON}/util/test/TestInetCommon.cpp
  ${NRFCONNECT_COMMON}/util/LEDWidget.cpp
)

target_link_libraries(app PRIVATE
  pigweed_test_protolib.nanopb_rpc
  pw_hdlc_lite
  pw_log
  pw_rpc.nanopb.echo_service
  pw_rpc.server
)
