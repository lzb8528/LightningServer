#include "service/ArticleService.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace lightning {

ArticleService::ArticleService(std::shared_ptr<DatabaseProxy> db, std::shared_ptr<CacheProxy> cache)
    : db_(std::move(db)), cache_(std::move(cache)) {}

std::string ArticleService::articleCacheKey(int id) const {
    return "article:info:" + std::to_string(id);
}

nlohmann::json ArticleService::articleToJson(const Article& a) const {
    return {
        {"id", a.id},
        {"title", a.title},
        {"content", a.content},
        {"author", a.author},
        {"createTime", a.createTime},
        {"updateTime", a.updateTime},
        {"viewCount", a.viewCount}
    };
}

nlohmann::json ArticleService::listArticles(int page, int pageSize) {
    if (page < 1) page = 1;
    if (pageSize < 1 || pageSize > 50) pageSize = 10;

    int total = 0;
    auto articles = db_->getArticles(page, pageSize, total);

    nlohmann::json list = nlohmann::json::array();
    for (const auto& a : articles) {
        nlohmann::json item = articleToJson(a);
        item.erase("content");
        list.push_back(item);
    }

    return {
        {"code", 0},
        {"data", {
            {"list", list},
            {"total", total},
            {"page", page},
            {"pageSize", pageSize}
        }}
    };
}

nlohmann::json ArticleService::getArticleDetail(int id) {
    std::string cacheKey = articleCacheKey(id);
    auto cached = cache_->get(cacheKey);

    if (cached) {
        spdlog::debug("Cache hit for article {}", id);
        return nlohmann::json::parse(*cached);
    }

    auto article = db_->getArticleById(id);
    if (!article) {
        return {{"code", 404}, {"message", "Article not found"}};
    }

    int64_t redisViews = cache_->incr("article:view:" + std::to_string(id));
    article->viewCount = std::max(article->viewCount, static_cast<int>(redisViews));

    cache_->zadd("hot:articles:weekly", redisViews, std::to_string(id));

    nlohmann::json result = {
        {"code", 0},
        {"data", articleToJson(*article)}
    };

    cache_->set(cacheKey, result.dump(), 3600);
    return result;
}

nlohmann::json ArticleService::getHotArticles(int limit) {
    if (limit < 1 || limit > 20) limit = 10;

    auto hotIds = cache_->zrevrange("hot:articles:weekly", 0, limit - 1);
    nlohmann::json list = nlohmann::json::array();

    if (!hotIds.empty()) {
        for (const auto& idStr : hotIds) {
            int id = std::stoi(idStr);
            auto article = db_->getArticleById(id);
            if (article) {
                auto viewStr = cache_->get("article:view:" + idStr);
                if (viewStr) {
                    article->viewCount = std::max(article->viewCount, std::stoi(*viewStr));
                }
                nlohmann::json item = articleToJson(*article);
                item.erase("content");
                list.push_back(item);
            }
        }
    } else {
        auto articles = db_->getHotArticles(limit);
        for (const auto& a : articles) {
            nlohmann::json item = articleToJson(a);
            item.erase("content");
            list.push_back(item);
        }
    }

    return {{"code", 0}, {"data", list}};
}

void ArticleService::syncViewCounts() {
    auto hotIds = cache_->zrevrange("hot:articles:weekly", 0, 99);
    for (const auto& idStr : hotIds) {
        auto viewStr = cache_->get("article:view:" + idStr);
        if (viewStr) {
            db_->setViewCount(std::stoi(idStr), std::stoi(*viewStr));
        }
    }
    spdlog::info("Synced view counts for {} articles", hotIds.size());
}

} // namespace lightning
