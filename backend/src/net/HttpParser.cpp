#include "net/HttpParser.h"
#include "net/Buffer.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace lightning {

static bool startsWith(const std::string& s, const char* prefix) {
    return s.compare(0, strlen(prefix), prefix) == 0;
}

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool HttpParser::parseRequestLine(const char* begin, const char* end, HttpRequest* req) {
    std::string line(begin, end);
    std::istringstream iss(line);
    iss >> req->method >> req->path >> req->version;
    if (req->method.empty() || req->path.empty() || req->version.empty()) {
        return false;
    }
    parseQueryString(req->path, req);
    return true;
}

void HttpParser::parseQueryString(const std::string& path, HttpRequest* req) {
    auto qpos = path.find('?');
    if (qpos == std::string::npos) return;

    std::string query = path.substr(qpos + 1);
    req->path = path.substr(0, qpos);

    std::istringstream iss(query);
    std::string pair;
    while (std::getline(iss, pair, '&')) {
        auto eq = pair.find('=');
        if (eq != std::string::npos) {
            req->queryParams[pair.substr(0, eq)] = pair.substr(eq + 1);
        }
    }
}

bool HttpParser::parseHeaders(const char* begin, const char* end, HttpRequest* req) {
    std::string line(begin, end);
    auto colon = line.find(':');
    if (colon == std::string::npos) return false;
    std::string key = trim(line.substr(0, colon));
    std::string value = trim(line.substr(colon + 1));
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    req->headers[key] = value;
    return true;
}

bool HttpParser::parse(Buffer* buf, HttpRequest* req) {
    const char* crlf = "\r\n";
    const char* dblcrlf = "\r\n\r\n";

    const char* data = buf->peek();
    const char* end = data + buf->readableBytes();

    const char* headerEnd = std::search(data, end, dblcrlf, dblcrlf + 4);
    if (headerEnd == end) return false;

    std::string headerBlock(data, headerEnd);
    buf->retrieve(headerEnd - data + 4);

    std::istringstream iss(headerBlock);
    std::string line;
    bool firstLine = true;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        if (firstLine) {
            if (!parseRequestLine(line.data(), line.data() + line.size(), req)) {
                return false;
            }
            firstLine = false;
        } else {
            parseHeaders(line.data(), line.data() + line.size(), req);
        }
    }

    auto clIt = req->headers.find("content-length");
    if (clIt != req->headers.end()) {
        size_t contentLength = std::stoul(clIt->second);
        if (buf->readableBytes() < contentLength) return false;
        req->body = buf->retrieveAsString(contentLength);
    }

    return true;
}

void HttpResponse::setStatusCode(StatusCode code) {
    statusCode = code;
    switch (code) {
        case k200Ok: statusMessage = "OK"; break;
        case k201Created: statusMessage = "Created"; break;
        case k400BadRequest: statusMessage = "Bad Request"; break;
        case k404NotFound: statusMessage = "Not Found"; break;
        case k405MethodNotAllowed: statusMessage = "Method Not Allowed"; break;
        case k500InternalError: statusMessage = "Internal Server Error"; break;
        case k503ServiceUnavailable: statusMessage = "Service Unavailable"; break;
    }
}

std::string HttpResponse::toString() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << static_cast<int>(statusCode) << " " << statusMessage << "\r\n";
    for (const auto& [k, v] : headers) {
        oss << k << ": " << v << "\r\n";
    }
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "\r\n" << body;
    return oss.str();
}

} // namespace lightning
