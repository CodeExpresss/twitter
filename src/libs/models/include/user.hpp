#ifndef USER_HPP
#define USER_HPP

#include <utility>

#include "models_header.hpp"

class User {
public:
    User(int id, std::string _email, std::string _password): user_id(id), password(std::move(_password)), email(std::move(_email)) {};
    User() = default;
    ~User() = default;

    bool login();
    bool logout();
    bool sign_up();
    bool change_user_data();
    //bool is_active();

    std::string get_email();

private:
    int user_id;
    std::string password;
    std::string email;
    bool is_active;
    std::string session;

    bool change_password();
    bool change_email();
    //bool set_username();
};

#endif
