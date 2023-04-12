#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <thread>
#include <atomic>

constexpr int PORT = 8080;

std::atomic<bool> waiting_for_answer(false);

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

void handle_user_input(int client_fd) {
    while (true) {
        if (waiting_for_answer) {
            char answer;
            std::cout << "Enter your answer (A/B/C/D) or 'S' to skip: ";
            std::cin >> answer;
            answer = std::toupper(answer); // Convert answer to uppercase
            send(client_fd, &answer, sizeof(answer), 0);
            waiting_for_answer = false;
        }
    }
}

void game_loop(int client_fd) {
    std::thread user_input_thread(handle_user_input, client_fd);

    while (true) {
        char buffer[1024];
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        buffer[bytes_received] = '\0';
        std::string message(buffer);

        if (message.find("it's your turn!") != std::string::npos) {
            std::cout << message << std::endl;
            waiting_for_answer = true;
        } else {
            std::cout << message << std::endl;
            if (message.find("TIME'S UP") != std::string::npos) {
                waiting_for_answer = false;
            }
        }
    }

    user_input_thread.join();
}


// void game_loop(int client_fd) {
//     pollfd client_pollfd;
//     client_pollfd.fd = client_fd;
//     client_pollfd.events = POLLIN;

//     while (true) {
//         poll(&client_pollfd, 1, -1);

//         if (client_pollfd.revents & POLLIN) {
//             char buffer[1024];
//             ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
//             buffer[bytes_received] = '\0';
//             std::string message(buffer);

//             if (message.find("it's your turn!") != std::string::npos) {
//                 std::cout << message << std::endl;

//                 char answer;
//                 std::cout << "Enter your answer (A/B/C/D) or 'S' to skip: ";
//                 std::cin >> answer;
//                 answer = std::toupper(answer); // Convert answer to uppercase
//                 send(client_fd, &answer, sizeof(answer), 0);
//             } else {
//                 std::cout << message << std::endl;
//             }
//         }
//     }
// }

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
            } else if (response.find("GAME INFORMATION") != std::string::npos) {
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