#include "chat_server.h"

ChatServer::Message::Message(const std::string& from, time_t time, const std::string& text):
        JSON(std::map<std::string, JSON>(makeFields(from, time, text))) {}

std::map<std::string, JSON> ChatServer::Message::makeFields(
        const std::string& from, time_t time, const std::string& text) {
    std::map<std::string, JSON> result;
    result["from"] = from;
    result["time"] = time;
    result["text"] = text;
    return result;
}

ChatServer::Object::Object(const std::map<std::string, JSON::Type>& types): types(types), size(types.size()) {}

std::map<std::string, JSON> ChatServer::Object::match(const std::string& data) {
    JSON payload = JSON::parseJSON(data);
    if (payload.getType() != JSON::Type::OBJECT || payload.getObjectValue().size() != size) {
        throw OwnException("Object mismatched: " + data);
    }
    std::map<std::string, JSON> payloadFields = payload.getObjectValue();
    for (std::map<std::string, JSON>::const_iterator it = payloadFields.begin(); it != payloadFields.end(); ++it) {
        if (types.find(it->first) == types.end() || it->second.getType() != types[it->first]) {
            throw OwnException("Object mismatched: " + data);
        }
    }
    return payload.getObjectValue();
}

std::pair<std::string, std::string> ChatServer::parseMessage(const std::string& string) {
    std::map<std::string, JSON::Type> pattern;
    pattern["message"] = JSON::Type::STRING;
    pattern["username"] = JSON::Type::STRING;
    std::map<std::string, JSON> messagePayload = Object(pattern).match(string);
    return std::make_pair(messagePayload["username"].getStringValue(), messagePayload["message"].getStringValue());
}

std::string ChatServer::historyAsJson(size_t begin, size_t end) {
    std::map<std::string, JSON> payload;
    payload["messages"] = JSON(std::vector<JSON>(history.begin() + begin, history.begin() + end));
    return JSON(payload).toString();
}

void ChatServer::logError(const HttpRequest& request, int code, const std::string& response) {
    std::cout << request.getMethodAsString() << " request to \"" << request.getUri() << "\"" << std::endl;
    std::cout << "  Body: \"" << request.getBody() << "\"" << std::endl;
    std::cout << "  Result: " << response << ", sending code " << code << std::endl;
}

