#pragma once

#include <string>
#include <map>
#include <unordered_map>

namespace lightning {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    std::unordered_map<std::string, std::string> queryParams;

    bool keepAlive() const {
        auto it = headers.find("Connection");
        if (it != headers.end()) {
            return it->second == "keep-alive";
        }
        return version == "HTTP/1.1";
    }
};

struct HttpResponse {
    enum StatusCode {
        k200Ok = 200,
        k201Created = 201,
        k400BadRequest = 400,
        k404NotFound = 404,
        k405MethodNotAllowed = 405,
        k500InternalError = 500,
        k503ServiceUnavailable = 503
    };

    StatusCode statusCode = k200Ok;
    std::string statusMessage = "OK";
    std::map<std::string, std::string> headers;
    std::string body;

    void setStatusCode(StatusCode code);
    void setContentType(const std::string& type) { headers["Content-Type"] = type; }
    void setBody(const std::string& b) { body = b; }
    void addHeader(const std::string& key, const std::string& value) { headers[key] = value; }

    std::string toString() const;
};

class HttpParser {
public:
    enum ParseState { kExpectRequestLine, kExpectHeaders, kExpectBody, kGotAll };

    bool parse(Buffer* buf, HttpRequest* req);

private:
    bool parseRequestLine(const char* begin, const char* end, HttpRequest* req);
    bool parseHeaders(const char* begin, const char* end, HttpRequest* req);
    void parseQueryString(const std::string& path, HttpRequest* req);
};

} // namespace lightning
