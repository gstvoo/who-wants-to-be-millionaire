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
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>
#include <random>
#include <memory> 

#include "./components/question.h"
#include "./components/game_state.h"
#include "./components/player.h"

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

std::string handle_client_registration(int client_fd, std::vector<std::string>& nicknames, int& client_count) {
    char buffer[1024];
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    buffer[bytes_received] = '\0';

    std::string nickname(buffer);
    if (std::find(nicknames.begin(), nicknames.end(), nickname) == nicknames.end()) {
        nicknames.push_back(nickname);
        client_count++;
        return "Registration Completed Successfully. Number of clients: " + std::to_string(client_count);
    } else {
        return "Nickname already exists. Please choose another nickname.";
    }
}

void broadcast_to_clients(const std::vector<pollfd>& fds, const std::string& message) {
    for (size_t i = 1; i < fds.size(); ++i) {
        send(fds[i].fd, message.c_str(), message.size() + 1, 0);
    }
}

void send_game_info(const std::vector<pollfd>& fds, const GameState& game_state) {
    std::string message = "GAME INFORMATION --------------------\n";
    message += "Number of players: " + std::to_string(game_state.players.size()) + "\n";
    message += "Players' order:";
    for (size_t i = 0; i < game_state.players.size(); ++i) {
        message += "\n" + std::to_string(i + 1) + ". " + game_state.players[i].get_nickname();
    }
    message += "\nNumber of questions in this set: " + std::to_string(game_state.questions.size()) + "\n";
    message += "GAME RULES ------------------------\n";
    message += "1.Each player will have 30s to answer the answer.\n";
    message += "2.Each player have 1 skip chance.\n";
    message += "3.Game ends when there's only 1 player left or all of the questions have been answered.\n";
    message += "----------------------------------\n";
    message += "THE GAME STARTS NOW!\n";
    broadcast_to_clients(fds, message);
}


std::string get_client_info(const std::vector<std::string>& nicknames, int client_count) {
    std::string client_info = "Current number of clients: " + std::to_string(client_count) + "\n";
    client_info += "Client nicknames: ";

    for (const auto& nickname : nicknames) {
        client_info += nickname + ", ";
    }
    client_info = client_info.substr(0, client_info.size() - 2); // Remove trailing comma and space
    client_info += "\n";
    return client_info;; 
}

int max_number_of_players() {
    std::cout << "Enter the number of players (2 to 10): "; 
    int players;
    std::cin >> players;
    while (players < 2 || players > 10) {
        std::cout << "Invalid number of players.\nEnter the number of players (2 to 10): ";
        std::cin >> players;
    }
    return players;
}

std::vector<Question> load_questions(const std::string& filename, int num_players) {
    std::vector<Question> all_questions;
    std::ifstream file(filename);

    if (file.is_open()) {
        std::string question_text, choice, blank_line;
        std::vector<std::string> choices;
        char correct_choice;

        while (std::getline(file, question_text)) {
            for (int i = 0; i < 4; ++i) {
                std::getline(file, choice);
                choices.push_back(choice.substr(2));
            }
            std::getline(file, choice); // Read the correct_choice line after the choices loop
            correct_choice = choice[0];
            std::getline(file, blank_line); // Read the blank line after the correct_choice line

            int correct_choice_index = correct_choice - 'A';
            all_questions.emplace_back(question_text, choices, correct_choice_index);
            choices.clear();
        }
        file.close();
    } else {
        std::cerr << "Unable to open file " << filename << std::endl;
    }

    int num_questions = num_players * 3;
    if (num_questions >= all_questions.size()) {
        return all_questions;
    }

    std::shuffle(all_questions.begin(), all_questions.end(), std::default_random_engine(std::random_device{}()));

    return std::vector<Question>(all_questions.begin(), all_questions.begin() + num_questions);
}

size_t count_active_players(const std::vector<Player>& players) {
    size_t active_players = 0;
    for (const auto& player : players) {
        if (player.get_is_active()) {
            active_players++;
        }
    }
    return active_players;
}


