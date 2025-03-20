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

#define LOOP_BACK 1
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

int main() {
    // Set buffer for data receive
    char *buffer = calloc(BUFF_SIZE, sizeof(char));

    // Declaration and assign socket descriptor
    int socket_file_descriptor = -1;
    // Declaration and assign sender message address struct size
    socklen_t sender_recvfrom_address_size = sizeof(struct sockaddr_storage);

    // Declaration and assign socket address unix
    struct sockaddr_in6 socket_address = {0};
    // Declaration and assign client socket address from message
    struct sockaddr_storage sender_recvfrom_address = {0};

    // Clean buffer
    memset(&socket_address, 0, sizeof(socket_address));
    memset(&sender_recvfrom_address, 0, sizeof(sender_recvfrom_address));

    // Create socket
    socket_file_descriptor = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_file_descriptor == -1) {
        perror("\n\nsocket");
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

    // Receive data
    ssize_t received = recvfrom(socket_file_descriptor, buffer, (size_t) BUFF_SIZE, 0,
                                (struct sockaddr *) &sender_recvfrom_address, &sender_recvfrom_address_size);
    if (received == -1) {
        perror("\n\nrecvfrom");
        return 1;
    }

    // Print info about sender
    debug_sock_v6(&sender_recvfrom_address_size, (const struct sockaddr_in6 *) &sender_recvfrom_address, "recvfrom");

    printf("Received data: %s\n", buffer);

    // Close socket
    close(socket_file_descriptor);

    // Clean memory
    free(buffer);

    return 0;
}
