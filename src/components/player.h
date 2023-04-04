#ifndef PLAYER_H
#define PLAYER_H

#include <string> 
class Player {
public: 
    Player(const std::string& nickname, bool is_active, bool has_skipped) : nickname(nickname), is_active(is_active), has_skipped(has_skipped) {}

    std::string get_nickname() const { return nickname; }
    bool get_is_active() const { return is_active; }
    bool get_has_skipped() const { return has_skipped; }

    void set_nickname(const std::string& new_nickname) { nickname = new_nickname; }
    void set_is_active(const bool& new_is_active) { is_active = new_is_active; }
    void set_has_skipped(const bool& new_has_skipped) { has_skipped = new_has_skipped; }
private: 
    std::string nickname; 
    bool is_active; 
    bool has_skipped; 
};


#endif 