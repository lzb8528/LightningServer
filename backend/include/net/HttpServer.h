#pragma once

#include "net/AsioUtil.h"
#include "net/HttpParser.h"
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>

namespace lightning {

using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;

class HttpServer {
public:
    HttpServer(const std::string& host, uint16_t port, int threadNum = 4);
    ~HttpServer();

    void setHttpCallback(HttpCallback cb) { httpCallback_ = std::move(cb); }

    net::io_context& ioContext() { return ioc_; }

    void start();
    void run();
    void stop();

private:
    void doAccept();
    void startAccept();

    net::io_context ioc_;
    net::tcp::acceptor acceptor_;
    std::vector<std::thread> threads_;
    int threadNum_;
    HttpCallback httpCallback_;
    std::atomic<bool> stopped_{false};
};

} // namespace lightning
