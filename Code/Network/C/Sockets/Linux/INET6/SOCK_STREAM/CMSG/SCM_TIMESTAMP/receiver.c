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

    // Clean memory
    free(ip_str);
}

int decode_timeval(struct timeval* timestamp) {
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

    // Clean memory
    free(time_info);
    free(time_str);
    free(timestamp);

    return 0;
}

int process_cmsg(struct cmsghdr* cmsg) {
    if (cmsg != NULL) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP) {
            // Declaration and assign timeval
            struct timeval *timestamp = calloc(1, sizeof(struct timeval));
            if (timestamp == NULL) {
                perror("\n\ncalloc");
                exit(EXIT_FAILURE);
            }

            memcpy(timestamp, CMSG_DATA(cmsg), sizeof(struct timeval));

            return decode_timeval(timestamp);
        }
        printf("Total current cmsg length: %lu\n", (size_t) cmsg->cmsg_len);
    }

    return 0;
}

int main() {
    // Set buffer for data receive
    char *iov_buffer = calloc((size_t) BUFF_SIZE, sizeof(char));
    // Set control buffer for receive data
    char *control_buffer = calloc((size_t) BUFF_SIZE, sizeof(char));

    // Declaration and assign socket descriptor
    int socket_file_descriptor = -1;
    // Declaration and assign client descriptor
    int client_file_descriptor = -1;
    // Declaration and assign sender address struct size
    socklen_t sender_address_size = sizeof(struct sockaddr_in6);
    // Declaration and assign sender message address struct size
    socklen_t sender_message_address_size = sizeof(struct sockaddr_storage);

    // Declaration and assign input/output vector
    struct iovec iov = {0};
    // Declaration and assign message header
    struct msghdr message = {0};
    // Declaration and assign socket address unix
    struct sockaddr_in6 socket_address = {0};
    // Declaration and assign client socket address
    struct sockaddr_storage sender_address = {0};
    // Declaration and assign client socket address from message
    struct sockaddr_storage sender_message_address = {0};

    // Clean buffer
    memset(&iov, 0, sizeof(iov));
    memset(&message, 0, sizeof(message));
    memset(&socket_address, 0, sizeof(socket_address));
    memset(&sender_address, 0, sizeof(sender_address));
    memset(&sender_message_address, 0, sizeof(sender_message_address));

    // Create socket
    socket_file_descriptor = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
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

    // Listen input connections
    if (listen(socket_file_descriptor, 1) == -1) {
        perror("\n\nlisten");
        return 1;
    }

    // Accept incoming connection
    client_file_descriptor = accept(socket_file_descriptor, (struct sockaddr *) &sender_address, &sender_address_size);
    if (client_file_descriptor == -1) {
        perror("\n\naccept");
        return 1;
    }

    // Print info about sender
    debug_sock_v6(&sender_address_size, (const struct sockaddr_in6 *) &sender_address, "accept");

    // Init iovec
    iov = (struct iovec) { .iov_base = iov_buffer, .iov_len = (size_t) BUFF_SIZE };

    // Init msghdr
    message = (struct msghdr) {
            .msg_name = &sender_message_address, .msg_namelen = sender_message_address_size,
            .msg_iov = &iov, .msg_iovlen = 1, .msg_control = control_buffer, .msg_controllen = (size_t) BUFF_SIZE
    };

    // Receive message with file descriptor
    ssize_t received = recvmsg(client_file_descriptor, &message, 0);
    if (received == -1) {
        perror("\n\nrecvmsg");
        return 1;
    }

    // Get sender address info from message
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
    close(client_file_descriptor);
    close(socket_file_descriptor);

    // Clean memory
    free(cmsg);
    free(iov_buffer);
    free(control_buffer);

    return 0;
}
