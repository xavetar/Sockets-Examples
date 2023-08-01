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
#if !defined(__linux__)
    printf("Sender length (%s): %i\n", from, address->sun_len);
#endif
    printf("Sender unix socket path (%s): %s\n", from, address->sun_path);
    printf("Sender family (%s): %hu\n\n", from, address->sun_family);
}

int main() {
    // Remove socket
    unlink(SOCKET_PATH);

    // Set buffer for data receive
    char *buffer = calloc(BUFF_SIZE, sizeof(char));

    // Declaration and assign socket descriptor
    int socket_file_descriptor = -1;
    // Declaration and assign sender message address struct size
    socklen_t sender_recvfrom_address_size = sizeof(struct sockaddr_storage);

    // Declaration and assign socket address unix
    struct sockaddr_un socket_address = {0};
    // Declaration and assign client socket address from message
    struct sockaddr_storage sender_recvfrom_address = {0};

    // Clean buffer
    memset(&socket_address, 0, sizeof(socket_address));
    memset(&sender_recvfrom_address, 0, sizeof(sender_recvfrom_address));

    // Create socket
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

    // Receive data
    ssize_t received = recvfrom(socket_file_descriptor, buffer, (size_t) BUFF_SIZE, 0,
                                (struct sockaddr *) &sender_recvfrom_address, &sender_recvfrom_address_size);
    if (received == -1) {
        perror("\n\nrecvfrom");
        return 1;
    }

    // Get sender address info from accept
    debug_sock_unix(&sender_recvfrom_address_size, (struct sockaddr_un *) &sender_recvfrom_address, "recvfrom");

    printf("Received data: %s\n", buffer);

    // Close socket
    close(socket_file_descriptor);

    // Clean memory
    free(buffer);

    return 0;
}
