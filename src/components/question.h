#ifndef QUESTION_H
#define QUESTION_H

#include <string> 
#include <vector> 

class Question {
public: 
    Question(const std::string& question_text, const std::vector<std::string>& choices, int correct_choice_index) : question_text(question_text), choices(choices), correct_choice_index(correct_choice_index) {}

    std::string get_question_text() const { return question_text; }
    std::vector<std::string> get_choices() const { return choices; }
    int get_correct_choice_index() const { return correct_choice_index; }
private: 
    std::string question_text; 
    std::vector<std::string> choices; 
    int correct_choice_index; 
}; 

#endif 