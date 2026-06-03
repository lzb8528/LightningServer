#include "cache/CacheProxy.h"
#include <spdlog/spdlog.h>

#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
#include <hiredis/hiredis.h>
#endif

namespace lightning {

#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
struct CacheProxy::RedisConn {
    redisContext* ctx = nullptr;
};
#endif

#if defined(MOCK_CACHE)
#include <unordered_map>
static std::unordered_map<std::string, std::string> mockStore;
static std::unordered_map<std::string, int64_t> mockCounters;
#endif

CacheProxy::CacheProxy(const Config& config) : config_(config) {}

CacheProxy::~CacheProxy() {
    shutdown();
}

bool CacheProxy::init() {
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    for (int i = 0; i < config_.poolSize; ++i) {
        auto* conn = new RedisConn();
        conn->ctx = redisConnect(config_.host.c_str(), config_.port);
        if (!conn->ctx || conn->ctx->err) {
            spdlog::error("Redis connect failed: {}",
                conn->ctx ? conn->ctx->errstr : "null context");
            return false;
        }
        if (!config_.password.empty()) {
            auto* reply = static_cast<redisReply*>(
                redisCommand(conn->ctx, "AUTH %s", config_.password.c_str()));
            if (reply) freeReplyObject(reply);
        }
        pool_.push(conn);
    }
    spdlog::info("Redis connection pool initialized (size={})", config_.poolSize);
#else
    spdlog::warn("Running with MOCK cache (no Redis)");
#endif
    initialized_ = true;
    return true;
}

void CacheProxy::shutdown() {
    if (!initialized_) return;
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    std::lock_guard<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        auto* conn = pool_.front();
        pool_.pop();
        redisFree(conn->ctx);
        delete conn;
    }
#endif
    initialized_ = false;
}

#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
CacheProxy::RedisConn* CacheProxy::acquireConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !pool_.empty(); });
    auto* conn = pool_.front();
    pool_.pop();
    return conn;
}

void CacheProxy::releaseConnection(RedisConn* conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push(conn);
    cv_.notify_one();
}
#endif

std::optional<std::string> CacheProxy::get(const std::string& key) {
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    auto* conn = acquireConnection();
    auto* reply = static_cast<redisReply*>(redisCommand(conn->ctx, "GET %s", key.c_str()));
    std::optional<std::string> result;
    if (reply && reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }
    if (reply) freeReplyObject(reply);
    releaseConnection(conn);
    return result;
#else
    auto it = mockStore.find(key);
    if (it != mockStore.end()) return it->second;
    return std::nullopt;
#endif
}

bool CacheProxy::set(const std::string& key, const std::string& value, int ttlSec) {
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    auto* conn = acquireConnection();
    redisReply* reply = nullptr;
    if (ttlSec > 0) {
        reply = static_cast<redisReply*>(
            redisCommand(conn->ctx, "SETEX %s %d %s", key.c_str(), ttlSec, value.c_str()));
    } else {
        reply = static_cast<redisReply*>(
            redisCommand(conn->ctx, "SET %s %s", key.c_str(), value.c_str()));
    }
    bool ok = reply && reply->type == REDIS_REPLY_STATUS;
    if (reply) freeReplyObject(reply);
    releaseConnection(conn);
    return ok;
#else
    mockStore[key] = value;
    (void)ttlSec;
    return true;
#endif
}

bool CacheProxy::del(const std::string& key) {
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    auto* conn = acquireConnection();
    auto* reply = static_cast<redisReply*>(redisCommand(conn->ctx, "DEL %s", key.c_str()));
    bool ok = reply && reply->integer > 0;
    if (reply) freeReplyObject(reply);
    releaseConnection(conn);
    return ok;
#else
    return mockStore.erase(key) > 0;
#endif
}

int64_t CacheProxy::incr(const std::string& key) {
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    auto* conn = acquireConnection();
    auto* reply = static_cast<redisReply*>(redisCommand(conn->ctx, "INCR %s", key.c_str()));
    int64_t val = reply ? reply->integer : 0;
    if (reply) freeReplyObject(reply);
    releaseConnection(conn);
    return val;
#else
    return ++mockCounters[key];
#endif
}

bool CacheProxy::zadd(const std::string& key, double score, const std::string& member) {
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    auto* conn = acquireConnection();
    auto* reply = static_cast<redisReply*>(
        redisCommand(conn->ctx, "ZADD %s %f %s", key.c_str(), score, member.c_str()));
    bool ok = reply != nullptr;
    if (reply) freeReplyObject(reply);
    releaseConnection(conn);
    return ok;
#else
    (void)key; (void)score; (void)member;
    return true;
#endif
}

std::vector<std::string> CacheProxy::zrevrange(const std::string& key, int start, int stop) {
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    auto* conn = acquireConnection();
    auto* reply = static_cast<redisReply*>(
        redisCommand(conn->ctx, "ZREVRANGE %s %d %d", key.c_str(), start, stop));
    std::vector<std::string> result;
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            result.emplace_back(reply->element[i]->str, reply->element[i]->len);
        }
    }
    if (reply) freeReplyObject(reply);
    releaseConnection(conn);
    return result;
#else
    (void)key; (void)start; (void)stop;
    return {};
#endif
}

bool CacheProxy::lpush(const std::string& key, const std::string& value) {
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    auto* conn = acquireConnection();
    auto* reply = static_cast<redisReply*>(
        redisCommand(conn->ctx, "LPUSH %s %s", key.c_str(), value.c_str()));
    bool ok = reply != nullptr;
    if (reply) freeReplyObject(reply);
    releaseConnection(conn);
    return ok;
#else
    mockStore[key] = value + (mockStore.count(key) ? mockStore[key] : "");
    return true;
#endif
}

std::vector<std::string> CacheProxy::lrange(const std::string& key, int start, int stop) {
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    auto* conn = acquireConnection();
    auto* reply = static_cast<redisReply*>(
        redisCommand(conn->ctx, "LRANGE %s %d %d", key.c_str(), start, stop));
    std::vector<std::string> result;
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            result.emplace_back(reply->element[i]->str, reply->element[i]->len);
        }
    }
    if (reply) freeReplyObject(reply);
    releaseConnection(conn);
    return result;
#else
    (void)start; (void)stop;
    if (mockStore.count(key)) return {mockStore[key]};
    return {};
#endif
}

bool CacheProxy::expire(const std::string& key, int ttlSec) {
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    auto* conn = acquireConnection();
    auto* reply = static_cast<redisReply*>(
        redisCommand(conn->ctx, "EXPIRE %s %d", key.c_str(), ttlSec));
    bool ok = reply && reply->integer == 1;
    if (reply) freeReplyObject(reply);
    releaseConnection(conn);
    return ok;
#else
    (void)key; (void)ttlSec;
    return true;
#endif
}

} // namespace lightning