ChatServer::ChatServer(uint16_t port, Poller& poller): httpServer(HttpServer(port, poller)) {
    httpServer.addRouteMatcher(RouteMatcher(Http::Method::POST, "/login"),
        [this](const HttpRequest& request, HttpServer::ResponseSocket responseSocket) {
            try {
                try {
                    std::map<std::string, JSON::Type> pattern;
                    pattern["username"] = JSON::Type::STRING;
                    std::map<std::string, JSON> usernamePayload = Object(pattern).match(request.getBody());
                    std::string username = usernamePayload["username"].getStringValue();

                    if (username == ADMIN_NAME) {
                        throw OwnException("One can't login with username Admin");
                    }

                    if (firstMessage.find(username) == firstMessage.end()) {
                        size_t first = history.size();
                        std::cout << "User \"" << username << "\" joined to chat" << std::endl;
                        history.push_back(Message(ADMIN_NAME, time(NULL), "User " + username + " joined to chat!"));
                        firstMessage[username] = first;
                    }
                } catch (const OwnException& exception) {
                    logError(request, 400, "Bad request: " + std::string(exception.what()));
                    HttpResponse response(request.getMethod(), Http::VERSION1_1, 400, "Bad Request");
                    responseSocket.end(response);
                    return;
                }

                HttpResponse response = HttpResponse(request.getMethod(), Http::VERSION1_1, 200, "OK");
                if (!request.shouldKeepAlive()) {
                    response.setHeader("Connection", "Keep-Alive");
                }
                responseSocket.end(response);
            } catch (const std::exception& exception) {
                std::cerr << "Exception while responding to request (method "
                          << Http::methodToString(request.getMethod()) << ", URL \"" << request.getUri()
                          << "\"), closing connection: " << exception.what() << "" << std::endl;
                responseSocket.close();
            }
        });

    httpServer.addRouteMatcher(RouteMatcher(Http::Method::GET, "/messages"),
        [this](const HttpRequest& request, HttpServer::ResponseSocket responseSocket) {
            try {
                size_t begin;

                try {
                    std::map<std::string, std::string> queryParams = Http::queryParameters(request.getUri());
                    if (queryParams.find("username") == queryParams.end() ||
                            queryParams.find("all") == queryParams.end()) {
                        throw OwnException(
                                std::string("There should be \"username\" and \"all\" query parameters")
                                + " in a GET request to \"messages\"");
                    }
                    std::string username = queryParams["username"];
                    std::string allMessages = queryParams["all"];
                    if (username == "" || (allMessages != "true" && allMessages != "false")) {
                        throw OwnException("Bad request: empty username or invalid all messages indicator");
                    } else if (username == ADMIN_NAME) {
                        throw OwnException("One can't get messages from username Admin");
                    }

                    bool isAll = (allMessages == "true");
                    if (isAll || firstUnreadMessage.find(username) == firstUnreadMessage.end()) {
                        begin = firstMessage[username];
                    } else {
                        begin = firstUnreadMessage[username];
                    }
                    firstUnreadMessage[username] = history.size();
                } catch (const OwnException& exception) {
                    logError(request, 400, "Bad request: " + std::string(exception.what()));
                    HttpResponse response(request.getMethod(), Http::VERSION1_1, 400, "Bad Request");
                    responseSocket.end(response);
                    return;
                }

                HttpResponse response(request.getMethod(), Http::VERSION1_1, 200, "OK");
                if (!request.shouldKeepAlive()) {
                    response.setHeader("Connection", "Keep-Alive");
                }
                response.setHeader("Content-Type", "application/json; charset=UTF-8");
                response.appendBody(historyAsJson(begin, history.size()));
                responseSocket.end(response);
            } catch (const std::exception& exception) {
                std::cerr << "Exception while responding to request (method "
                          << Http::methodToString(request.getMethod()) << ", URL \"" << request.getUri()
                          << "\"), closing connection: " << exception.what() << "" << std::endl;
                responseSocket.close();
            }
        });

    httpServer.addRouteMatcher(RouteMatcher(Http::Method::HEAD, "/messages"),
        [this](const HttpRequest& request, HttpServer::ResponseSocket responseSocket) {
            try {
                try {
                    std::map<std::string, std::string> queryParams = Http::queryParameters(request.getUri());
                    if (queryParams.find("username") == queryParams.end() ||
                            queryParams.find("all") == queryParams.end()) {
                        throw OwnException(
                                std::string("There should be \"username\" and \"all\" query parameters")
                                + " in a GET request to \"messages\"");
                    }
                    std::string username = queryParams["username"];
                    std::string allMessages = queryParams["all"];
                    if (username == "" || (allMessages != "true" && allMessages != "false")) {
                        throw OwnException("Bad request: empty username or invalid all messages indicator");
                    } else if (username == ADMIN_NAME) {
                        throw OwnException("One can't get messages from username Admin");
                    }
                } catch (const OwnException& exception) {
                    logError(request, 400, "Bad request: " + std::string(exception.what()));
                    HttpResponse response(request.getMethod(), Http::VERSION1_1, 400, "Bad Request");
                    responseSocket.end(response);
                    return;
                }

                HttpResponse response(request.getMethod(), Http::VERSION1_1, 200, "OK");
                if (!request.shouldKeepAlive()) {
                    response.setHeader("Connection", "Keep-Alive");
                }
                response.setHeader("Content-Type", "application/json; charset=UTF-8");
                responseSocket.end(response);
            } catch (const std::exception& exception) {
                std::cerr << "Exception while responding to request (method "
                          << Http::methodToString(request.getMethod()) << ", URL \"" << request.getUri()
                          << "\"), closing connection: " << exception.what() << "" << std::endl;
                responseSocket.close();
            }
        });

    httpServer.addRouteMatcher(RouteMatcher(Http::Method::POST, "/messages"),
        [this](const HttpRequest& request, HttpServer::ResponseSocket responseSocket) {
            try {
                try {
                    std::pair<std::string, std::string> identifiedMessage = parseMessage(request.getBody());
                    std::string username = identifiedMessage.first;
                    std::string message = identifiedMessage.second;
                    if (username == "" || message == "") {
                        throw OwnException("Bad request: empty username or message");
                    } else if (username == ADMIN_NAME) {
                        throw OwnException("One can't post a message from username Admin");
                    }

                    std::cout << "User \"" << username << "\" sent message: \"" << message << "\"" << std::endl;
                    history.push_back(Message(username, time(NULL), message));
                } catch (const OwnException& exception) {
                    logError(request, 400, "Bad request: " + std::string(exception.what()));
                    HttpResponse response(request.getMethod(), Http::VERSION1_1, 400, "Bad Request");
                    responseSocket.end(response);
                    return;
                }

                HttpResponse response(request.getMethod(), Http::VERSION1_1, 200, "OK");
                if (!request.shouldKeepAlive()) {
                    response.setHeader("Connection", "Keep-Alive");
                }
                responseSocket.end(response);
            } catch (const std::exception& exception) {
                std::cerr << "Exception while responding to request (method "
                          << Http::methodToString(request.getMethod()) << ", URL \"" << request.getUri()
                          << "\"), closing connection: " << exception.what() << "" << std::endl;
                responseSocket.close();
            }
        });

    httpServer.addRouteMatcher(RouteMatcher(Http::Method::GET, "*"),
        [](const HttpRequest& request, HttpServer::ResponseSocket responseSocket) {
            try {
                std::string body, type;

                try {
                    std::string filename = Http::getUriPath(request.getUri());
                    if (filename[0] == '/') {
                        filename.erase(0, 1);
                    }
                    if (filename == "") {
                        filename = "index.html";
                    }

                    try {
                        Resource resource = Resource::getResource(filename);
                        body = std::string(resource.data(), resource.size());
                    } catch (const std::out_of_range& out_of_range) {
                        logError(request, 404, "Not found: " + filename);
                        HttpResponse response(request.getMethod(), Http::VERSION1_1, 404, "Not Found");
                        response.appendBody("<html>"
                                                    "<head>"
                                                    "<title>The resource isn't found</title>"
                                                    "</head>"
                                                    "<body>"
                                                    "<h1>Not found</h1>"
                                                    "<p>The requested URL " + Http::getUriPath(request.getUri())
                                            + " was not found on this server.</p>"
                                                    "<hr>"
                                                    "</body>"
                                                    "</html>");
                        responseSocket.end(response);
                        return;
                    }

                    size_t ext = filename.find('.');
                    type = (ext != std::string::npos)
                                       ? filename.substr(ext + 1, filename.size() - ext - 1)
                                       : "";
                } catch (const OwnException& own_exception) {
                    logError(request, 400, "Bad request: " + std::string(own_exception.what()));
                    HttpResponse response(request.getMethod(), Http::VERSION1_1, 400, "Bad Request");
                    responseSocket.end(response);
                    return;
                }

                HttpResponse response(request.getMethod(), Http::VERSION1_1, 200, "OK");
                if (!request.shouldKeepAlive()) {
                    response.setHeader("Connection", "Keep-Alive");
                }
                if (type == "js") {
                    response.setHeader("Content-Type", "application/javascript");
                } else if (type == "html" || type == "htm") {
                    response.setHeader("Content-Type", "text/html");
                } else if (type == "css") {
                    response.setHeader("Content-Type", "text/css");
                }
                response.appendBody(body);
                responseSocket.end(response);
            } catch (const std::exception& exception) {
                std::cerr << "Exception while responding to request (method "
                          << Http::methodToString(request.getMethod()) << ", URL \"" << request.getUri()
                          << "\"), closing connection: " << exception.what() << "" << std::endl;
                responseSocket.close();
            }
        });

    httpServer.addRouteMatcher(RouteMatcher(Http::Method::HEAD, "*"),
        [](const HttpRequest& request, HttpServer::ResponseSocket responseSocket) {
            try {
                std::string type;

                try {
                    std::string filename = Http::getUriPath(request.getUri());
                    if (filename[0] == '/') {
                        filename.erase(0, 1);
                    }
                    if (filename == "") {
                        filename = "index.html";
                    }

                    try {
                        Resource::getResource(filename);
                    } catch (const std::out_of_range& out_of_range) {
                        logError(request, 404, "Not found: " + filename);
                        HttpResponse response(request.getMethod(), Http::VERSION1_1, 404, "Not Found");
                        responseSocket.end(response);
                        return;
                    }

                    size_t ext = filename.find('.');
                    type = (ext != std::string::npos)
                                       ? filename.substr(ext + 1, filename.size() - ext - 1)
                                       : "";
                } catch (const OwnException& exception) {
                    logError(request, 400, "Bad request: " + std::string(exception.what()));
                    HttpResponse response(request.getMethod(), Http::VERSION1_1, 400, "Bad Request");
                    responseSocket.end(response);
                    return;
                }

                HttpResponse response(request.getMethod(), Http::VERSION1_1, 200, "OK");
                if (!request.shouldKeepAlive()) {
                    response.setHeader("Connection", "Keep-Alive");
                }
                if (type == "js") {
                    response.setHeader("Content-Type", "application/javascript");
                } else if (type == "html" || type == "htm") {
                    response.setHeader("Content-Type", "text/html");
                }
                responseSocket.end(response);
            } catch (const std::exception& exception) {
                std::cerr << "Exception while responding to request (method "
                          << Http::methodToString(request.getMethod()) << ", URL \"" << request.getUri()
                          << "\"), closing connection: " << exception.what() << "" << std::endl;
                responseSocket.close();
            }
        });
}
