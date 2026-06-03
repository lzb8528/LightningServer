#include "db/DatabaseProxy.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

#if defined(HAS_MYSQL) && !defined(MOCK_DB)
#include <mysql/mysql.h>
#endif

namespace lightning {

#if defined(HAS_MYSQL) && !defined(MOCK_DB)
struct DatabaseProxy::Connection {
    MYSQL* mysql = nullptr;
};

// Escape a string for safe use in a MySQL SQL statement.
// Uses mysql_real_escape_string() which handles the connection charset.
static std::string escapeString(Connection* conn, const std::string& raw) {
    if (raw.empty()) return raw;
    std::string escaped(raw.size() * 2 + 1, '\0');
    unsigned long len = mysql_real_escape_string(conn->mysql,
        escaped.data(), raw.c_str(), static_cast<unsigned long>(raw.size()));
    escaped.resize(len);
    return escaped;
}
#endif

#if defined(MOCK_DB)
static std::vector<Article> mockArticles() {
    return {
        {1, "高并发服务器架构设计", "Reactor 模式是高性能网络编程的核心...", "张三", "2026-06-01 10:00:00", "2026-06-01 10:00:00", 1280},
        {2, "Redis 缓存最佳实践", "Cache-Aside 模式在读写分离场景下的应用...", "李四", "2026-06-01 11:30:00", "2026-06-01 11:30:00", 856},
        {3, "Vue 3 组合式 API 入门", "使用 Composition API 构建可维护的前端组件...", "王五", "2026-06-01 14:00:00", "2026-06-01 14:00:00", 642},
        {4, "MySQL 连接池优化指南", "连接池大小、超时设置与慢查询分析...", "赵六", "2026-06-02 09:00:00", "2026-06-02 09:00:00", 423},
        {5, "epoll 与 select 性能对比", "Linux 下 I/O 多路复用机制深度解析...", "张三", "2026-06-02 15:30:00", "2026-06-02 15:30:00", 1105},
    };
}

static std::vector<Comment> mockComments() {
    return {
        {1, 1, "写得很详细，Reactor 模式讲解清晰！", "读者A", "2026-06-01 12:00:00"},
        {2, 1, "One Loop Per Thread 的设计思路很有启发", "读者B", "2026-06-01 13:30:00"},
        {3, 2, "Cache-Aside 模式在实际项目中确实好用", "读者C", "2026-06-01 16:00:00"},
    };
}
#endif

DatabaseProxy::DatabaseProxy(const Config& config) : config_(config) {}

DatabaseProxy::~DatabaseProxy() {
    shutdown();
}

bool DatabaseProxy::init() {
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    for (int i = 0; i < config_.poolSize; ++i) {
        auto* conn = new Connection();
        conn->mysql = mysql_init(nullptr);
        if (!conn->mysql) {
            spdlog::error("mysql_init failed");
            return false;
        }

        unsigned int timeout = config_.connectTimeoutSec;
        mysql_options(conn->mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

        if (!mysql_real_connect(conn->mysql, config_.host.c_str(), config_.user.c_str(),
                                config_.password.c_str(), config_.database.c_str(),
                                config_.port, nullptr, 0)) {
            spdlog::error("MySQL connect failed: {}", mysql_error(conn->mysql));
            mysql_close(conn->mysql);
            delete conn;
            return false;
        }
        mysql_set_character_set(conn->mysql, "utf8mb4");
        pool_.push(conn);
    }
    spdlog::info("MySQL connection pool initialized (size={})", config_.poolSize);
#else
    spdlog::warn("Running with MOCK database (no MySQL)");
#endif
    initialized_ = true;
    return true;
}

void DatabaseProxy::shutdown() {
    if (!initialized_) return;
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    std::lock_guard<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        auto* conn = pool_.front();
        pool_.pop();
        mysql_close(conn->mysql);
        delete conn;
    }
#endif
    initialized_ = false;
}

#if defined(HAS_MYSQL) && !defined(MOCK_DB)
DatabaseProxy::Connection* DatabaseProxy::acquireConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::seconds(config_.connectTimeoutSec), [this] {
        return !pool_.empty();
    });
    if (pool_.empty()) return nullptr;
    auto* conn = pool_.front();
    pool_.pop();
    return conn;
}

void DatabaseProxy::releaseConnection(Connection* conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push(conn);
    cv_.notify_one();
}
#endif

