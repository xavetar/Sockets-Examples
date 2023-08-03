/*
 * Copyright 2023 Stanislav Mikhailov (xavetar)
 *
 * Licensed under the Creative Commons Zero v1.0 Universal (CC0) License.
 * You may obtain a copy of the License at
 *
 *     http://creativecommons.org/publicdomain/zero/1.0/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the CC0 license is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define LOOP_BACK 1
#define TIME_SIZE 20
#define BUFF_SIZE 65535
#define RECEIVER_PORT 54321

void debug_sock_v6(const socklen_t* address_size, const struct sockaddr_in6* address, char* from) {
    printf("\nSender size (%s): %u\n", from, *address_size);
    printf("Sender length (%s): %hhu\n", from, address->sin6_len);
    printf("Sender family (%s): %hu\n", from, address->sin6_family);
    printf("Sender port (%s):: %hu\n", from, ntohs(address->sin6_port));
    printf("Sender flow info (%s):: %hu\n", from, ntohs(address->sin6_flowinfo));

    char *ip_str = calloc(INET6_ADDRSTRLEN, sizeof(char));
    if (ip_str == NULL) {
        perror("\n\ncalloc");
        exit(EXIT_FAILURE);
    }

    inet_ntop(AF_INET6, &(address->sin6_addr), ip_str, INET6_ADDRSTRLEN);
    printf("Sender address (%s): %s\n", from, ip_str);

    printf("Sender flow info (%s):: %hu\n", from, ntohs(address->sin6_scope_id));
    printf("\n");
}

int decode_sock_timestamp_info(const struct sock_timestamp_info* sti_info) {
    printf("\nSTI Flags (st_info_flags): 0x%04x\n", sti_info->st_info_flags);
    printf("STI Hardware (ST_INFO_HW): %s\n", (sti_info->st_info_flags & ST_INFO_HW) ? "Yes" : "No");
    printf("STI Hardware High-Precision Record (ST_INFO_HW_HPREC): %s\n", (sti_info->st_info_flags & ST_INFO_HW_HPREC) ? "Yes" : "No");
    printf("STI Padding (st_info_pad0): %lu\n", (unsigned long) sti_info->st_info_pad0);
    printf("STI Reserved (st_info_rsv): [");
    for (int i = 0; i < 7; i++) {
        printf("%llu", (unsigned long long) sti_info->st_info_rsv[i]);
        if (i < 6)
            printf(", ");
    }
    printf("]\n\n");

    return 0;
}

int decode_timeval(const struct timeval* timestamp) {
    // Convert time_t to the broken-down time (struct tm)
    struct tm *time_info = localtime(&timestamp->tv_sec);

    // Print the time in the desired format
    char *time_str = calloc((size_t) TIME_SIZE, sizeof(char));
    if (time_str == NULL) {
        perror("\n\ncalloc");
        return -1;
    }

    strftime(time_str, (size_t) TIME_SIZE, "%Y-%m-%d %H:%M:%S", time_info);
    printf("Timestamp (strftime - timeval): %s\n", time_str);

    // Print the time in the manual format
    printf("Timestamp (manually - timeval): %02d-%02d-%04d %02d:%02d:%02d.%06ld\n\n",
           time_info->tm_mday, time_info->tm_mon + 1, time_info->tm_year + 1900,
           time_info->tm_hour, time_info->tm_min, time_info->tm_sec, (unsigned long) timestamp->tv_usec);

    return 0;
}

int process_cmsg(struct cmsghdr* cmsg) {
    if (cmsg != NULL) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP) {
            // Declaration and assign timeval
            struct timeval *timestamp = calloc(1, sizeof(struct timeval));

            memcpy(timestamp, CMSG_DATA(cmsg), sizeof(struct timeval));

            return decode_timeval(timestamp);
        }
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIME_INFO) {
            // Declaration and assign socket timestamp info
            struct sock_timestamp_info *timestamp = calloc(1, sizeof(struct sock_timestamp_info));

            memcpy(timestamp, CMSG_DATA(cmsg), sizeof(struct sock_timestamp_info));

            return decode_sock_timestamp_info(timestamp);
        }
        printf("Total current cmsg length: %lu\n", (size_t) cmsg->cmsg_len);
    }

    return 0;
}

int main() {
    // Set buffer for data receive
    char *iov_buffer = calloc(BUFF_SIZE, sizeof(char));
    // Set control buffer for receive data
    char *control_buffer = calloc(BUFF_SIZE, sizeof(char));

    // Declaration and assign socket descriptor
    int socket_file_descriptor = -1;
    // Declaration and assign sender address struct size
    socklen_t sender_message_address_size = sizeof(struct sockaddr_storage);

    // Declaration and assign io vector
    struct iovec iov = {0};
    // Declaration and assign message header
    struct msghdr message = {0};
    // Declaration and assign socket address unix
    struct sockaddr_in6 socket_address = {0};
    // Declaration and assign client socket address
    struct sockaddr_storage sender_message_address = {0};

    // Clean buffer
    memset(&iov, 0, sizeof(iov));
    memset(&message, 0, sizeof(message));
    memset(&socket_address, 0, sizeof(socket_address));
    memset(&sender_message_address, 0, sizeof(sender_message_address));

    // Create socket
    socket_file_descriptor = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_file_descriptor == -1) {
        perror("\n\nsocket");
        return 1;
    }

    int timestamp_option = 1;
    if (setsockopt(
            socket_file_descriptor, SOL_SOCKET, SO_TIMESTAMP, &timestamp_option, sizeof(timestamp_option)
        ) == -1) {
        perror("setsockopt");
        close(socket_file_descriptor);
        return 1;
    }

    int ts_clock_option = SO_TS_REALTIME_MICRO;
    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_TS_CLOCK, &ts_clock_option, sizeof(ts_clock_option)) == -1) {
        perror("setsockopt");
        close(socket_file_descriptor);
        exit(EXIT_FAILURE);
    }

    // Set socket socket address
    socket_address.sin6_family = PF_INET6;
#if LOOP_BACK == 0
    socket_address.sin6_addr = in6addr_any;
#elif LOOP_BACK == 1
    socket_address.sin6_addr = in6addr_loopback;
#endif
    socket_address.sin6_port = htons(RECEIVER_PORT);

    // Bind socket to socket address
    if (bind(socket_file_descriptor, (struct sockaddr *) &socket_address, sizeof(socket_address)) == -1) {
        perror("\n\nbind");
        return 1;
    }

    // Init iovec
    iov = (struct iovec) { .iov_base = iov_buffer, .iov_len = (size_t) BUFF_SIZE };

    // Init msghdr
    message = (struct msghdr) {
            .msg_name = &sender_message_address, .msg_namelen = sender_message_address_size,
            .msg_iov = &iov, .msg_iovlen = 1, .msg_control = control_buffer, .msg_controllen = (size_t) BUFF_SIZE
    };

    // Receive message with file descriptor
    ssize_t received = recvmsg(socket_file_descriptor, &message, 0);
    if (received == -1) {
        perror("\n\nrecvmsg");
        return 1;
    }

    debug_sock_v6(&sender_message_address_size, (struct sockaddr_in6 *) &sender_message_address, "recvmsg");

    printf("iov_base: %s\n", iov_buffer);
    printf("iov_base_len: %lu\n", iov.iov_len);
    printf("Current iov length: %i\n\n", message.msg_iovlen);

    // Handle received ancillary data
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&message);

    while (cmsg != NULL) {
        if (process_cmsg(cmsg) == -1) {
            fprintf(stderr, "Error message: Something went wrong with process cmsg!\n");
            exit(1);
        }
        cmsg = CMSG_NXTHDR(&message, cmsg);
    }

    // Close socket
    close(socket_file_descriptor);

    // Clean memory
    free(cmsg);
    free(iov_buffer);

    return 0;
}
