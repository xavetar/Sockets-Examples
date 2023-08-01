# Interprocess Communication via Control Messages (Ancillary Data)

This project demonstrates examples of using control messages (ancillary data) in the context of interprocess communication. Control messages allow you to pass additional information along with your data when sending and receiving messages between processes or sockets.

Control messages are implemented using the `cmsghdr` structure, which is part of the standard POSIX headers and is defined in `<sys/socket.h>`.

### Supported Control Messages and Their Usage

#### Linux

- ✓ SCM_RIGHTS (0x01): Access rights (array of file descriptors)
    - LU/SOCK_STREAM - UNIX/SOCK_DGRAM - UNIX/SOCK_SEQPACKET - UNIX
- ✓ SCM_TIMESTAMP (0x02): Timestamp (struct timeval)
    - LU/SOCK_DGRAM - UNIX
    - INET/SOCK_STREAM - IPPROTO_TCP/SOCK_DGRAM - IPPROTO_UDP
    - INET6/SOCK_STREAM - IPPROTO_TCP/SOCK_DGRAM - IPPROTO_UDP
- ✓ SCM_CREDENTIALS (0x03): Process credentials (struct ucred)
    - LU/SOCK_STREAM - UNIX/SOCK_DGRAM - UNIX
- × SCM_SECURITY (0x04): Security label
    - NOT IMPLEMENTED
- × SCM_PIDFD (0x05): PID file descriptor (int)
    - NOT IMPLEMENTED
- ✓ SCM_TIMESTAMPNS (0x24/0x40): Timestamp in nanoseconds (struct timespec)
    - LU/SOCK_DGRAM - UNIX
    - INET/SOCK_STREAM - IPPROTO_TCP/SOCK_DGRAM - IPPROTO_UDP
    - INET6/SOCK_STREAM - IPPROTO_TCP/SOCK_DGRAM - IPPROTO_UDP
- ✓ SCM_TIMESTAMPING (0x25/0x41): Complex timestamps (struct scm_timestamping)
    - INET/SOCK_STREAM - IPPROTO_TCP/SOCK_DGRAM - IPPROTO_UDP
    - INET6/SOCK_STREAM - IPPROTO_TCP/SOCK_DGRAM - IPPROTO_UDP

#### Haiku

- ✓ SCM_RIGHTS (0x01): Access rights (array of file descriptors)
    - LU/SOCK_STREAM - UNIX/SOCK_DGRAM - UNIX

#### XNU

- ✓ SCM_RIGHTS (0x01): Access rights (array of file descriptors)
    - LU/SOCK_STREAM - UNIX/SOCK_DGRAM - UNIX
- ✓ SCM_TIMESTAMP (0x02): Timestamp (struct timeval)
    - INET/SOCK_DGRAM - IPPROTO_UDP
    - INET6/SOCK_DGRAM - IPPROTO_UDP
- × SCM_CREDS (0x03): Process creds (struct cmsgcred)
    - Unsupported by Apple
- ✓ SCM_TIMESTAMP_MONOTONIC (0x04): Timestamp (uint64_t)
    - INET/SOCK_DGRAM - IPPROTO_UDP
    - INET6/SOCK_DGRAM - IPPROTO_UDP

#### FreeBSD

- ✓ SCM_RIGHTS (0x01): Access rights (array of file descriptors)
    - LU/SOCK_STREAM - UNIX/SOCK_DGRAM - UNIX
- ✓ SCM_TIMESTAMP (0x02): Timestamp (struct timeval)
    - INET/SOCK_DGRAM - IPPROTO_UDP
    - INET6/SOCK_DGRAM - IPPROTO_UDP
- ✓ SCM_CREDS (0x03): Process creds (struct cmsgcred)
    - LU/SOCK_STREAM - UNIX/SOCK_DGRAM - UNIX
- ✓ SCM_BINTIME (0x04): Timestamp (struct bintime)
    - INET/SOCK_DGRAM - IPPROTO_UDP
- ✓ SCM_REALTIME (0x05): Timestamp (struct timespec)
    - INET/SOCK_DGRAM - IPPROTO_UDP
    - INET6/SOCK_DGRAM - IPPROTO_UDP
- ✓ SCM_MONOTONIC (0x06): Timestamp (struct timespec)
    - INET/SOCK_DGRAM - IPPROTO_UDP
    - INET6/SOCK_DGRAM - IPPROTO_UDP
- ✓ SCM_TIME_INFO * (0x07): Timestamp info
    - Supported only with certain types of network devices on FreeBSD
- ✓ SCM_CREDS2 (0x08): Process creds (struct sockcred2)
    - LU/SOCK_STREAM - UNIX/SOCK_DGRAM - UNIX

#### Time Stamping Options (BSD)

- ✓ SO_TS_REALTIME_MICRO (0x00): Microsecond resolution, realtime
- ✓ SO_TS_BINTIME (0x01): Sub-nanosecond resolution, realtime
- ✓ SO_TS_REALTIME (0x02): Nanosecond resolution, realtime
- ✓ SO_TS_MONOTONIC (0x03): Nanosecond resolution, monotonic

### Additional Notes

To explore the examples and their implementations, refer to the following links for detailed information:

- Linux: [linux/socket.h](https://github.com/torvalds/linux/tree/master/include/linux/socket.h#L177), [linux/socket.h](https://github.com/torvalds/linux/tree/master/include/uapi/asm-generic/socket.h#L157C1-L157C1)
- Haiku: [haiku/headers/posix/sys/socket.h](https://github.com/haiku/haiku/tree/master/headers/posix/sys/socket.h#L151)
- XNU: [apple-oss-distributions/xnu/bsd/sys/socket.h](https://github.com/apple-oss-distributions/xnu/tree/master/bsd/sys/socket.h#L1164)
- FreeBSD: [freebsd/freebsd-src/sys/sys/socket.h](https://github.com/freebsd/freebsd-src/tree/master/sys/sys/socket.h#L587)
- * Supported only with certain types of network devices on FreeBSD
- * Driver support is only implemented for ConnectX4/5: [https://reviews.freebsd.org/D12638](https://reviews.freebsd.org/D12638)
- * Research:
    - [FreeBSD: ip_input.c](https://github.com/freebsd/freebsd-src/tree/master/sys/netinet/ip_input.c#L1191)
    - [FreeBSD: in_pcb.h](https://github.com/freebsd/freebsd-src/tree/master/sys/netinet/in_pcb.h#L231)

For more information on each control message's behavior and usage, refer to the links provided for the corresponding
