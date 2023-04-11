#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>

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

void game_loop(int client_fd) {
    pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;

    while (true) {
        poll(&client_pollfd, 1, -1);

        if (client_pollfd.revents & POLLIN) {
            char buffer[1024];
            ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            buffer[bytes_received] = '\0';
            std::string message(buffer);

            if (message.find("it's your turn!") != std::string::npos) {
                std::cout << message << std::endl;

                char answer;
                std::cout << "Enter your answer (A/B/C/D) or 'S' to skip: ";
                std::cin >> answer;
                answer = std::toupper(answer); // Convert answer to uppercase
                send(client_fd, &answer, sizeof(answer), 0);
            } else {
                std::cout << message << std::endl;
            }
        }
    }
}

int main() {
    int client_fd = create_client_socket();
    std::string nickname = get_nickname();
    ssize_t bytes_sent = send(client_fd, nickname.c_str(), nickname.size() + 1, 0);
    
    if (bytes_sent < 0) {
        perror("send");
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
            } else if (response.find("Game Information:") != std::string::npos) {
                game_loop(client_fd);
                break;
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