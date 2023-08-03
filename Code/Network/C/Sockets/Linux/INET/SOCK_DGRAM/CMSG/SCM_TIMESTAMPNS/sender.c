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
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define CONNECT 0
#define SENDER_PORT 12345
#define RECEIVER_PORT 54321

int main() {
    // Declaration and assign socket descriptor
    int socket_file_descriptor = -1;

    // Declaration and assign socket address unix
    struct sockaddr_in socket_address = {0};
    // Declaration and assign target socket address unix
    struct sockaddr_in target_socket_address = {0};

    // Clean buffer
    memset(&socket_address, 0, sizeof(socket_address));

    // Create socket
    socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_file_descriptor == -1) {
        perror("\n\nsocket");
        return 1;
    }

    // Set socket socket address
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(SENDER_PORT);

    // Bind socket to socket address
    if (bind(socket_file_descriptor, (struct sockaddr *) &socket_address, sizeof(socket_address)) == -1) {
        perror("\n\nbind");
        return 1;
    }

    // Set target socket address
    target_socket_address.sin_family = AF_INET;
    target_socket_address.sin_addr.s_addr = INADDR_ANY;
    target_socket_address.sin_port = htons(RECEIVER_PORT);

#if CONNECT == 1
    // Connect to socket
    if (connect(
            socket_file_descriptor, (struct sockaddr *) &target_socket_address, sizeof(target_socket_address)
    ) == -1) {
        perror("\n\nconnect");
        return 1;
    }
#endif

    // Send data
    const char* message = "Hello, receiver!";
#if CONNECT == 0
    ssize_t bytes_sent = sendto(socket_file_descriptor, message, strlen(message), 0,
                                (struct sockaddr *) &target_socket_address, sizeof(target_socket_address));
#elif CONNECT == 1
    ssize_t bytes_sent = send(socket_file_descriptor, message, strlen(message), 0);
#endif
    if (bytes_sent == -1) {
        perror("\n\nsendto");
        return 1;
    }

    printf("Message sent: %s\n", message);

    // Close socket
    close(socket_file_descriptor);

    return 0;
}
