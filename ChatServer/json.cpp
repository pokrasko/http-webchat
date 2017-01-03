#include "json.h"

JSON::JSON(): type(NULL_VALUE) {}

JSON::JSON(bool booleanValue): type(BOOLEAN), booleanValue(booleanValue) {}

JSON::JSON(long integerValue): type(INTEGER), integerValue(integerValue) {}

JSON::JSON(double doubleValue): type(DOUBLE), doubleValue(doubleValue) {}

JSON::JSON(const std::string& stringValue): type(STRING), stringValue(stringValue) {}

JSON::JSON(const std::vector<JSON>& arrayElements): type(ARRAY), arrayElements(arrayElements) {}

JSON::JSON(const std::map<std::string, JSON>& objectFields): type(OBJECT), objectFields(objectFields) {}

JSON JSON::_parseJSON(const std::string& data, size_t& cur) {
    for (; cur < data.size()
           && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
    if (cur == data.size()) {
        throw std::runtime_error("Wrong JSON (unfinished): " + data);
    }
    if (data[cur] == 'n') {
        if (data[cur + 1] == 'u' && data[cur + 2] == 'l' && data[cur + 3] == 'l') {
            cur += 4;
            return JSON();
        } else {
            throw std::runtime_error("Wrong JSON at symbol " + std::to_string(cur) + ": \"" + data + "\"");
        }
    } else if (data[cur] == 't') {
        if (data[cur + 1] == 'r' && data[cur + 2] == 'u' && data[cur + 3] == 'e') {
            cur += 4;
            return JSON(true);
        } else {
            throw std::runtime_error("Wrong JSON at symbol " + std::to_string(cur) + ": \"" + data + "\"");
        }
    } else if (data[cur] == 'f') {
        if (data[cur + 1] == 'a' && data[cur + 2] == 'l' && data[cur + 3] == 's' && data[cur + 4] == 'e') {
            cur += 5;
            return JSON(false);
        } else {
            throw std::runtime_error("Wrong JSON at symbol " + std::to_string(cur) + ": \"" + data + "\"");
        }
    } else if (data[cur] == '-') {
        std::string numberAsString = "" + data[cur++];
        for (; cur < data.size() && data[cur] >= '0' && data[cur] <= '9'; ++cur) {
            numberAsString += data[cur];
        }
        if (data[cur] != '.') {
            return JSON(std::stol(numberAsString));
        } else {
            numberAsString += data[cur++];
            for (; cur < data.size() && data[cur] >= '0' && data[cur] <= '9'; ++cur) {
                numberAsString += data[cur];
            }
            return JSON(std::stod(numberAsString));
        }
    } else if (data[cur] >= '0' && data[cur] <= '9') {
        std::string numberAsString = "" + data[cur];
        for (; cur < data.size() && data[cur] >= '0' && data[cur] <= '9'; ++cur) {
            numberAsString += data[cur];
        }
        if (data[cur] != '.') {
            return JSON(std::stol(numberAsString));
        } else {
            numberAsString += data[cur++];
            for (; cur < data.size() && data[cur] >= '0' && data[cur] <= '9'; ++cur) {
                numberAsString += data[cur];
            }
            return JSON(std::stod(numberAsString));
        }
    } else if (data[cur] == '[') {
        ++cur;

        std::vector<JSON> arrayElements;
        while (cur < data.size() && data[cur] != ']') {
            for (; cur < data.size()
                   && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
            if (cur == data.size()) {
                throw std::runtime_error("Wrong JSON (unfinished): " + data);
            }

            arrayElements.push_back(_parseJSON(data, cur));

            for (; cur < data.size()
                   && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
            if (cur == data.size()) {
                throw std::runtime_error("Wrong JSON (unfinished): " + data);
            }

            if (data[cur] == ']') {
                break;
            } else if (data[cur++] != ',') {
                throw std::runtime_error("Wrong JSON at symbol " + std::to_string(cur) + ": \"" + data + "\"");
            }
        }

        if (cur == data.size()) {
            throw std::runtime_error("Wrong JSON (unfinished): " + data);
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
                throw std::runtime_error("Wrong JSON (unfinished): " + data);
            }

            JSON key = _parseJSON(data, cur);
            if (key.type != STRING) {
                throw std::runtime_error("Wrong JSON at symbol " + std::to_string(cur) + ": \"" + data + "\"");
            }

            for (; cur < data.size()
                   && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
            if (cur == data.size()) {
                throw std::runtime_error("Wrong JSON (unfinished): " + data);
            }

            JSON value = _parseJSON(data, cur);
            objectValues[key.stringValue] = value;

            for (; cur < data.size()
                   && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
            if (cur == data.size()) {
                throw std::runtime_error("Wrong JSON (unfinished): " + data);
            }

            if (cur == data.size()) {
                throw std::runtime_error("Wrong JSON (unfinished): " + data);
            } else if (data[cur] == '}') {
                break;
            } else if (data[cur++] != ',') {
                throw std::runtime_error("Wrong JSON at symbol " + std::to_string(cur) + ": \"" + data + "\"");
            }
        }

        if (cur == data.size()) {
            throw std::runtime_error("Wrong JSON (unfinished): " + data);
        } else {
            ++cur;
            return JSON(objectValues);
        }
    } else {
        throw std::runtime_error("Wrong JSON at symbol " + std::to_string(cur) + ": \"" + data + "\"");
    }
}

JSON::Type JSON::getType() {
    return type;
}

JSON JSON::parseJSON(const std::string& data) {
    size_t cur = 0;
    JSON object = _parseJSON(data, &cur);
    for (; cur < data.size()
           && (data[cur] == ' ' || data[cur] == '\t' || data[cur] == '\r' || data[cur] == '\n'); ++cur);
    if (cur != data.size()) {
        throw std::runtime_error("Wrong JSON at symbol " + std::to_string(cur) + ": \"" + data + "\"");
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
            return "\"" + stringValue + "\"";
        } case ARRAY: {
            std::string result = "[";
            for (std::vector<JSON>::const_iterator it = arrayElements.begin();
                    it != arrayElements.end(); ++it) {
                if (it != arrayElements.begin()) {
                    result += ", ";
                }
                result += it->toString();
            }
            result += "]";
            return result;
        } case OBJECT: {
            std::string result = "{";
            for (std::map<std::string, JSON>::const_iterator it = objectFields.begin();
                 it != objectFields.end(); ++it) {
                if (it != objectFields.begin()) {
                    result += ", ";
                }
                result += "\"" + it->first + "\": " + it->second.toString();
            }
            result += "}";
            return result;
        } default: {
            throw std::runtime_error("Impossible JSON parse exception");
        }
    }
}
