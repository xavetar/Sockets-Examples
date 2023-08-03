cmake_minimum_required(VERSION 3.0...999999.0)

# # # # # # # # # # # # # # # # # # # # #
#         LU - SOCK_DGRAM - UNIX        #
# # # # # # # # # # # # # # # # # # # # #

# LOCAL/UNIX - SOCK_DGRAM - F_UNIX - STANDARD +
add_executable(LU_SOCK_DGRAM_UNIX_STANDARD_SENDER STANDARD/sender.c)
add_executable(LU_SOCK_DGRAM_UNIX_STANDARD_RECEIVER STANDARD/receiver.c)

# LOCAL/UNIX - SOCK_DGRAM - F_UNIX - SCM_RIGHTS +
add_executable(LU_SOCK_DGRAM_UNIX_SCM_RIGHTS_SENDER CMSG/SCM_RIGHTS/sender.c)
add_executable(LU_SOCK_DGRAM_UNIX_SCM_RIGHTS_RECEIVER CMSG/SCM_RIGHTS/receiver.c)

# Search library network for Haiku
find_library(LIBNETWORK network)
if(NOT LIBNETWORK)
    message(FATAL_ERROR "libnetwork not found on Haiku.")
endif()

target_link_libraries(LU_SOCK_DGRAM_UNIX_STANDARD_SENDER ${LIBNETWORK})
target_link_libraries(LU_SOCK_DGRAM_UNIX_STANDARD_RECEIVER ${LIBNETWORK})

target_link_libraries(LU_SOCK_DGRAM_UNIX_SCM_RIGHTS_SENDER ${LIBNETWORK})
target_link_libraries(LU_SOCK_DGRAM_UNIX_SCM_RIGHTS_RECEIVER ${LIBNETWORK})
