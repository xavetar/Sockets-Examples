cmake_minimum_required(VERSION 3.0...999999.0)

# # # # # # # # # # # # # # # # # # # # #
#    INET - SOCK_DGRAM - IPPROTO_UDP    #
# # # # # # # # # # # # # # # # # # # # #

# INET - SOCK_DGRAM - IPPROTO_UDP - STANDARD +
add_executable(INET_SOCK_DGRAM_IPPROTO_UDP_STANDARD_SENDER STANDARD/sender.c)
add_executable(INET_SOCK_DGRAM_IPPROTO_UDP_STANDARD_RECEIVER STANDARD/receiver.c)

# INET - SOCK_DGRAM - IPPROTO_UDP - SCM_TIMESTAMP +
add_executable(INET_SOCK_DGRAM_IPPROTO_UDP_SCM_TIMESTAMP_SENDER CMSG/SCM_TIMESTAMP/sender.c)
add_executable(INET_SOCK_DGRAM_IPPROTO_UDP_SCM_TIMESTAMP_RECEIVER CMSG/SCM_TIMESTAMP/receiver.c)

# INET - SOCK_DGRAM - IPPROTO_UDP - SCM_TIMESTAMP_MONOTONIC +
add_executable(INET_SOCK_DGRAM_IPPROTO_UDP_SCM_TIMESTAMP_MONOTONIC_SENDER CMSG/SCM_TIMESTAMP_MONOTONIC/sender.c)
add_executable(INET_SOCK_DGRAM_IPPROTO_UDP_SCM_TIMESTAMP_MONOTONIC_RECEIVER CMSG/SCM_TIMESTAMP_MONOTONIC/receiver.c)
