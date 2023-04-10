#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <vector>
#include <unordered_set>
#include "./player.h"
#include "./question.h"

class GameState {
public:
    GameState(const std::unordered_set<std::string>& player_names, const std::vector<Question>& questions)
        : players(create_players(player_names)), questions(questions), current_question_index(0) {}

    std::vector<Player> players;
    std::vector<Question> questions;
    int current_question_index = 0;

    // bool is_game_over() const {
    //     // Implement game over condition

    // }

private:
    std::vector<Player> create_players(const std::unordered_set<std::string>& player_names) {
        std::vector<Player> temp_players;
        for (const auto& name : player_names) {
            temp_players.emplace_back(name, true, false);
        }
        return temp_players;
    }
};

#endif
