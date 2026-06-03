#pragma once

#include "net/AsioUtil.h"
#include "net/HttpParser.h"
#include "net/Buffer.h"
#include <memory>
#include <array>
#include <functional>

namespace lightning {

using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(net::tcp::socket socket, HttpCallback callback);

    void start();

private:
    void doRead();
    void onRead(const net::error_code& ec, std::size_t bytes);
    void handleRequest(const HttpRequest& req);
    void sendResponse(const HttpResponse& resp, bool close);
    void onWrite(const net::error_code& ec, bool close);

    net::tcp::socket socket_;
    HttpCallback callback_;
    Buffer buffer_;
    HttpParser parser_;
    std::array<char, 8192> readBuf_;
};

} // namespace lightning
