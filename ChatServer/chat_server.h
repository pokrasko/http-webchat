#ifndef HTTPWEBCHAT_CHATSERVER_H
#define HTTPWEBCHAT_CHATSERVER_H


#include <fstream>

#include "../resource.h"
#include "../HTTP/http_server.h"
#include "json.h"

class ChatServer {
public:
    static constexpr const char* ADMIN_NAME = "Admin";

    class Message: public JSON {
        static std::map<std::string, JSON> makeFields(const std::string&, time_t, const std::string&);
    public:
        Message(const std::string&, time_t, const std::string&);
    };

    class Object {
        std::map<std::string, JSON::Type> types;
        size_t size;
    public:
        Object(const std::map<std::string, JSON::Type>&);

        std::map<std::string, JSON> match(const std::string&);
    };
private:
    HttpServer httpServer;
    std::vector<Message> history;
    std::map<std::string, size_t> firstMessage, firstUnreadMessage;

    static std::pair<std::string, std::string> parseMessage(const std::string&);
    std::string historyAsJson(size_t, size_t);
    static void logError(const HttpRequest&, int, const std::string&);
public:
    ChatServer(uint16_t, Poller&);
};


#endif //HTTPWEBCHAT_CHATSERVER_H
