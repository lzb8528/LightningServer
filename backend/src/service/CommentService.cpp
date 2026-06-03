#include "service/CommentService.h"
#include "util/Platform.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace lightning {

CommentService::CommentService(std::shared_ptr<DatabaseProxy> db, std::shared_ptr<CacheProxy> cache)
    : db_(std::move(db)), cache_(std::move(cache)) {}

std::string CommentService::commentListKey(int articleId) const {
    return "comment:list:" + std::to_string(articleId);
}

nlohmann::json CommentService::commentToJson(const Comment& c) const {
    return {
        {"id", c.id},
        {"articleId", c.articleId},
        {"content", c.content},
        {"commenter", c.commenter},
        {"createTime", c.createTime}
    };
}

nlohmann::json CommentService::getComments(int articleId, int limit) {
    if (limit < 1 || limit > 50) limit = 20;

    auto cached = cache_->lrange(commentListKey(articleId), 0, limit - 1);
    nlohmann::json list = nlohmann::json::array();

    if (!cached.empty()) {
        for (const auto& item : cached) {
            try {
                list.push_back(nlohmann::json::parse(item));
            } catch (...) {
                spdlog::warn("Failed to parse cached comment");
            }
        }
    }

    if (list.empty()) {
        auto comments = db_->getCommentsByArticleId(articleId, limit);
        for (const auto& c : comments) {
            auto j = commentToJson(c);
            list.push_back(j);
            cache_->lpush(commentListKey(articleId), j.dump());
        }
        cache_->expire(commentListKey(articleId), 1800);
    }

    return {
        {"code", 0},
        {"data", {
            {"list", list},
            {"total", db_->getCommentCount(articleId)}
        }}
    };
}

nlohmann::json CommentService::createComment(int articleId, const std::string& content,
                                              const std::string& commenter) {
    if (content.empty() || commenter.empty()) {
        return {{"code", 400}, {"message", "content and commenter are required"}};
    }
    if (content.size() > 1000) {
        return {{"code", 400}, {"message", "content too long (max 1000 chars)"}};
    }

    auto article = db_->getArticleById(articleId);
    if (!article) {
        return {{"code", 404}, {"message", "Article not found"}};
    }

    Comment comment;
    comment.articleId = articleId;
    comment.content = content;
    comment.commenter = commenter;

    if (!db_->createComment(comment)) {
        return {{"code", 500}, {"message", "Failed to create comment"}};
    }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    lightning::platform::safeLocaltime(&time, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    comment.createTime = oss.str();

    auto j = commentToJson(comment);
    cache_->del(commentListKey(articleId));

    return {{"code", 0}, {"data", j}, {"message", "Comment created"}};
}

} // namespace lightning
