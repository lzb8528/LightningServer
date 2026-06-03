#include "net/HttpServer.h"
#include "net/HttpSession.h"
#include <spdlog/spdlog.h>

namespace lightning {

HttpServer::HttpServer(const std::string& host, uint16_t port, int threadNum)
    : acceptor_(ioc_),
      threadNum_(threadNum) {
    net::error_code ec;
    net::tcp::endpoint endpoint(
        net::ip::make_address(host, ec), port);
    if (ec) {
        endpoint = net::tcp::endpoint(net::tcp::v4(), port);
    }

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) throw std::runtime_error("Failed to open acceptor: " + ec.message());

    acceptor_.set_option(net::tcp::acceptor::reuse_address(true), ec);
    acceptor_.bind(endpoint, ec);
    if (ec) throw std::runtime_error("Failed to bind: " + ec.message());

    acceptor_.listen(net::tcp::socket::max_listen_connections, ec);
    if (ec) throw std::runtime_error("Failed to listen: " + ec.message());
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    startAccept();
    threads_.reserve(threadNum_);
    for (int i = 0; i < threadNum_; ++i) {
        threads_.emplace_back([this] {
            try {
                ioc_.run();
            } catch (const std::exception& e) {
                spdlog::error("IO thread exception: {}", e.what());
            }
        });
    }
    spdlog::info("HttpServer started with {} IO threads", threadNum_);
}

void HttpServer::run() {
    start();
    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }
}

void HttpServer::stop() {
    if (stopped_.exchange(true)) return;

    net::error_code ec;
    acceptor_.close(ec);
    ioc_.stop();

    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }
    threads_.clear();
}

void HttpServer::startAccept() {
    doAccept();
}

void HttpServer::doAccept() {
    acceptor_.async_accept(
        [this](const net::error_code& ec, net::tcp::socket socket) {
            if (!ec) {
                std::make_shared<HttpSession>(std::move(socket), httpCallback_)->start();
            } else if (ec != net::error::operation_aborted) {
                spdlog::warn("Accept error: {}", ec.message());
            }

            if (!stopped_) {
                doAccept();
            }
        });
}

} // namespace lightning
