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
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>

#define F_UNIX 0
#define TIME_SIZE 20
#define BUFF_SIZE 65535
#define SOCKET_PATH "/tmp/RECEIVER"

void debug_sock_unix(const socklen_t* address_size, const struct sockaddr_un* address, char* from) {
    printf("\nSender size (%s): %u\n", from, *address_size);
    printf("Sender unix socket path (%s): %s\n", from, address->sun_path);
    printf("Sender family (%s): %hu\n\n", from, address->sun_family);
}

int decode_timespec(const struct timespec* timestamp) {
    // Convert time_t to the broken-down time (struct tm)
    struct tm *time_info = localtime(&timestamp->tv_sec);

    // Print the time in the desired format
    char *time_str = calloc((size_t) TIME_SIZE, sizeof(char));
    if (time_str == NULL) {
        perror("\n\ncalloc");
        return -1;
    }

    strftime(time_str, (size_t) TIME_SIZE, "%Y-%m-%d %H:%M:%S", time_info);
    printf("Timestamp (strftime - timespec): %s\n", time_str);

    // Print the time in the manual format
    printf("Timestamp (manually - timespec): %02d-%02d-%04d %02d:%02d:%02d.%06ld\n\n",
           time_info->tm_mday, time_info->tm_mon + 1, time_info->tm_year + 1900,
           time_info->tm_hour, time_info->tm_min, time_info->tm_sec, timestamp->tv_nsec);

    // Clean memory
    free(time_info);
    free(time_str);

    return 0;
}

int decode_scm_timestamping(struct scm_timestamping* ts) {
    // Print each timestamp in the structure
    for (int i = 0; i < 3; ++i) {
        printf("Timestamp %d:\n", i + 1);
        if (decode_timespec(&ts->ts[i]) == -1) {
            return -1;
        }
        printf("\n");
    }

    // Clean memory
    free(ts);

    return 0;
}

int process_cmsg(struct cmsghdr* cmsg) {
    if (cmsg != NULL) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMPING) {
            // Declaration and assign timestamp
            struct scm_timestamping *ts = calloc(1, sizeof(struct scm_timestamping));
            if (ts == NULL) {
                perror("\n\ncalloc");
                exit(EXIT_FAILURE);
            }

            memcpy(ts, CMSG_DATA(cmsg), sizeof(struct scm_timestamping));

            return decode_scm_timestamping(ts);
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

    int options = SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE;
    if (setsockopt(socket_file_descriptor, SOL_SOCKET, SO_TIMESTAMPING, &options, sizeof(options)) == -1) {
        perror("setsockopt");
        close(socket_file_descriptor);
        exit(EXIT_FAILURE);
    }

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
    free(control_buffer);

    return 0;
}
