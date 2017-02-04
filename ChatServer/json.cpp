#include "json.h"

JSON::JSON(): type(NULL_VALUE) {}

JSON::JSON(bool booleanValue)
    : type(BOOLEAN)
{
    new (&this->booleanValue) bool(booleanValue);
}

JSON::JSON(long integerValue)
    : type(INTEGER)
{
    new (&this->integerValue) long(integerValue);
}

JSON::JSON(double doubleValue)
    : type(DOUBLE)
{
    new (&this->doubleValue) double(doubleValue);
}

JSON::JSON(const std::string& stringValue)
    : type(STRING)
{
    new (&this->stringValue) std::string(stringValue);
}

JSON::JSON(const std::vector<JSON>& arrayElements)
    : type(ARRAY)
{
    new (&this->arrayValue) std::vector<JSON>(arrayElements);
}

JSON::JSON(const std::map<std::string, JSON>& objectFields)
    : type(OBJECT)
{
    new (&this->objectValue) std::map<std::string, JSON>(objectFields);
}

JSON::~JSON()
{
    clear();
}

JSON::JSON(const JSON& rhs)
    : JSON()
{
    *this = rhs;
}

JSON& JSON::operator=(const JSON& rhs)
{
    if (this == &rhs)
        return *this;

    clear();
    switch (rhs.type)
    {
    case JSON::NULL_VALUE:
        break;
    case JSON::BOOLEAN:
        new (&this->booleanValue) bool(rhs.booleanValue);
        break;
    case JSON::INTEGER:
        new (&this->integerValue) long(rhs.integerValue);
        break;
    case JSON::DOUBLE:
        new (&this->doubleValue) double(rhs.doubleValue);
        break;
    case JSON::STRING:
        new (&this->stringValue) std::string(rhs.stringValue);
        break;
    case JSON::ARRAY:
        new (&this->arrayValue) std::vector<JSON>(rhs.arrayValue);
        break;
    case JSON::OBJECT:
        new (&this->objectValue) std::map<std::string, JSON>(rhs.objectValue);
        break;
    }
    type = rhs.type;

    return *this;
}

JSON JSON::readNumber(const std::string& data, size_t& cur, bool isNegative) {
    std::string numberAsString = (isNegative ? "-" : "");
    if (data[cur] == '0') {
        if (data[cur + 1] >= '0' && data[cur + 1] <= '9') {
            throw OwnException("Wrong JSON (number starting with 0) at symbol " + std::to_string(cur) + ": \""
                                     + data + "\"");
        } else {
            numberAsString += data[cur++];
        }
    } else if (data[cur] >= '1' && data[cur] <= '9') {
        for (; cur < data.size() && data[cur] >= '0' && data[cur] <= '9'; ++cur) {
            numberAsString += data[cur];
        }
    } else {
        throw OwnException("Wrong JSON (pure minus) at symbol " + std::to_string(cur) + ": \"" + data + "\"");
    }

    if (data[cur] != '.') {
        return JSON(std::stol(numberAsString));
    } else {
        numberAsString += data[cur++];
        if (cur == data.size()) {
            throw OwnException("Wrong JSON (unfinished): " + data);
        }
        for (; cur < data.size() && data[cur] >= '0' && data[cur] <= '9'; ++cur) {
            numberAsString += data[cur];
        }
        return JSON(std::stod(numberAsString));
    }
}

