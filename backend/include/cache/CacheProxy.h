#pragma once

#include <string>
#include <vector>
#include <optional>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace lightning {

class CacheProxy {
public:
    struct Config {
        std::string host = "127.0.0.1";
        int port = 6379;
        std::string password;
        int poolSize = 4;
        int defaultTtlSec = 3600;
    };

    explicit CacheProxy(const Config& config);
    ~CacheProxy();

    bool init();
    void shutdown();

    std::optional<std::string> get(const std::string& key);
    bool set(const std::string& key, const std::string& value, int ttlSec = -1);
    bool del(const std::string& key);
    int64_t incr(const std::string& key);
    bool zadd(const std::string& key, double score, const std::string& member);
    std::vector<std::string> zrevrange(const std::string& key, int start, int stop);
    bool lpush(const std::string& key, const std::string& value);
    std::vector<std::string> lrange(const std::string& key, int start, int stop);
    bool expire(const std::string& key, int ttlSec);

private:
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    struct RedisConn;
    RedisConn* acquireConnection();
    void releaseConnection(RedisConn* conn);
#endif

    Config config_;
    bool initialized_ = false;
#if defined(HAS_REDIS) && !defined(MOCK_CACHE)
    std::queue<RedisConn*> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;
#endif
};

} // namespace lightning
