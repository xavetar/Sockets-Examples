cmake_minimum_required(VERSION 3.0...999999.0)

# # # # # # # # # # # # # # # # # # # # #
#    INET - SOCK_STREAM - IPPROTO_TCP   #
# # # # # # # # # # # # # # # # # # # # #

# INET - SOCK_STREAM - IPPROTO_TCP - STANDARD +
add_executable(INET_SOCK_STREAM_IPPROTO_TCP_STANDARD_SENDER STANDARD/sender.c)
add_executable(INET_SOCK_STREAM_IPPROTO_TCP_STANDARD_RECEIVER STANDARD/receiver.c)

# Search library network for Haiku
find_library(LIBNETWORK network)
if(NOT LIBNETWORK)
    message(FATAL_ERROR "libnetwork not found on Haiku.")
endif()

target_link_libraries(INET_SOCK_STREAM_IPPROTO_TCP_STANDARD_SENDER ${LIBNETWORK})
target_link_libraries(INET_SOCK_STREAM_IPPROTO_TCP_STANDARD_RECEIVER ${LIBNETWORK})
