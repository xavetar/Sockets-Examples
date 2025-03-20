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
#include <fcntl.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/socket.h>

// Apple on Ventura 13.5 not support sending multiple cmsghdr with SCM_RIGHTS!

#define ONE_CMSGHDR 1

#define F_UNIX 0
#define BUFF_SIZE 65535
#define FP "../Test Files/"
#define SOCKET_PATH "/tmp/SENDER"
#define TARGET_SOCKET_PATH "/tmp/RECEIVER"

char* concat(const char* str1, const char* str2) {
    // Alloc memory for new string
    char *result = calloc(strlen(str1) + strlen(str2) + 1, sizeof(char));
    if (result == NULL) {
        perror("\n\ncalloc");
        exit(EXIT_FAILURE);
    }

    // Copy and concatenate strings
    strcpy(result, str1); strcat(result, str2);

    return result;
}

int* open_descriptors(char* folder_path, char** filenames, size_t* array_size) {
    // Declaration and assign current descriptor
    int fd = 0;
    // Logical allocate address in heap for realloc without size
    int *file_fds = NULL;

    // Iteration on filenames
    for (void *i = (void *) 1; i != NULL;) {
        // Clean memory
        memset(&fd, 0, sizeof(fd));
        // Get index from pointer i
        intptr_t index = (intptr_t) i - 1;

        // Check current index of element on NULL
        if (filenames[index] != NULL) {
            char *file_path = concat(folder_path, filenames[index]);

            // Open file and get file descriptor
            fd = open(file_path, O_RDONLY);
            if (fd == -1) {
                perror("\n\nopen");
                exit(EXIT_FAILURE);
            }

            // Realloc memory for array of file descriptors
            int *new_mem_file_fds = realloc(file_fds, (*array_size + 1) * sizeof(int));
            if (new_mem_file_fds == NULL) {
                perror("\n\nrealloc");
                exit(EXIT_FAILURE);
            }
            file_fds = new_mem_file_fds;

            // Assign file descriptor to array of file descriptors
            file_fds[*array_size] = fd;

            // Increase size of array
            *array_size += 1; i++;

            // Info about filename and array index
            if (!index) {
                printf("Index::Filename: %li::%s", index, filenames[index]);
            } else {
                printf(", %li::%s", index, filenames[index]);
            }

            // Clean memory
            free(file_path);
        } else {
            // Assign NULL pointer i for end iteration
            i = NULL;
        }
    }
    return file_fds;
}

int main() {
    // Remove socket
    unlink(SOCKET_PATH);

    // Declaration and assign io vector
    char *iov_base_buff = calloc(BUFF_SIZE, sizeof(char));
    char *iov_base_buff_s = calloc(BUFF_SIZE, sizeof(char));

    // Declaration and assign count of descriptors
    size_t file_count = 0;

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

    // Clean buffer
    memset(&iov, 0, sizeof(iov));
    memset(&message, 0, sizeof(message));
    memset(&socket_address, 0, sizeof(socket_address));
    memset(&target_socket_address, 0, sizeof(target_socket_address));

    // Filenames
    char *filenames[] = {
            "file.txt",
            "file1.txt",
            "file2.txt",
            "file3.txt",
            NULL
    };

    // Array descriptors
    int *file_fds = open_descriptors(FP, filenames, &file_count);
    printf("\n\nCount descriptors: %zu\n\n", file_count);
    for (int i = 0; i < file_count; i++) {
        if (!i) { printf("Index::Descriptor: %i::%i", i, file_fds[i]); } else { printf(", %i::%i", i, file_fds[i]); }
    }

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

    // Prepare ancillary data (control message)
    snprintf(iov_base_buff, (size_t) BUFF_SIZE, "Hello, receiver! ");
    snprintf(iov_base_buff_s, (size_t) BUFF_SIZE, "I'm sender, my pid: %d!", getpid());

    // Init iovec
    iov[0] = (struct iovec) { .iov_base = iov_base_buff, .iov_len = strlen(iov_base_buff) };
    iov[1] = (struct iovec) { .iov_base = iov_base_buff_s, .iov_len = strlen(iov_base_buff_s) };

    // Init msghdr
    message = (struct msghdr) {
        .msg_name = &target_socket_address, .msg_namelen = sizeof(target_socket_address),
        .msg_iov = iov, .msg_iovlen = sizeof(iov) / sizeof(iov[0])
    };

#if ONE_CMSGHDR == 0
    // WARNING
    printf("\n\n\033[31mWarning! The Apple networking stack is not support sending multiple SCM_RIGHTS cmsghdr.\033[0m\n");

    // Calculate total length of control messages
    size_t cmsg_total_size = 0;
    for (int i = 0; i < file_count; i++) {
        cmsg_total_size += CMSG_SPACE(sizeof(int));
    }
#elif ONE_CMSGHDR == 1
    // Calculate total length of control messages with alignment
    size_t cmsg_total_size = CMSG_SPACE(sizeof(int) * file_count);
#endif

    // Allocate memory for the control messages buffer
    char *cmsgbuf = calloc(cmsg_total_size, sizeof(char));
    if (cmsgbuf == NULL) {
        perror("\n\ncalloc");
        return 1;
    }

    // Initialize pointer to the current position in the control messages buffer
    char *cmsg_ptr = cmsgbuf;

#if ONE_CMSGHDR == 0
    // Populate the control messages
    for (int i = 0; i < file_count; i++) {
        int file_fd = file_fds[i];

        printf("\n\nPopulate the control messages: %i", i);

        // Calculate the size of the current control message
        size_t cmsg_size = CMSG_LEN(sizeof(int));

        // Prepare the control message to send the file descriptor
        struct cmsghdr *cmsg = (struct cmsghdr *) cmsg_ptr;
        cmsg->cmsg_len = cmsg_size;
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        memcpy(CMSG_DATA(cmsg), &file_fd, sizeof(int));

        // Move the pointer to the next position
        cmsg_ptr += CMSG_SPACE(sizeof(int));
    }
#elif ONE_CMSGHDR == 1
    // Prepare the control message to send the file descriptor
    struct cmsghdr *cmsg = (struct cmsghdr *)cmsg_ptr;

    // Calculate the size of the current control message
    size_t cmsg_size = CMSG_LEN(sizeof(int) * file_count);

    *cmsg = (struct cmsghdr) { .cmsg_len = cmsg_size, .cmsg_level = SOL_SOCKET, .cmsg_type = SCM_RIGHTS };

    memcpy(CMSG_DATA(cmsg), file_fds, sizeof(int) * file_count);
#endif

    // Set the control message buffer and length in the message header
    message.msg_control = cmsgbuf;
    message.msg_controllen = cmsg_total_size;

    // Send the message with the file descriptors
    size_t send_size = sendmsg(socket_file_descriptor, &message, 0);
    if (send_size == -1) {
        perror("\n\nsendmsg");
        return 1;
    }
    printf("\n\nSend size: %lu\n", send_size);

    // Close file descriptors and socket
    for (int i = 0; i < file_count; i++) {
        close(file_fds[i]);
    }
    close(socket_file_descriptor);

    // Clean memory
    free(cmsgbuf);
    free(file_fds);
    free(iov_base_buff);
    free(iov_base_buff_s);

    return 0;
}
