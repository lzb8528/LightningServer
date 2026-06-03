#include "net/HttpSession.h"
#include <spdlog/spdlog.h>

namespace lightning {

HttpSession::HttpSession(net::tcp::socket socket, HttpCallback callback)
    : socket_(std::move(socket)), callback_(std::move(callback)) {}

void HttpSession::start() {
    doRead();
}

void HttpSession::doRead() {
    auto self = shared_from_this();
    socket_.async_read_some(
        net::buffer(readBuf_),
        [self](const net::error_code& ec, std::size_t bytes) {
            self->onRead(ec, bytes);
        });
}

void HttpSession::onRead(const net::error_code& ec, std::size_t bytes) {
    if (ec) {
        if (ec != net::error::eof && ec != net::error::operation_aborted) {
            spdlog::debug("Read error: {}", ec.message());
        }
        return;
    }

    buffer_.append(readBuf_.data(), bytes);

    HttpRequest req;
    if (parser_.parse(&buffer_, &req)) {
        handleRequest(req);
    } else {
        doRead();
    }
}

void HttpSession::handleRequest(const HttpRequest& req) {
    HttpResponse resp;
    resp.setContentType("application/json; charset=utf-8");
    resp.addHeader("Access-Control-Allow-Origin", "*");
    resp.addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    resp.addHeader("Access-Control-Allow-Headers", "Content-Type");

    if (req.method == "OPTIONS") {
        resp.setStatusCode(HttpResponse::k200Ok);
        resp.setBody("{}");
        sendResponse(resp, !req.keepAlive());
        return;
    }

    if (callback_) {
        callback_(req, &resp);
    } else {
        resp.setStatusCode(HttpResponse::k404NotFound);
        resp.setBody(R"({"error":"Not Found"})");
    }

    sendResponse(resp, !req.keepAlive());
}

void HttpSession::sendResponse(const HttpResponse& resp, bool close) {
    auto response = std::make_shared<std::string>(resp.toString());
    auto self = shared_from_this();
    net::async_write(
        socket_,
        net::buffer(*response),
        [self, response, close](const net::error_code& ec, std::size_t) {
            self->onWrite(ec, close);
        });
}

void HttpSession::onWrite(const net::error_code& ec, bool close) {
    if (ec) {
        spdlog::debug("Write error: {}", ec.message());
        return;
    }

    if (close) {
        net::error_code ignored;
        socket_.shutdown(net::tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
    } else {
        buffer_.retrieveAll();
        doRead();
    }
}

} // namespace lightning
