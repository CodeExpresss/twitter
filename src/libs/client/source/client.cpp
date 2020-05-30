#include "client.hpp"

int profile_id;

std::shared_ptr<UnitOfWork> HTTPClient::worker = make_shared<UnitOfWork>();
//регулярные выражения для GET
std::regex HTTPClient::get_profile_regex = std::regex("/api/profile/.+");
std::regex HTTPClient::current_user_regex = std::regex("/api/user/current/");
std::regex HTTPClient::get_news_feed_regex = std::regex("/api/tweet/index/.+");
std::regex HTTPClient::get_subscription_regex = std::regex("/api/user/subscription/.+");
std::regex HTTPClient::tag_search_regex = std::regex("/api/tweet/tag/.+");
//регулярные выражения для POST
std::regex HTTPClient::register_regex = std::regex("/api/user/register/");
std::regex HTTPClient::login_regex = std::regex("/api/user/login/");
std::regex HTTPClient::user_update_regex = std::regex("/api/user/update/");
std::regex HTTPClient::profile_update_regex = std::regex("/api/profile/update/");
std::regex HTTPClient::create_tweet_regex = std::regex("/api/tweet/create/");
std::regex HTTPClient::vote_tweet_regex = std::regex("/api/tweet/vote/");
std::regex HTTPClient::follow_regex = std::regex("/api/profile/follow/");
std::regex HTTPClient::make_subscription_regex = std::regex("/api/user/make_subscription");

void HTTPClient::start() {
    read_request();
    check_deadline();
}

void HTTPClient::session() {
    std::string session_id;
    for (int i = request[http::field::cookie].find("=") + 1; i < request[http::field::cookie].size(); i++) {
        session_id += request[http::field::cookie][i];
    }

    std::shared_ptr<SessionController> cont = make_shared<SessionController>(worker);
    profile_id = cont->get_profile_id(session_id);
}

void HTTPClient::read_request() {
    auto self = shared_from_this();

    http::async_read(
            socket,
            buffer,
            request,
            [self](beast::error_code ec,
                   std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);
                if(!ec)
                    self->process_request();
            });
}

void HTTPClient::process_request() {
    response.version(request.version());
    response.keep_alive(false);

    session();

    response.set("Access-Control-Allow-Origin", "http://127.0.0.1:8080");
    response.set("Access-Control-Allow-Credentials", "true");

    switch (request.method()) {
        case http::verb::get:
            response.set(http::field::content_type, "application/json");
            response.result(http::status::ok);
            routing_get_method();
            break;
        case http::verb::post:
            response.result(http::status::ok);
            routing_post_method();
            break;
        default:
            // неопределённый метод запроса
            response.result(http::status::bad_request);
            response.set(http::field::content_type, "application/json");
            beast::ostream(response.body())
                    << "Invalid request-method '"
                    << std::string(request.method_string())
                    << "'";
            break;
    }
    write_response();
}

void HTTPClient::routing_post_method() {

    boost::property_tree::ptree json_response;
    boost::property_tree::ptree json_request;

    std::string request_string = request.target().to_string();
    std::stringstream ss;
    ss << request.body().data();
    boost::property_tree::read_json(ss, json_request);
    ss.str(std::string());

    if(profile_id == -1) {
        if (std::regex_match(request_string, register_regex)) { // регистрац~ия

//	std::shared_ptr<SignUpController<Serialize<Profile>>> cont =
//                make_shared<SignUpController<Serialize<Profile>>>(worker);
//
//        json_response = cont->get_queryset();


            boost::property_tree::json_parser::write_json(ss, json_response);
            beast::ostream(response.body()) << ss.str();

            return;

        } else if (std::regex_match(request_string, login_regex)) { // логин

            auto email = json_request.get<std::string>("email");
            auto password = json_request.get<std::string>("password");

            std::shared_ptr<LoginController<Serialize<std::pair<unsigned short int, std::string>>>> cont =
                    make_shared<LoginController<Serialize<std::pair<unsigned short int, std::string>>>>(worker,
                                                                                                        email, password);

            std::string session = cont->get_queryset();
            std::string query =
                    (boost::format("sessionid=%1%; HttpOnly; Path=/")
                     % session).str();
            response.set(http::field::set_cookie, query);

            json_response.put("status", 200);

            boost::property_tree::json_parser::write_json(ss, json_response);
            beast::ostream(response.body()) << ss.str();

            return;

        } else {
            std::cout << "Bad gateway" << std::endl;

            response.result(http::status::not_found);
            response.set(http::field::content_type, "text/plain");
            beast::ostream(response.body()) << "File not found";
        }
    }

    if(std::regex_match(request_string, follow_regex)) {
        auto inviter_id = json_request.get<int>("inviter_id");
        auto invitee_id = json_request.get<int>("invitee_id");

        std::shared_ptr<FollowController<Serialize<std::pair<unsigned short int, std::string>>>> cont =
                make_shared<FollowController<Serialize<std::pair<unsigned short int, std::string>>>>(worker, inviter_id, invitee_id);

        json_response = cont->get_queryset();

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

    } else if (std::regex_match(request_string, user_update_regex)) {

        auto id = json_request.get<int>("id");
        auto email = json_request.get<std::string>("email");
        auto password = json_request.get<std::string>("password");

        std::shared_ptr<UpdateUserController<Serialize<User>>> cont =
                make_shared<UpdateUserController<Serialize<User>>>(worker,
                                                                   id, email, password);

        json_response = cont->get_queryset();

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

        return;

    } else if (std::regex_match(request_string, profile_update_regex)) {

        auto id = json_request.get<int>("id");
        auto username = json_request.get<std::string>("username");
        auto birthday = json_request.get<std::string>("birthday");
        auto avatar = json_request.get<std::string>("avatar");

        std::shared_ptr<UpdateProfileController<Serialize<Profile>>> cont =
                make_shared<UpdateProfileController<Serialize<Profile>>>(worker,
                                                                         id, username, birthday, avatar);

        json_response = cont->get_queryset();

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

        return;

    } else if (std::regex_match(request_string, create_tweet_regex)) {

        auto text = json_request.get<std::string>("text");
        auto id = json_request.get<int>("id");

        std::shared_ptr<AddTweetController<Serialize<std::pair<unsigned short int, std::string>>>> cont =
                make_shared<AddTweetController<Serialize<std::pair<unsigned short int, std::string>>>>(worker,
                                                                                                       text, id);

        json_response = cont->get_queryset();

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

        return;

    } else if (std::regex_match(request_string, vote_tweet_regex)) {

        auto profile_id = json_request.get<int>("profile_id");
        auto tweet_id = json_request.get<int>("tweet_id");

        std::shared_ptr<VoteController<Serialize<std::pair<unsigned short int, std::string>>>> cont =
                make_shared<VoteController<Serialize<std::pair<unsigned short int, std::string>>>>(worker,
                                                                                                   profile_id, tweet_id);

        json_response = cont->get_queryset();

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

        return;

    } else if (std::regex_match(request_string, make_subscription_regex)) {

        auto inviter_id = json_request.get<int>("inviter_id");
        auto invitee_id = json_request.get<int>("invitee_id");

//        std::shared_ptr<VoteController<Serialize<std::pair<unsigned short int, std::string>>>> cont =
//                make_shared<VoteController<Serialize<std::pair<unsigned short int, std::string>>>>(worker,
//                                                                                                   profile_id, tweet_id);

//        json_response = cont->get_queryset();

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

    } else {
        std::cout << "Bad gateway" << std::endl;

        response.result(http::status::not_found);
        response.set(http::field::content_type, "text/plain");
        beast::ostream(response.body()) << "File not found";
    }
}


