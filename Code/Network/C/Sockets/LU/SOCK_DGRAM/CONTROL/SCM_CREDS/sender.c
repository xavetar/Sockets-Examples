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
#define SOCKET_PATH "/tmp/SENDER"
#define TARGET_SOCKET_PATH "/tmp/RECEIVER"

int main() {
    // Remove socket
    unlink(SOCKET_PATH);

    // Declaration and assign io vector
    char *iov_base_buff = calloc(BUFF_SIZE, sizeof(char));
    char *iov_base_buff_s = calloc(BUFF_SIZE, sizeof(char));

    // Declaration and assign socket descriptor
    int socket_file_descriptor = -1;

    // Declaration and assign input/output vector
    struct iovec iov[2] = {0};
    // Declaration and assign message header
    struct msghdr message = {0};
    // Declaration and assign socket address unix
    struct sockaddr_un socket_address = {0};
    // Declaration and assign target socket address unix
    struct sockaddr_un target_socket_address = {0};
    // Declaration and assign control message credentials
    struct cmsgcred control_message_credentials = {0};

    // Clean buffer
    memset(&iov, 0, sizeof(iov));
    memset(&message, 0, sizeof(message));
    memset(&socket_address, 0, sizeof(socket_address));
    memset(&target_socket_address, 0, sizeof(target_socket_address));
    memset(&control_message_credentials, 0, sizeof(struct cmsgcred));

    // Create socket.
    socket_file_descriptor = socket(AF_UNIX, SOCK_DGRAM, F_UNIX);
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

    // Set target socket address
    target_socket_address.sun_family = PF_UNIX;
    strcpy(target_socket_address.sun_path, TARGET_SOCKET_PATH);

    // Connect to socket
    if (connect(
            socket_file_descriptor, (struct sockaddr *) &target_socket_address, sizeof(target_socket_address)
    ) == -1) {
        perror("\n\nconnect");
        return 1;
    }

    // Prepare ancillary data (control message)
    snprintf(iov_base_buff, (size_t) BUFF_SIZE, "Hello, receiver! ");
    snprintf(iov_base_buff_s, (size_t) BUFF_SIZE, "I'm sender, my pid: %d!", getpid());

    // Init iovec
    iov[0] = (struct iovec) { .iov_base = iov_base_buff, .iov_len = strlen(iov_base_buff) };
    iov[1] = (struct iovec) { .iov_base = iov_base_buff_s, .iov_len = strlen(iov_base_buff_s) };

    // Init msghdr
    message = (struct msghdr) { .msg_iov = iov, .msg_iovlen = sizeof(iov) / sizeof(iov[0]) };

    // Calculate total length of control messages with alignment
    ssize_t cmsg_total_size = CMSG_SPACE(sizeof(struct cmsgcred));

    // Allocate memory for the control messages buffer
    char *cmsgbuf = calloc(cmsg_total_size, sizeof(char));
    if (cmsgbuf == NULL) {
        perror("\n\ncalloc");
        return 1;
    }

    // Initialize pointer to the current position in the control messages buffer
    char *cmsg_ptr = cmsgbuf;

    // Prepare the control message to send the file descriptor
    struct cmsghdr *cmsg = (struct cmsghdr *) cmsg_ptr;
    cmsg->cmsg_len = CMSG_LEN(sizeof(struct cmsgcred));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_CREDS;
    memcpy(CMSG_DATA(cmsg), &control_message_credentials, sizeof(struct cmsgcred));

    // Set the control message buffer and length in the message header
    message.msg_control = cmsgbuf;
    message.msg_controllen = cmsg_total_size;

    // Send the message with the file descriptors
    ssize_t send_size = sendmsg(socket_file_descriptor, &message, 0);
    if (send_size == -1) {
        perror("\n\nsendmsg");
        return 1;
    }
    printf("Send size: %lu\n", send_size);

    // Close socket descriptor
    close(socket_file_descriptor);

    // Clean memory
    free(iov_base_buff);
    free(iov_base_buff_s);

    return 0;
}
