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
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#if defined(__APPLE__)
    #include <mach/mach_time.h>
#endif

#define F_UNIX 0
#define TIME_SIZE 20
#define BUFF_SIZE 65535
#define SOCKET_PATH "/tmp/RECEIVER"

void debug_sock_unix(const socklen_t* address_size, const struct sockaddr_un* address, char* from) {
    printf("\nSender size (%s): %u\n", from, *address_size);
#if !defined(__linux__)
    printf("Sender length (%s): %i\n", from, address->sun_len);
#endif
    printf("Sender unix socket path (%s): %s\n", from, address->sun_path);
    printf("Sender family (%s): %hu\n\n", from, address->sun_family);
}

int decode_uptime(const uint64_t* uptime) {
    // SCM_TIMESTAMP_MONOTONIC - is CLOCK_UPTIME_RAW
    // https://developer.apple.com/library/archive/qa/qa1398/_index.html
    // https://github.com/apple-oss-distributions/xnu/blob/main/bsd/netinet/ip_input.c#L3766
    static mach_timebase_info_data_t timebase_info = {0};

    kern_return_t result = mach_timebase_info(&timebase_info);
    if (result != KERN_SUCCESS) {
        fprintf(stderr, "Error calling mach_timebase_info: %d\n", result);
        return -1;
    }

    // Convert to nanoseconds
    uint64_t nanoseconds = (*uptime * timebase_info.numer) / timebase_info.denom;

    // Convert nanoseconds to seconds and remainder nanoseconds
    uint64_t seconds_total = nanoseconds / NSEC_PER_SEC;

    // Calculate hours, minutes, seconds from the total seconds
    unsigned long long hours = seconds_total / 3600;
    unsigned long long minutes = (seconds_total % 3600) / 60;
    unsigned long long seconds = seconds_total % 60;

    printf("Uptime (in second): %02llu\n", seconds_total);
    printf("Uptime (hours:minutes::seconds): %02llu:%02llu:%02llu\n\n", hours, minutes, seconds);

    return 0;
}

int process_cmsg(struct cmsghdr* cmsg) {
    if (cmsg != NULL) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP_MONOTONIC) {
            // Declaration and assign timestamp
            uint64_t *timestamp = calloc(1, sizeof(uint64_t));

            memcpy(timestamp, CMSG_DATA(cmsg), sizeof(uint64_t));

            return decode_uptime(timestamp);
        }
        printf("Total current cmsg length: %lu\n", (size_t) cmsg->cmsg_len);
    }

    return 0;
}

int main() {
    // Remove socket
    unlink(SOCKET_PATH);

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
    struct sockaddr_un socket_address = {0};
    // Declaration and assign client socket address
    struct sockaddr_storage sender_message_address = {0};

    // Clean buffer
    memset(&iov, 0, sizeof(iov));
    memset(&message, 0, sizeof(message));
    memset(&socket_address, 0, sizeof(socket_address));
    memset(&sender_message_address, 0, sizeof(sender_message_address));

    // Create socket
    socket_file_descriptor = socket(AF_UNIX, SOCK_DGRAM, F_UNIX);
    if (socket_file_descriptor == -1) {
        perror("\n\nsocket");
        return 1;
    }

#if defined(__APPLE__)
    int timestamp_option = 1;
    if (setsockopt(
            socket_file_descriptor, SOL_SOCKET, SO_TIMESTAMP_MONOTONIC, &timestamp_option, sizeof(timestamp_option)
    ) == -1) {
        perror("setsockopt");
        close(socket_file_descriptor);
        return 1;
    }
#else
    printf("\n\n\033[31mWarning! Unsupported Kernel.\033[0m\n");
    exit(EXIT_SUCCESS);
#endif

    // Set socket socket address
    socket_address.sun_family = PF_UNIX;
    strcpy(socket_address.sun_path, SOCKET_PATH);

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

    // Get sender address info
    debug_sock_unix(&sender_message_address_size, (struct sockaddr_un *) &sender_message_address, "recvmsg");

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