JSON JSON::_parseJSON(const std::string& data, size_t& cur) {
    for (; cur < data.size()
           && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
    if (cur == data.size()) {
        throw OwnException("Wrong JSON (unfinished): " + data);
    }
    if (data[cur] == 'n') {
        if (data[cur + 1] == 'u' && data[cur + 2] == 'l' && data[cur + 3] == 'l') {
            cur += 4;
            return JSON();
        } else {
            throw OwnException("Wrong JSON (couln't parse null) at symbol " + std::to_string(cur) + ": \""
                                     + data + "\"");
        }
    } else if (data[cur] == 't') {
        if (data[cur + 1] == 'r' && data[cur + 2] == 'u' && data[cur + 3] == 'e') {
            cur += 4;
            return JSON(true);
        } else {
            throw OwnException("Wrong JSON (couldn't parse true) at symbol " + std::to_string(cur) + ": \""
                                     + data + "\"");
        }
    } else if (data[cur] == 'f') {
        if (data[cur + 1] == 'a' && data[cur + 2] == 'l' && data[cur + 3] == 's' && data[cur + 4] == 'e') {
            cur += 5;
            return JSON(false);
        } else {
            throw OwnException("Wrong JSON (couldn't parse false) at symbol " + std::to_string(cur) + ": \""
                                     + data + "\"");
        }
    } else if (data[cur] == '-') {
        if (++cur == data.size()) {
            throw OwnException("Wrong JSON (unfinished): " + data);
        } else {
            return readNumber(data, cur, true);
        }
    } else if (data[cur] >= '0' && data[cur] <= '9') {
        return readNumber(data, cur, false);
    } else if (data[cur] == '\"') {
        ++cur;

        std::string string;
        size_t fnes;
        for (fnes = cur; cur < data.size() && data[cur] != '\"'; ++cur) {
            if (data[cur] == '\\') {
                if (fnes < cur) {
                    string += data.substr(fnes, cur - fnes);
                }
                if (data[cur + 1] == '"') {
                    string += '"';
                } else if (data[cur + 1] == 'n') {
                    string += '\n';
                } else if (data[cur + 1] == '\\') {
                    string += '\\';
                } else if (data[cur + 1] == 't') {
                    string += '\t';
                } else if (data[cur + 1] == 'r') {
                    string += '\r';
                } else if (data[cur + 1] == 'b') {
                    string += '\b';
                } else if (data[cur + 1] == 'f') {
                    string += '\f';
                } else if (data[cur + 1] == '/') {
                    string += '/';
                } else {
                    throw OwnException("Wrong JSON (invalid escape sequence) at symbol "
                                             + std::to_string(cur + 1) + ": \"" + data + "\"");
                }
                fnes = cur + 2;
                ++cur;
            }
        }
        if (fnes < cur) {
            string += data.substr(fnes, cur - fnes);
        }

        if (cur == data.size()) {
            throw OwnException("Wrong JSON (unfinished): " + data);
        } else {
            ++cur;
            return JSON(string);
        }
    } else if (data[cur] == '[') {
        ++cur;

        std::vector<JSON> arrayElements;
        while (cur < data.size() && data[cur] != ']') {
            for (; cur < data.size()
                   && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
            if (cur == data.size()) {
                throw OwnException("Wrong JSON (unfinished): " + data);
            }

            arrayElements.push_back(_parseJSON(data, cur));

            for (; cur < data.size()
                   && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
            if (cur == data.size()) {
                throw OwnException("Wrong JSON (unfinished): " + data);
            }

            if (data[cur] == ']') {
                break;
            } else if (data[cur++] != ',') {
                throw OwnException("Wrong JSON (should be comma in array) at symbol " + std::to_string(cur)
                                         + ": \"" + data + "\"");
            }
        }

        if (cur == data.size()) {
            throw OwnException("Wrong JSON (unfinished): " + data);
        } else {
            ++cur;
            return JSON(arrayElements);
        }
    } else if (data[cur] == '{') {
        ++cur;

        std::map<std::string, JSON> objectValues;
        while (cur < data.size() && data[cur] != ']') {
            for (; cur < data.size()
                   && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
            if (cur == data.size()) {
                throw OwnException("Wrong JSON (unfinished): " + data);
            }

            JSON key = _parseJSON(data, cur);
            if (key.getType() != STRING) {
                throw OwnException("Wrong JSON (key type isn't a string) at symbol " + std::to_string(cur)
                                         + ": \"" + data + "\"");
            }

            for (; cur < data.size()
                   && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
            if (cur == data.size()) {
                throw OwnException("Wrong JSON (unfinished): " + data);
            } else if (data[cur++] != ':') {
                throw OwnException("Wrong JSON (should be colon between key and value in object) at symbol "
                                         + std::to_string(cur) + ": \"" + data + "\"");
            }
            for (; cur < data.size()
                   && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
            if (cur == data.size()) {
                throw OwnException("Wrong JSON (unfinished): " + data);
            }

            JSON value = _parseJSON(data, cur);
            objectValues[key.stringValue] = value;

            for (; cur < data.size()
                   && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
            if (cur == data.size()) {
                throw OwnException("Wrong JSON (unfinished): " + data);
            }

            if (cur == data.size()) {
                throw OwnException("Wrong JSON (unfinished): " + data);
            } else if (data[cur] == '}') {
                break;
            } else if (data[cur++] != ',') {
                throw OwnException("Wrong JSON (should be comma in object) at symbol " + std::to_string(cur)
                                         + ": \"" + data + "\"");
            }
        }

        if (cur == data.size()) {
            throw OwnException("Wrong JSON (unfinished): " + data);
        } else {
            ++cur;
            return JSON(objectValues);
        }
    } else {
        throw OwnException("Wrong JSON (new entity isn't parsable) at symbol " + std::to_string(cur) + ": \""
                                 + data + "\"");
    }
}

JSON::Type JSON::getType() const {
    return type;
}

bool JSON::getBooleanValue() const {
    if (type != BOOLEAN) {
        throw OwnException("Taking boolean value from JSON type " + type);
    }
    return booleanValue;
}

long JSON::getIntegerValue() const {
    if (type != INTEGER) {
        throw OwnException("Taking integer value from JSON type " + type);
    }
    return integerValue;
}

double JSON::getDoubleValue() const {
    if (type != DOUBLE) {
        throw OwnException("Taking double value from JSON type " + type);
    }
    return doubleValue;
}

std::string JSON::getStringValue() const {
    if (type != STRING) {
        throw OwnException("Taking string value from JSON type " + type);
    }
    return stringValue;
}

std::vector<JSON> JSON::getArrayValue() const {
    if (type != ARRAY) {
        throw OwnException("Taking array value from JSON type " + type);
    }
    return arrayValue;
}

std::map<std::string, JSON> JSON::getObjectValue() const {
    if (type != OBJECT) {
        throw OwnException("Taking boolean value from JSON type " + type);
    }
    return objectValue;
}

void JSON::clear()
{
    switch (type)
    {
    case JSON::NULL_VALUE:
    case JSON::BOOLEAN:
    case JSON::INTEGER:
    case JSON::DOUBLE:
        break;
    case JSON::STRING:
        this->stringValue.~basic_string();
        break;
    case JSON::ARRAY:
        this->arrayValue.~vector();
        break;
    case JSON::OBJECT:
        this->objectValue.~map();
        break;
    }

    type = JSON::NULL_VALUE;
}

JSON JSON::parseJSON(const std::string& data) {
    size_t cur = 0;
    JSON object = _parseJSON(data, cur);
    for (; cur < data.size()
           && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
    if (cur != data.size()) {
        throw OwnException("Wrong JSON (unsuspected end of JSON) at symbol " + std::to_string(cur) + ": \""
                                 + data + "\"");
    } else {
        return object;
    }
}

std::string JSON::toString() const {
    switch (type) {
        case NULL_VALUE: {
            return "null";
        } case BOOLEAN: {
            return booleanValue ? "true" : "false";
        } case INTEGER: {
            return std::to_string(integerValue);
        } case DOUBLE: {
            return std::to_string(doubleValue);
        } case STRING: {
            std::string encodedValue;
            for (size_t i = 0; i < stringValue.size(); ++i) {
                if (stringValue[i] == '"') {
                    encodedValue += "\\\"";
                } else if (stringValue[i] == '\n') {
                    encodedValue += "\\n";
                } else if (stringValue[i] == '\\') {
                    encodedValue += "\\\\";
                } else if (stringValue[i] == '\t') {
                    encodedValue += "\\t";
                } else if (stringValue[i] == '\r') {
                    encodedValue += "\\r";
                } else if (stringValue[i] == '\b') {
                    encodedValue += "\\b";
                } else if (stringValue[i] == '\f') {
                    encodedValue += "\\f";
                } else {
                    encodedValue += stringValue[i];
                }
            }

            return "\"" + encodedValue + "\"";
        } case ARRAY: {
            std::string result = "[";
            for (std::vector<JSON>::const_iterator it = arrayValue.begin();
                    it != arrayValue.end(); ++it) {
                if (it != arrayValue.begin()) {
                    result += ", ";
                }
                result += it->toString();
            }
            result += "]";
            return result;
        } case OBJECT: {
            std::string result = "{";
            for (std::map<std::string, JSON>::const_iterator it = objectValue.begin();
                 it != objectValue.end(); ++it) {
                if (it != objectValue.begin()) {
                    result += ", ";
                }
                result += "\"" + it->first + "\": " + it->second.toString();
            }
            result += "}";
            return result;
        } default: {
            throw OwnException("Impossible JSON parse exception");
        }
    }
}
