#pragma once

#include "db/DatabaseProxy.h"
#include "cache/CacheProxy.h"
#include <nlohmann/json.hpp>
#include <memory>

namespace lightning {

class ArticleService {
public:
    ArticleService(std::shared_ptr<DatabaseProxy> db, std::shared_ptr<CacheProxy> cache);

    nlohmann::json listArticles(int page, int pageSize);
    nlohmann::json getArticleDetail(int id);
    nlohmann::json getHotArticles(int limit);
    void syncViewCounts();

private:
    std::string articleCacheKey(int id) const;
    nlohmann::json articleToJson(const Article& a) const;

    std::shared_ptr<DatabaseProxy> db_;
    std::shared_ptr<CacheProxy> cache_;
};

} // namespace lightning