void HTTPClient::routing_get_method() {

    boost::property_tree::ptree json_response;

    std::string request_string = request.target().to_string();
    auto query_string_map = get_map_from_query( get_query_string(request_string) );
    std::stringstream ss;

    if(profile_id == -1) {
        std::cout << "Bad gateway" << std::endl;

        response.result(http::status::not_found);
        response.set(http::field::content_type, "text/plain");
        beast::ostream(response.body()) << "File not found";
        return;
    }

    if (request.target() == "/echo") {
        response.set(http::field::content_type, "text/html");
        beast::ostream(response.body())
                << request.method() << "\n"
                << request.target() << "\n";

    } else if (std::regex_match(request_string, current_user_regex)) {

        std::shared_ptr<GetProfileController<Serialize<Profile>>> cont =
                make_shared<GetProfileController<Serialize<Profile>>>(worker);

        json_response = cont->get_queryset(profile_id);

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

        return;

    } else if (std::regex_match(request_string, get_profile_regex)) {

        std::shared_ptr<GetProfileController<Serialize<Profile>>> cont =
                make_shared<GetProfileController<Serialize<Profile>>>(worker);

        json_response = cont->get_queryset(std::stoi(query_string_map["id"]));

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

        return;

    } else if (std::regex_match(request_string, get_subscription_regex)) {

        std::shared_ptr<SubscriptionController<Serialize<std::vector<Profile>>>> cont =
                make_shared<SubscriptionController<Serialize<std::vector<Profile>>>>(worker);

        json_response = cont->get_queryset(std::stoi(query_string_map["id"]));

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

        return;

    } else if (std::regex_match(request_string, get_news_feed_regex)) {

        std::shared_ptr<IndexController<Serialize< std::vector<std::pair<Tweet, Profile> > > > > cont =
                make_shared<IndexController<Serialize< std::vector<std::pair<Tweet, Profile>>>>>(worker);

        json_response = cont->get_queryset(profile_id);

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

        return;

    } else if (std::regex_match(request_string, tag_search_regex)) {

        std::shared_ptr<TagSearchController<Serialize< std::vector<std::pair<Tweet, Profile> > > > > cont =
                make_shared<TagSearchController<Serialize< std::vector<std::pair<Tweet, Profile>>>>>(worker);

        json_response = cont->get_queryset(query_string_map["tag"]);

        boost::property_tree::json_parser::write_json(ss, json_response);
        beast::ostream(response.body()) << ss.str();

        return;

    } else {
        std::cout << "Bad gateway" << std::endl;

        response.result(http::status::not_found);
        response.set(http::field::content_type, "text/plain");
        beast::ostream(response.body()) << "File not found";
    }
}

void HTTPClient::write_response() {
    auto self = shared_from_this();

    response.set(http::field::content_length, response.body().size());

    http::async_write(
            socket,
            response,
            [self](beast::error_code ec, std::size_t) {
                self->socket.shutdown(tcp::socket::shutdown_send, ec);
                self->deadline_.cancel();
            });
}

void HTTPClient::check_deadline() {
    auto self = shared_from_this();

    deadline_.async_wait(
            [self](beast::error_code ec) {
                if(!ec) {
                    self->socket.close(ec);
                }
            });
}