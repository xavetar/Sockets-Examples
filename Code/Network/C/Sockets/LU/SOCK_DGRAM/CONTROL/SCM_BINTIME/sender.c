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
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#define F_UNIX 0
#define BUFF_SIZE 65535
#define SOCKET_PATH "/tmp/SENDER"
#define TARGET_SOCKET_PATH "/tmp/RECEIVER"

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
    #define __BSD__
#endif

int main() {
#if defined(__BSD__)
    // WARNING
    printf("\033[33mWarning! The network stack does not support getting SCM_BINTIME for SOCK_DGRAM with"
           " PF_UNIX.\033[0m\n");
#endif
    // Remove socket
    unlink(SOCKET_PATH);

    // Declaration and assign socket descriptor
    int socket_file_descriptor = -1;

    // Declaration and assign socket address unix
    struct sockaddr_un socket_address = {0};
    // Declaration and assign target socket address unix
    struct sockaddr_un target_socket_address = {0};

    // Clean buffer
    memset(&socket_address, 0, sizeof(socket_address));
    memset(&target_socket_address, 0, sizeof(target_socket_address));

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

    // Send data
    const char* message = "Hello, receiver!";
    ssize_t bytes_sent = sendto(socket_file_descriptor, message, strlen(message), 0,
                                (struct sockaddr *) &target_socket_address, sizeof(target_socket_address));
    if (bytes_sent == -1) {
        perror("\n\nsendto");
        return 1;
    }

    printf("Message sent: %s\n", message);

    close(socket_file_descriptor);

    return 0;
}