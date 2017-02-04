#include "http_request.h"

HttpRequest::HttpRequest(): HttpMessage() {}

HttpRequest::HttpRequest(Http::Method method, const std::string& uri, const std::string& version):
        HttpMessage(version), method(method), uri(uri) {}

HttpRequest::~HttpRequest() {}

bool HttpRequest::shouldHaveBody() const {
    return (isParsed && (getHeader("Content-Length") != "" || getHeader("Transfer-Encoding") != ""))
           || (!isParsed && method == Http::Method::POST);
}

Http::Method HttpRequest::getMethod() const {
    return method;
}

std::string HttpRequest::getMethodAsString() const {
    return methodToString(method);
}

std::string HttpRequest::getUri() const {
    return uri;
}

std::string HttpRequest::getUriDecoded() const {
    return Http::uriDecode(uri);
}

void HttpRequest::append(std::string data) {
    if (!isParsed) {
        throw "The message is constructed, not parsed";
    }

    try {
        switch (state) {
            case START: {
                size_t space = data.find(' ');
                std::string method = data.substr(0, space);
                this->method = Http::stringToMethod(method);
                data.erase(0, space + 1);

                space = data.find(' ');
                uri = data.substr(0, space);
                data.erase(0, space + 1);

                if (data.size() != 8
                    || data[0] != 'H' || data[1] != 'T' || data[2] != 'T' || data[3] != 'P' || data[4] != '/'
                    || data[6] != '.' || data[5] < '0' || data[5] > '9' || data[7] < '0' || data[7] > '9') {
                    state = INVALID;
                    return;
                }
                version = data;
                state = HEADER;
                return;
            }
            case HEADER: {
                parseHeader(data);
                return;
            }
            case BODY: {
                appendBody(data);
                if (getBodySize() == getDeclaredBodySize()) {
                    state = FINISHED;
                    return;
                } else {
                    return;
                }
            }
            default: {
                throw OwnException("The message is finished");
            }
        }
    } catch (const std::exception& exception) {
        if (strcmp(exception.what(), "The message is finished")) {
//        if (exception.what() == "The message is finished") {
            throw exception;
        } else {
            state = INVALID;
        }
    }
}

std::string HttpRequest::firstLine() const {
    return methodToString(method) + " " + uri + " " + version + CRLF;
}
