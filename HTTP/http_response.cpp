#include "http_response.h"

HttpResponse::HttpResponse(): HttpMessage() {}

HttpResponse::HttpResponse(Http::Method requestedMethod,
                           const std::string& version, int statusCode, const std::string& reasonPhrase):
        HttpMessage(version), requestedMethod(requestedMethod), statusCode(statusCode), reasonPhrase(reasonPhrase) {
}

bool HttpResponse::shouldHaveBody() const {
    return (isParsed && (getHeader("Content-Length") != "" || getHeader("Transfer-Encoding") != ""))
           || (!isParsed && requestedMethod != Http::Method::HEAD);
}

Http::Method HttpResponse::getRequestedMethod() const {
    if (isParsed) {
        throw OwnException("Requested method isn't known");
    }
    return requestedMethod;
}

int HttpResponse::getStatusCode() const {
    return statusCode;
}

std::string HttpResponse::getReasonPhrase() const {
    return reasonPhrase;
}

void HttpResponse::setRequestedMethod(Http::Method method) {
    if (!isParsed) {
        throw OwnException("Requested method was set during construction");
    }
    requestedMethod = method;
}

void HttpResponse::append(std::string data) {
    if (!isParsed) {
        throw OwnException("The message is constructed, not parsed");
    }

    try {
        switch (state) {
            case START: {
                size_t space = data.find(' ');
                std::string version = data.substr(0, space);
                if (version.size() != 8
                    || version[0] != 'H' || version[1] != 'T' || version[2] != 'T' || version[3] != 'P'
                    || version[4] != '/' || version[6] != '.' || version[5] < '0' || version[5] > '9'
                    || version[7] < '0' || version[7] > '9') {
                    state = INVALID;
                    return;
                }
                this->version = version;
                data.erase(0, space + 1);

                space = data.find(' ');
                statusCode = std::stoi(data.substr(0, space));
                data.erase(0, space + 1);

                reasonPhrase = data;
                state = HEADER;
                return;
            } case HEADER: {
                parseHeader(data);
                return;
            } case BODY: {
                appendBody(data);
                if (getBodySize() == getDeclaredBodySize()) {
                    state = FINISHED;
                    return;
                } else {
                    return;
                }
            } default: {
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

std::string HttpResponse::firstLine() const {
    return version + " " + std::to_string(statusCode) + " " + reasonPhrase + CRLF;
}
