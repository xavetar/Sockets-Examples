cmake_minimum_required(VERSION 3.0...999999.0)

# # # # # # # # # # # # # # # # # # # # #
#    INET6 - SOCK_DGRAM - IPPROTO_UDP   #
# # # # # # # # # # # # # # # # # # # # #

# INET6 - SOCK_DGRAM - IPPROTO_UDP - STANDARD +
add_executable(INET6_SOCK_DGRAM_IPPROTO_UDP_STANDARD_SENDER STANDARD/sender.c)
add_executable(INET6_SOCK_DGRAM_IPPROTO_UDP_STANDARD_RECEIVER STANDARD/receiver.c)

# INET6 - SOCK_DGRAM - IPPROTO_UDP - SCM_TIMESTAMP +
add_executable(INET6_SOCK_DGRAM_IPPROTO_UDP_SCM_TIMESTAMP_SENDER CMSG/SCM_TIMESTAMP/sender.c)
add_executable(INET6_SOCK_DGRAM_IPPROTO_UDP_SCM_TIMESTAMP_RECEIVER CMSG/SCM_TIMESTAMP/receiver.c)

# INET6 - SOCK_DGRAM - IPPROTO_UDP - SCM_TIMESTAMPING +
add_executable(INET6_SOCK_DGRAM_IPPROTO_UDP_SCM_TIMESTAMPING_SENDER CMSG/SCM_TIMESTAMPING/sender.c)
add_executable(INET6_SOCK_DGRAM_IPPROTO_UDP_SCM_TIMESTAMPING_RECEIVER CMSG/SCM_TIMESTAMPING/receiver.c)

# INET6 - SOCK_DGRAM - IPPROTO_UDP - SCM_TIMESTAMPNS +
add_executable(INET6_SOCK_DGRAM_IPPROTO_UDP_SCM_TIMESTAMPNS_SENDER CMSG/SCM_TIMESTAMPNS/sender.c)
add_executable(INET6_SOCK_DGRAM_IPPROTO_UDP_SCM_TIMESTAMPNS_RECEIVER CMSG/SCM_TIMESTAMPNS/receiver.c)
