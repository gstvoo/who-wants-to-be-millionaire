#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

constexpr int PORT = 8080;

int create_client_socket() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    return client_fd;
}

std::string get_nickname() {
    std::string nickname;
    std::cout << "Enter your nickname: ";
    std::cin >> nickname;
    return nickname;
}

int main() {
    int client_fd = create_client_socket();
    std::string nickname = get_nickname();

    ssize_t bytes_sent = send(client_fd, nickname.c_str(), nickname.size() + 1, 0);

    if (bytes_sent < 0) {
        perror("send");
    } else {
        std::cout << "Sent " << bytes_sent << " bytes." << std::endl;
    }

    char buffer[1024];

    while (true) {
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string response(buffer);
            std::cout << response << std::endl;

            if (response == "Nickname already exists. Please choose another nickname.") {
                nickname = get_nickname();
                send(client_fd, nickname.c_str(), nickname.size() + 1, 0);
            }
        } else if (bytes_received == 0) {
            // If recv returns 0, it means that the server closed the connection.
            break;
        } else if (bytes_received < 0) {
            // If recv returns a negative value, it means that there was an error.
            perror("recv");
            break;
        }
    }

    close(client_fd);
    return 0;
}

