#pragma once

#include "net/HttpParser.h"
#include "service/ArticleService.h"
#include "service/CommentService.h"
#include <memory>

namespace lightning {

class ApiHandler {
public:
    ApiHandler(std::shared_ptr<ArticleService> articleService,
               std::shared_ptr<CommentService> commentService);

    void handleRequest(const HttpRequest& req, HttpResponse* resp);

private:
    void handleGetArticles(const HttpRequest& req, HttpResponse* resp);
    void handleGetArticleDetail(const HttpRequest& req, HttpResponse* resp, int id);
    void handleGetHotArticles(const HttpRequest& req, HttpResponse* resp);
    void handleGetComments(const HttpRequest& req, HttpResponse* resp, int articleId);
    void handleCreateComment(const HttpRequest& req, HttpResponse* resp, int articleId);

    int parseIntParam(const std::string& s, int defaultVal) const;
    std::string jsonOk(const nlohmann::json& data) const;
    std::string jsonError(const std::string& msg, int code = 400) const;

    std::shared_ptr<ArticleService> articleService_;
    std::shared_ptr<CommentService> commentService_;
};

} // namespace lightning
