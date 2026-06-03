#include "handler/ApiHandler.h"
#include <nlohmann/json.hpp>
#include <regex>
#include <spdlog/spdlog.h>

namespace lightning {

ApiHandler::ApiHandler(std::shared_ptr<ArticleService> articleService,
                       std::shared_ptr<CommentService> commentService)
    : articleService_(std::move(articleService)),
      commentService_(std::move(commentService)) {}

int ApiHandler::parseIntParam(const std::string& s, int defaultVal) const {
    try {
        return std::stoi(s);
    } catch (...) {
        return defaultVal;
    }
}

std::string ApiHandler::jsonOk(const nlohmann::json& data) const {
    return data.dump();
}

std::string ApiHandler::jsonError(const std::string& msg, int code) const {
    return nlohmann::json{{"code", code}, {"message", msg}}.dump();
}

void ApiHandler::handleRequest(const HttpRequest& req, HttpResponse* resp) {
    const std::string& path = req.path;

    if (path == "/api/health" && req.method == "GET") {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setBody(R"({"code":0,"message":"ok"})");
        return;
    }

    if (path == "/api/articles" && req.method == "GET") {
        handleGetArticles(req, resp);
        return;
    }

    if (path == "/api/articles/hot" && req.method == "GET") {
        handleGetHotArticles(req, resp);
        return;
    }

    std::regex articleDetailRe(R"(/api/articles/(\d+)$)");
    std::smatch match;
    if (std::regex_match(path, match, articleDetailRe) && req.method == "GET") {
        handleGetArticleDetail(req, resp, std::stoi(match[1]));
        return;
    }

    std::regex commentsRe(R"(/api/articles/(\d+)/comments$)");
    if (std::regex_match(path, match, commentsRe)) {
        int articleId = std::stoi(match[1]);
        if (req.method == "GET") {
            handleGetComments(req, resp, articleId);
        } else if (req.method == "POST") {
            handleCreateComment(req, resp, articleId);
        } else {
            resp->setStatusCode(HttpResponse::k405MethodNotAllowed);
            resp->setBody(jsonError("Method not allowed", 405));
        }
        return;
    }

    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setBody(jsonError("Not Found", 404));
}

void ApiHandler::handleGetArticles(const HttpRequest& req, HttpResponse* resp) {
    int page = 1, pageSize = 10;
    auto pageIt = req.queryParams.find("page");
    auto sizeIt = req.queryParams.find("pageSize");
    if (pageIt != req.queryParams.end()) page = parseIntParam(pageIt->second, 1);
    if (sizeIt != req.queryParams.end()) pageSize = parseIntParam(sizeIt->second, 10);

    auto result = articleService_->listArticles(page, pageSize);
    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setBody(jsonOk(result));
}

void ApiHandler::handleGetArticleDetail(const HttpRequest& req, HttpResponse* resp, int id) {
    (void)req;
    auto result = articleService_->getArticleDetail(id);
    if (result.value("code", 0) == 404) {
        resp->setStatusCode(HttpResponse::k404NotFound);
    } else {
        resp->setStatusCode(HttpResponse::k200Ok);
    }
    resp->setBody(jsonOk(result));
}

void ApiHandler::handleGetHotArticles(const HttpRequest& req, HttpResponse* resp) {
    int limit = 10;
    auto it = req.queryParams.find("limit");
    if (it != req.queryParams.end()) limit = parseIntParam(it->second, 10);

    auto result = articleService_->getHotArticles(limit);
    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setBody(jsonOk(result));
}

void ApiHandler::handleGetComments(const HttpRequest& req, HttpResponse* resp, int articleId) {
    int limit = 20;
    auto it = req.queryParams.find("limit");
    if (it != req.queryParams.end()) limit = parseIntParam(it->second, 20);

    auto result = commentService_->getComments(articleId, limit);
    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setBody(jsonOk(result));
}

void ApiHandler::handleCreateComment(const HttpRequest& req, HttpResponse* resp, int articleId) {
    try {
        auto body = nlohmann::json::parse(req.body);
        std::string content = body.value("content", "");
        std::string commenter = body.value("commenter", "匿名用户");

        auto result = commentService_->createComment(articleId, content, commenter);
        int code = result.value("code", 0);
        if (code == 400) {
            resp->setStatusCode(HttpResponse::k400BadRequest);
        } else if (code == 404) {
            resp->setStatusCode(HttpResponse::k404NotFound);
        } else if (code == 500) {
            resp->setStatusCode(HttpResponse::k500InternalError);
        } else {
            resp->setStatusCode(HttpResponse::k201Created);
        }
        resp->setBody(jsonOk(result));
    } catch (const std::exception& e) {
        resp->setStatusCode(HttpResponse::k400BadRequest);
        resp->setBody(jsonError("Invalid JSON body"));
    }
}

} // namespace lightning
