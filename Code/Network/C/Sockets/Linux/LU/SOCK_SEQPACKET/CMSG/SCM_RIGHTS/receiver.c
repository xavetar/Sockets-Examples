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

#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#define F_UNIX 0
#define BUFF_SIZE 65535
#define SOCKET_PATH "/tmp/RECEIVER"

void debug_sock_unix(const socklen_t* address_size, const struct sockaddr_un* address, char* from) {
    printf("\nSender size (%s): %u\n", from, *address_size);
    printf("Sender unix socket path (%s): %s\n", from, address->sun_path);
    printf("Sender family (%s): %hu\n\n", from, address->sun_family);
}

int read_descriptors(int* cmsg_descriptors, size_t cmsg_count_descriptors) {
    // Declaration and assign count of real file descriptors without align area
    size_t count_descriptors = 0;
    // Set buffer for read data
    char *data = calloc((size_t) BUFF_SIZE, sizeof(char));

    printf("Count of cmsg file descriptors: %lu\n", cmsg_count_descriptors);

    for (int i = 0; i < cmsg_count_descriptors; i++) {
        // Get file descriptor
        int *fd = &cmsg_descriptors[i];

        // We check the file descriptor on NULL, because after alignment we can get a 0 descriptor.
        if (*fd != 0) {
            // Print file descriptor
            printf("Received file descriptor: %d\n", cmsg_descriptors[i]);

            // Read from descriptor and save read data to buffer
            if ((read(*fd, data, (size_t) BUFF_SIZE)) == -1) {
                perror("\n\nread");
                return -1;
            }

            // Print read data
            printf("Read data: %s\n", data);

            // Clean buffer
            memset(data, 0, (size_t) BUFF_SIZE);

            // Close descriptor
            close(cmsg_descriptors[i]);

            // Increase count of read descriptors
            ++count_descriptors;
        } else {
            // Break from cycle
            break;
        }
    }

    // Print real count of file descriptors
    printf("Count of real file descriptors: %zu\n", count_descriptors);

    // Clean memory
    free(data);

    return 0;
}

size_t get_count_descriptors(socklen_t cmsg_len) {
    // Get control message data length
    size_t cmsg_data_length = (size_t) cmsg_len - (size_t) CMSG_LEN(0);
    // Return count of file descriptors from control message with align area
    return cmsg_data_length / sizeof(int);
}

int process_cmsg(struct cmsghdr* cmsg) {
    if (cmsg != NULL) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {

            printf("Total current SCM_RIGHTS length: %lu\n", (size_t) cmsg->cmsg_len);

            // Declaration and assign count of file descriptors with align area
            size_t cmsg_count_descriptors = get_count_descriptors(cmsg->cmsg_len);

            // Declaration and assign array of file descriptors
            int *cmsg_descriptors = calloc(cmsg_count_descriptors, sizeof(int));

            memcpy(cmsg_descriptors, CMSG_DATA(cmsg), sizeof(int) * cmsg_count_descriptors);

            return read_descriptors(cmsg_descriptors, cmsg_count_descriptors);
        }
        printf("Total current cmsg length: %lu\n", (size_t) cmsg->cmsg_len);
    }

    return 0;
}

int main() {
    // Remove socket
    unlink(SOCKET_PATH);

    // Set buffer for data receive
    char *iov_buffer = calloc((size_t) BUFF_SIZE, sizeof(char));
    // Set control buffer for receive data
    char *control_buffer = calloc((size_t) BUFF_SIZE, sizeof(char));

    // Declaration and assign socket descriptor
    int socket_file_descriptor = -1;
    // Declaration and assign client descriptor
    int client_file_descriptor = -1;
    // Declaration and assign sender address struct size
    socklen_t sender_address_size = sizeof(struct sockaddr_un);
    // Declaration and assign sender message address struct size
    socklen_t sender_message_address_size = sizeof(struct sockaddr_storage);

    // Declaration and assign input/output vector
    struct iovec iov = {0};
    // Declaration and assign message header
    struct msghdr message = {0};
    // Declaration and assign socket address unix
    struct sockaddr_un socket_address = {0};
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
    socket_file_descriptor = socket(AF_UNIX, SOCK_SEQPACKET, F_UNIX);
    if (socket_file_descriptor == -1) {
        perror("\n\nsocket");
        return 1;
    }

    // Set socket socket address
    socket_address.sun_family = PF_UNIX;
    strcpy(socket_address.sun_path, SOCKET_PATH);

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

    // Get sender address info from accept
    debug_sock_unix(&sender_address_size, (struct sockaddr_un *) &sender_address, "accept");

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
    close(client_file_descriptor);
    close(socket_file_descriptor);

    // Clean memory
    free(cmsg);
    free(iov_buffer);

    // Remove socket
    unlink(SOCKET_PATH);

    return 0;
}
