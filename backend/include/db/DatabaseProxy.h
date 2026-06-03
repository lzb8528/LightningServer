#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <optional>

namespace lightning {

struct Article {
    int id = 0;
    std::string title;
    std::string content;
    std::string author;
    std::string createTime;
    std::string updateTime;
    int viewCount = 0;
};

struct Comment {
    int id = 0;
    int articleId = 0;
    std::string content;
    std::string commenter;
    std::string createTime;
};

class DatabaseProxy {
public:
    struct Config {
        std::string host = "127.0.0.1";
        int port = 3306;
        std::string user = "lightning";
        std::string password = "lightning123";
        std::string database = "lightning_db";
        int poolSize = 8;
        int connectTimeoutSec = 5;
    };

    explicit DatabaseProxy(const Config& config);
    ~DatabaseProxy();

    bool init();
    void shutdown();

    std::vector<Article> getArticles(int page, int pageSize, int& total);
    std::optional<Article> getArticleById(int id);
    bool createArticle(const Article& article);
    bool incrementViewCount(int id, int delta);
    bool setViewCount(int id, int count);

    std::vector<Comment> getCommentsByArticleId(int articleId, int limit);
    bool createComment(const Comment& comment);
    int getCommentCount(int articleId);

    std::vector<Article> getHotArticles(int limit);

private:
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    struct Connection;
    Connection* acquireConnection();
    void releaseConnection(Connection* conn);
#endif

    Config config_;
    bool initialized_ = false;
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    std::queue<Connection*> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;
#endif
};

} // namespace lightning
