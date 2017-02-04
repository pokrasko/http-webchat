#ifndef HTTPWEBCHAT_JSON_H
#define HTTPWEBCHAT_JSON_H


#include <map>
#include <vector>

#include "../common.h"

class JSON {
public:
    enum Type {NULL_VALUE, BOOLEAN, INTEGER, DOUBLE, STRING, ARRAY, OBJECT};
private:
    Type type;

    union
    {
        bool booleanValue;
        long integerValue;
        double doubleValue;
        std::string stringValue;
        std::vector<JSON> arrayValue;
        std::map<std::string, JSON> objectValue;
    };

    static JSON readNumber(const std::string&, size_t&, bool);
    static JSON _parseJSON(const std::string&, size_t&);
public:
    JSON();
    JSON(bool);
    JSON(long);
    JSON(double);
    JSON(const std::string&);
    JSON(const std::vector<JSON>&);
    JSON(const std::map<std::string, JSON>&);
    ~JSON();

    JSON(JSON const&);
    JSON& operator=(const JSON& rhs);

    Type getType() const;
    bool getBooleanValue() const;
    long getIntegerValue() const;
    double getDoubleValue() const;
    std::string getStringValue() const;
    std::vector<JSON> getArrayValue() const;
    std::map<std::string, JSON> getObjectValue() const;

    void clear();

    static JSON parseJSON(const std::string&);
    std::string toString() const;
};


#endif //HTTPWEBCHAT_JSON_H
