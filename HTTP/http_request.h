#ifndef HTTPWEBCHAT_HTTPREQUEST_H
#define HTTPWEBCHAT_HTTPREQUEST_H


#include "http_message.h"

class HttpRequest: public HttpMessage {
    Http::Method method;
    std::string uri;

    virtual bool shouldHaveBody() const;
public:
    HttpRequest();
    HttpRequest(Http::Method, const std::string&, const std::string&);
    virtual ~HttpRequest();

    Http::Method getMethod() const;
    std::string getMethodAsString() const;
    std::string getUri() const;
    std::string getUriDecoded() const;

    void append(std::string);
    virtual std::string firstLine() const;
};


#endif //HTTPWEBCHAT_HTTPREQUEST_H