std::vector<Article> DatabaseProxy::getArticles(int page, int pageSize, int& total) {
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    auto* conn = acquireConnection();
    if (!conn) return {};

    std::string countSql = "SELECT COUNT(*) FROM articles";
    if (mysql_query(conn->mysql, countSql.c_str()) != 0) {
        spdlog::error("MySQL query error: {}", mysql_error(conn->mysql));
        releaseConnection(conn);
        return {};
    }
    MYSQL_RES* countRes = mysql_store_result(conn->mysql);
    MYSQL_ROW countRow = mysql_fetch_row(countRes);
    total = countRow ? std::stoi(countRow[0]) : 0;
    mysql_free_result(countRes);

    int offset = (page - 1) * pageSize;
    std::ostringstream sql;
    sql << "SELECT id, title, content, author, create_time, update_time, view_count "
        << "FROM articles ORDER BY create_time DESC LIMIT " << pageSize << " OFFSET " << offset;

    if (mysql_query(conn->mysql, sql.str().c_str()) != 0) {
        spdlog::error("MySQL query error: {}", mysql_error(conn->mysql));
        releaseConnection(conn);
        return {};
    }

    std::vector<Article> articles;
    MYSQL_RES* result = mysql_store_result(conn->mysql);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Article a;
        a.id = std::stoi(row[0]);
        a.title = row[1] ? row[1] : "";
        a.content = row[2] ? row[2] : "";
        a.author = row[3] ? row[3] : "";
        a.createTime = row[4] ? row[4] : "";
        a.updateTime = row[5] ? row[5] : "";
        a.viewCount = row[6] ? std::stoi(row[6]) : 0;
        articles.push_back(a);
    }
    mysql_free_result(result);
    releaseConnection(conn);
    return articles;
#else
    auto all = mockArticles();
    total = static_cast<int>(all.size());
    int offset = (page - 1) * pageSize;
    if (offset >= total) return {};
    int end = std::min(offset + pageSize, total);
    return std::vector<Article>(all.begin() + offset, all.begin() + end);
#endif
}

std::optional<Article> DatabaseProxy::getArticleById(int id) {
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    auto* conn = acquireConnection();
    if (!conn) return std::nullopt;

    std::string sql = "SELECT id, title, content, author, create_time, update_time, view_count "
                      "FROM articles WHERE id = " + std::to_string(id);

    if (mysql_query(conn->mysql, sql.c_str()) != 0) {
        spdlog::error("MySQL query error: {}", mysql_error(conn->mysql));
        releaseConnection(conn);
        return std::nullopt;
    }

    MYSQL_RES* result = mysql_store_result(conn->mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    std::optional<Article> article;
    if (row) {
        Article a;
        a.id = std::stoi(row[0]);
        a.title = row[1] ? row[1] : "";
        a.content = row[2] ? row[2] : "";
        a.author = row[3] ? row[3] : "";
        a.createTime = row[4] ? row[4] : "";
        a.updateTime = row[5] ? row[5] : "";
        a.viewCount = row[6] ? std::stoi(row[6]) : 0;
        article = a;
    }
    mysql_free_result(result);
    releaseConnection(conn);
    return article;
#else
    for (const auto& a : mockArticles()) {
        if (a.id == id) return a;
    }
    return std::nullopt;
#endif
}

bool DatabaseProxy::createArticle(const Article& article) {
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    auto* conn = acquireConnection();
    if (!conn) return false;

    std::ostringstream sql;
    sql << "INSERT INTO articles (title, content, author) VALUES ('"
        << escapeString(conn, article.title) << "', '"
        << escapeString(conn, article.content) << "', '"
        << escapeString(conn, article.author) << "')";

    bool ok = mysql_query(conn->mysql, sql.str().c_str()) == 0;
    releaseConnection(conn);
    return ok;
#else
    (void)article;
    return true;
#endif
}

bool DatabaseProxy::incrementViewCount(int id, int delta) {
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    auto* conn = acquireConnection();
    if (!conn) return false;

    std::string sql = "UPDATE articles SET view_count = view_count + " +
                      std::to_string(delta) + " WHERE id = " + std::to_string(id);
    bool ok = mysql_query(conn->mysql, sql.c_str()) == 0;
    releaseConnection(conn);
    return ok;
#else
    (void)id; (void)delta;
    return true;
#endif
}

bool DatabaseProxy::setViewCount(int id, int count) {
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    auto* conn = acquireConnection();
    if (!conn) return false;

    std::string sql = "UPDATE articles SET view_count = " +
                      std::to_string(count) + " WHERE id = " + std::to_string(id);
    bool ok = mysql_query(conn->mysql, sql.c_str()) == 0;
    releaseConnection(conn);
    return ok;
#else
    (void)id; (void)count;
    return true;
#endif
}

std::vector<Comment> DatabaseProxy::getCommentsByArticleId(int articleId, int limit) {
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    auto* conn = acquireConnection();
    if (!conn) return {};

    std::ostringstream sql;
    sql << "SELECT id, article_id, content, commenter, create_time FROM comments "
        << "WHERE article_id = " << articleId << " ORDER BY create_time DESC LIMIT " << limit;

    if (mysql_query(conn->mysql, sql.str().c_str()) != 0) {
        releaseConnection(conn);
        return {};
    }

    std::vector<Comment> comments;
    MYSQL_RES* result = mysql_store_result(conn->mysql);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Comment c;
        c.id = std::stoi(row[0]);
        c.articleId = std::stoi(row[1]);
        c.content = row[2] ? row[2] : "";
        c.commenter = row[3] ? row[3] : "";
        c.createTime = row[4] ? row[4] : "";
        comments.push_back(c);
    }
    mysql_free_result(result);
    releaseConnection(conn);
    return comments;
#else
    std::vector<Comment> result;
    for (const auto& c : mockComments()) {
        if (c.articleId == articleId) result.push_back(c);
    }
    if (static_cast<int>(result.size()) > limit) {
        result.resize(limit);
    }
    return result;
#endif
}

