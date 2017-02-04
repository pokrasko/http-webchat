#ifndef HTTPWEBCHAT_HTTPCOMMON_H
#define HTTPWEBCHAT_HTTPCOMMON_H


#include <map>

#include "../common.h"

namespace Http {
    enum Method {GET, HEAD, OPTIONS, POST};

    const std::string VERSION1_0 = "HTTP/1.0";
    const std::string VERSION1_1 = "HTTP/1.1";

    std::string methodToString(Method);
    Method stringToMethod(const std::string&);

    std::string uriEncode(const std::string&);
    std::string uriDecode(const std::string&);

    std::string getUriPath(const std::string&);
    std::map<std::string, std::string> queryParameters(const std::string&);
}


#endif //HTTPWEBCHAT_HTTPCOMMON_H
