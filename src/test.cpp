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

int main() {
    int client_count = 3; 
    std::vector<Question> questions = load_questions("../data/questions.txt", client_count);
    std::cout << questions.size() << '\n';
}