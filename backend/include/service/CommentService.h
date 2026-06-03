#pragma once

#include "db/DatabaseProxy.h"
#include "cache/CacheProxy.h"
#include <nlohmann/json.hpp>
#include <memory>

namespace lightning {

class CommentService {
public:
    CommentService(std::shared_ptr<DatabaseProxy> db, std::shared_ptr<CacheProxy> cache);

    nlohmann::json getComments(int articleId, int limit = 20);
    nlohmann::json createComment(int articleId, const std::string& content, const std::string& commenter);

private:
    std::string commentListKey(int articleId) const;
    nlohmann::json commentToJson(const Comment& c) const;

    std::shared_ptr<DatabaseProxy> db_;
    std::shared_ptr<CacheProxy> cache_;
};

} // namespace lightning