void game_loop(const std::vector<pollfd>& fds, GameState& game_state) {
    size_t current_question_index = 0;
    size_t current_player_index = 0;
    
    bool all_questions_answered = false; 

    while (current_question_index < game_state.questions.size()) {

        size_t active_players = count_active_players(game_state.players);
        if (active_players <= 1) {
            // The game is over
            break;
        }

        const Question& current_question = game_state.questions[current_question_index];
        Player& current_player = game_state.players[current_player_index];

        if (current_player.get_is_active()) {
            // Send the question to the current player
            std::string question_message = "Player \"" + current_player.get_nickname() + "\", it's your turn!\n";
            question_message += current_question.get_question_text() + "\n";
            for (size_t i = 0; i < current_question.get_choices().size(); ++i) {
                question_message += std::string(1, static_cast<char>('A' + i)) + ". " + current_question.get_choices()[i] + "\n";
            }
            send(fds[current_player_index + 1].fd, question_message.c_str(), question_message.size() + 1, 0);

            // Wait for the current player's answer
            pollfd current_player_pollfd = fds[current_player_index + 1];
            int poll_result = poll(&current_player_pollfd, 1, 30000); //30 seconds timeout

            if (poll_result > 0 && (current_player_pollfd.revents & POLLIN)) {
                char answer;
                recv(current_player_pollfd.fd, &answer, sizeof(answer), 0);
                if (answer == 'S') { // Player chose to skip their turn
                    if (!current_player.get_has_skipped()) {
                        current_player.set_has_skipped(true);
                    } else {
                        // Handle case where player tries to skip their turn more than once
                        std::string response_message = "YOU'VE ALREADY SKIPPED YOUR TURN. YOU'RE DISQUALIFIED\n";
                        send(fds[current_player_index+1].fd, response_message.c_str(), response_message.size() + 1, 0);
                        current_player.set_is_active(false);
                    }
                } else {
                    int answer_index = answer - 'A';
                    std::string response_message; 
                    if (answer_index == current_question.get_correct_choice_index()) {
                        // Correct answer
                        response_message = "CORRECT!\n"; 
                        current_question_index++;
                    
                        if (current_question_index == game_state.questions.size()) {
                            all_questions_answered = true; 
                            break;    
                        }

                    } else {
                        // Incorrect answer
                        response_message = "INCORREECT! YOU'RE DISQUALIFIED.\n";
                        current_player.set_is_active(false);
                    }
                    send(fds[current_player_index+1].fd, response_message.c_str(), response_message.size() + 1, 0);
                }
            } else if (poll_result == 0) {
                // Handle case where player doesn't answer in time
                std::string response_message = "\nTIME'S UP! YOU'RE DISQUALIFIED.\n";
                send(fds[current_player_index+1].fd, response_message.c_str(), response_message.size() + 1, 0);
                current_player.set_is_active(false);
            } else {
                // Handle case where poll() failed
                std::cerr << "poll() failed" << std::endl;
            }
        }

        // Move to the next player
        current_player_index = (current_player_index + 1) % game_state.players.size();
    }

    if (all_questions_answered) { // Check if the game ended with all questions answered correctly
        std::string end_message = "Congratulations to all players! You all are winners!\n";
        broadcast_to_clients(fds, end_message);
    }
}


int main() {
    int server_fd = create_server_socket();
    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // std::unordered_set<std::string> nicknames;
    std::vector<std::string> nicknames; 
    int client_count = 0;
    int required_players = max_number_of_players(); 

    std::vector<pollfd> fds;
    pollfd server_pollfd;
    server_pollfd.fd = server_fd;
    server_pollfd.events = POLLIN;
    fds.push_back(server_pollfd);

    std::unique_ptr<GameState> game_state;
    std::string filename = "../data/questions.txt"; 
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
                std::string response = handle_client_registration(fds[i].fd, nicknames, client_count);
                if (response == "Registration Completed Successfully. Number of clients: " + std::to_string(client_count)) {
                    std::string message = get_client_info(nicknames, client_count);
                    if(client_count == required_players) {
                        std::string message = get_client_info(nicknames, client_count); 
                        broadcast_to_clients(fds, message);

                        std::vector<Question> questions = load_questions(filename, client_count);
                        game_state = std::make_unique<GameState>(nicknames, questions);

                        send_game_info(fds, *game_state);
                        game_loop(fds, *game_state); 
                        // Declare the winner(s) after the game loop
                        size_t active_players = count_active_players(game_state->players);
                        std::string end_message;
                        if (active_players <= 1) {
                            for (const auto& player : game_state->players) {
                                if (player.get_is_active()) {
                                    end_message = "\nCongratulations, \"" + player.get_nickname() + "\"! You are the winner!\n";
                                    broadcast_to_clients(fds, end_message);
                                    break;
                                }
                            }
                        } else {
                            break; 
                        }
    
                    }
                    else {
                        message += "Waiting for more players to join.\n";
                        broadcast_to_clients(fds, message);
                    }
                        
                } else {
                    send(fds[i].fd, response.c_str(), response.size() + 1, 0);
                }
            }
        }

    }

    close(server_fd);
    return 0;
}
