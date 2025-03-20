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
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFF_SIZE 65535
#define RECEIVER_PORT 54321

void debug_sock_v4(const socklen_t* address_size, const struct sockaddr_in* address, char* from) {
    printf("\nSender size (%s): %u\n", from, *address_size);
    printf("Sender family (%s): %hu\n", from, address->sin_family);
    printf("Sender port (%s):: %hu\n", from, ntohs(address->sin_port));

    char *ip_str = calloc(INET_ADDRSTRLEN, sizeof(char));
    if (ip_str == NULL) {
        perror("\n\ncalloc");
        exit(EXIT_FAILURE);
    }

    inet_ntop(AF_INET, &(address->sin_addr), ip_str, INET_ADDRSTRLEN);
    printf("Sender address (%s): %s\n", from, ip_str);

    printf("Sender zero (%s): ", from);
    for (int i = 0; i < sizeof(address->sin_zero); i++) {
        printf("%hhu ", address->sin_zero[i]);
    }
    printf("\n\n");

    // Clean memory
    free(ip_str);
}

int main() {
    // Set buffer for data receive
    char *buffer = calloc((size_t) BUFF_SIZE, sizeof(char));

    // Declaration and assign socket descriptor
    int socket_file_descriptor = -1;
    // Declaration and assign client descriptor
    int client_file_descriptor = -1;
    // Declaration and assign sender address struct size
    socklen_t sender_address_size = sizeof(struct sockaddr_storage);
    // Declaration and assign sender message address struct size
    socklen_t sender_recvfrom_address_size = sizeof(struct sockaddr_storage);

    // Declaration and assign socket address unix
    struct sockaddr_in socket_address = {0};
    // Declaration and assign client socket address
    struct sockaddr_storage sender_address = {0};
    // Declaration and assign client socket address from message
    struct sockaddr_storage sender_recvfrom_address = {0};

    // Clean buffer
    memset(&sender_address, 0, sizeof(sender_address));
    memset(&socket_address, 0, sizeof(socket_address));

    // Create socket
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_file_descriptor == -1) {
        perror("\n\nsocket");
        return 1;
    }

    // Set socket socket address
    socket_address.sin_family = PF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(RECEIVER_PORT);

    // Bind socket to address
    if (bind(socket_file_descriptor, (struct sockaddr *) &socket_address, sizeof(socket_address)) == -1) {
        perror("\n\nbind");
        return 1;
    }

    // Listen for incoming connections
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
    debug_sock_v4(&sender_address_size, (const struct sockaddr_in *) &sender_address, "accept");

    // Receive data
    ssize_t received = recvfrom(client_file_descriptor, buffer, (size_t) BUFF_SIZE, 0,
                                (struct sockaddr *) &sender_recvfrom_address, &sender_recvfrom_address_size);
    if (received == -1) {
        perror("\n\nrecvfrom");
        return 1;
    }

    // Get sender address info from message
    debug_sock_v4(&sender_recvfrom_address_size, (struct sockaddr_in *) &sender_recvfrom_address, "recvmsg");

    printf("Received data: %s\n", buffer);

    // Close sockets
    close(client_file_descriptor);
    close(socket_file_descriptor);

    // Clean memory
    free(buffer);

    return 0;
}
