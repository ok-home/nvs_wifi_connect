# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

#set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
list(APPEND EXTRA_COMPONENT_DIRS ../prv_wifi_connect)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(example_prv_Wifi_Connect)
