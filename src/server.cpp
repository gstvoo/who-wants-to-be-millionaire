#include <iostream>
#include <string>
#include <unordered_set>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <poll.h>
#include <vector>

constexpr int PORT = 8080;
constexpr int BACKLOG = 10;

int create_server_socket() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, BACKLOG);

    return server_fd;
}

std::string handle_client_registration(int client_fd, std::unordered_set<std::string>& nicknames, int& client_count, int required_players, std::vector<pollfd>& fds) {
    char buffer[1024];
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    buffer[bytes_received] = '\0';

    std::string nickname(buffer);
    if (nicknames.find(nickname) == nicknames.end()) {
        nicknames.insert(nickname);
        client_count++;
        std::string response = "Registration Completed Successfully. Number of clients: " + std::to_string(client_count);
        if (client_count == required_players) {
            response += "\nGame start";
            for (size_t i = 1; i < fds.size(); ++i) {
                send(fds[i].fd, "Game start", strlen("Game start") + 1, 0);
            }
        }
        return response;
    } else {
        return "Nickname already exists. Please choose another nickname.";
    }
}


void broadcast_client_info(const std::vector<pollfd>& fds, const std::unordered_set<std::string>& nicknames, int client_count) {
    std::string client_info = "Current number of clients: " + std::to_string(client_count) + "\n";
    client_info += "Client nicknames: ";

    for (const auto& nickname : nicknames) {
        client_info += nickname + ", ";
    }
    client_info = client_info.substr(0, client_info.size() - 2); // Remove trailing comma and space
    client_info += "\n";

    for (size_t i = 1; i < fds.size(); ++i) {
        send(fds[i].fd, client_info.c_str(), client_info.size() + 1, 0);
    }
}

int get_num_players_to_start() {
    int num_players;
    std::cout << "Enter the number of players required to start the game: ";
    std::cin >> num_players;
    return num_players;
}

void send_game_start_notification(const std::vector<pollfd>& fds) {
    std::string game_start_message = "THE GAME IS STARTING";
    for (size_t i = 1; i < fds.size(); ++i) {
        send(fds[i].fd, game_start_message.c_str(), game_start_message.size() + 1, 0);
    }
}


int main() {
    int server_fd = create_server_socket();
    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    std::unordered_set<std::string> nicknames;
    int client_count = 0;
    int num_players_to_start = get_num_players_to_start();

    std::vector<pollfd> fds;
    pollfd server_pollfd;
    server_pollfd.fd = server_fd;
    server_pollfd.events = POLLIN;
    fds.push_back(server_pollfd);

    

    while (true) {
        poll(fds.data(), fds.size(), -1);

        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (client_fd >= 0) {
                fcntl(client_fd, F_SETFL, O_NONBLOCK);
                pollfd client_pollfd;
                client_pollfd.fd = client_fd;
                client_pollfd.events = POLLIN;
                fds.push_back(client_pollfd);
            }
        }

        for (size_t i = 1; i < fds.size(); ++i) {
            if (fds[i].revents & POLLIN) {
                std::string response = handle_client_registration(fds[i].fd, nicknames, client_count, num_players_to_start, fds);
                send(fds[i].fd, response.c_str(), response.size() + 1, 0);

                if (client_count == num_players_to_start) {
                    send_game_start_notification(fds);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