bool DatabaseProxy::createComment(const Comment& comment) {
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    auto* conn = acquireConnection();
    if (!conn) return false;

    std::ostringstream sql;
    sql << "INSERT INTO comments (article_id, content, commenter) VALUES ("
        << comment.articleId << ", '"
        << escapeString(conn, comment.content) << "', '"
        << escapeString(conn, comment.commenter) << "')";

    bool ok = mysql_query(conn->mysql, sql.str().c_str()) == 0;
    releaseConnection(conn);
    return ok;
#else
    (void)comment;
    return true;
#endif
}

int DatabaseProxy::getCommentCount(int articleId) {
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    auto* conn = acquireConnection();
    if (!conn) return 0;

    std::string sql = "SELECT COUNT(*) FROM comments WHERE article_id = " + std::to_string(articleId);
    mysql_query(conn->mysql, sql.c_str());
    MYSQL_RES* result = mysql_store_result(conn->mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = row ? std::stoi(row[0]) : 0;
    mysql_free_result(result);
    releaseConnection(conn);
    return count;
#else
    int count = 0;
    for (const auto& c : mockComments()) {
        if (c.articleId == articleId) ++count;
    }
    return count;
#endif
}

std::vector<Article> DatabaseProxy::getHotArticles(int limit) {
#if defined(HAS_MYSQL) && !defined(MOCK_DB)
    auto* conn = acquireConnection();
    if (!conn) return {};

    std::ostringstream sql;
    sql << "SELECT id, title, content, author, create_time, update_time, view_count "
        << "FROM articles ORDER BY view_count DESC LIMIT " << limit;

    if (mysql_query(conn->mysql, sql.str().c_str()) != 0) {
        releaseConnection(conn);
        return {};
    }

    std::vector<Article> articles;
    MYSQL_RES* result = mysql_store_result(conn->mysql);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Article a;
        a.id = std::stoi(row[0]);
        a.title = row[1] ? row[1] : "";
        a.content = row[2] ? row[2] : "";
        a.author = row[3] ? row[3] : "";
        a.createTime = row[4] ? row[4] : "";
        a.updateTime = row[5] ? row[5] : "";
        a.viewCount = row[6] ? std::stoi(row[6]) : 0;
        articles.push_back(a);
    }
    mysql_free_result(result);
    releaseConnection(conn);
    return articles;
#else
    auto all = mockArticles();
    std::sort(all.begin(), all.end(), [](const Article& a, const Article& b) {
        return a.viewCount > b.viewCount;
    });
    if (static_cast<int>(all.size()) > limit) all.resize(limit);
    return all;
#endif
}

} // namespace lightning
