#ifndef GAME_STATE_H
#define GAME_STATE_H 

#include <vector> 
#include "./player.h"
#include "./question.h"

class GameState {
public: 
    std::vector<Player> players; 
    std::vector<Question> questions; 
    int current_question_index = 0; 

    bool is_game_over() const {}

private: 

}; 

#endif 