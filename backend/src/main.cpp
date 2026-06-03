#include "net/HttpServer.h"
#include "db/DatabaseProxy.h"
#include "cache/CacheProxy.h"
#include "service/ArticleService.h"
#include "service/CommentService.h"
#include "handler/ApiHandler.h"
#include "util/Config.h"

#include <spdlog/spdlog.h>
#include <functional>
#include <csignal>
#include <atomic>
#include <memory>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

// Signal handler must be minimal and async-signal-safe.
// It only sets the shutdown flag; all cleanup happens in main().
std::atomic<bool> g_running{true};

void signalHandler(int sig) {
    // Only atomic store and default signal re-raise are async-signal-safe.
    // Logging, I/O, and memory allocation are NOT safe here.
    bool wasRunning = g_running.exchange(false);
    // Re-raise default handler if signal arrives again during shutdown
    if (!wasRunning) {
        std::signal(sig, SIG_DFL);
        std::raise(sig);
    }
}

#ifdef _WIN32
BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_BREAK_EVENT || ctrlType == CTRL_CLOSE_EVENT) {
        // Windows console handler runs in its own thread — same constraint: be minimal.
        g_running.store(false);
        return TRUE;
    }
    return FALSE;
}
#endif
} // namespace

int main() {
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    // Register signal handlers
    std::signal(SIGINT, signalHandler);
#ifdef _WIN32
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#else
    std::signal(SIGTERM, signalHandler);
#endif

    auto config = lightning::ServerConfig::loadFromEnv();
    spdlog::info("LightningServer starting on {}:{}", config.host, config.port);

    lightning::DatabaseProxy::Config dbConfig;
    dbConfig.host = config.mysqlHost;
    dbConfig.port = config.mysqlPort;
    dbConfig.user = config.mysqlUser;
    dbConfig.password = config.mysqlPassword;
    dbConfig.database = config.mysqlDatabase;
    dbConfig.poolSize = config.mysqlPoolSize;

    lightning::CacheProxy::Config cacheConfig;
    cacheConfig.host = config.redisHost;
    cacheConfig.port = config.redisPort;
    cacheConfig.password = config.redisPassword;
    cacheConfig.poolSize = config.redisPoolSize;

    auto db = std::make_shared<lightning::DatabaseProxy>(dbConfig);
    auto cache = std::make_shared<lightning::CacheProxy>(cacheConfig);

    if (!db->init()) {
        spdlog::error("Failed to initialize database");
        return 1;
    }
    if (!cache->init()) {
        spdlog::error("Failed to initialize cache");
        return 1;
    }

    auto articleService = std::make_shared<lightning::ArticleService>(db, cache);
    auto commentService = std::make_shared<lightning::CommentService>(db, cache);
    auto apiHandler = std::make_shared<lightning::ApiHandler>(articleService, commentService);

    lightning::HttpServer server(config.host, static_cast<uint16_t>(config.port), config.threadNum);

    server.setHttpCallback([apiHandler](const lightning::HttpRequest& req, lightning::HttpResponse* resp) {
        apiHandler->handleRequest(req, resp);
    });

    lightning::net::steady_timer syncTimer(server.ioContext());

    std::function<void()> scheduleSync;
    scheduleSync = [&]() {
        syncTimer.expires_after(std::chrono::seconds(300));
        syncTimer.async_wait([&](const lightning::net::error_code& ec) {
            if (!ec && g_running) {
                articleService->syncViewCounts();
                scheduleSync();
            }
        });
    };
    scheduleSync();

    server.start();
    spdlog::info("LightningServer is running. Press Ctrl+C to stop.");

    // Main loop — all cleanup is done here after shutdown signal, not in the handler
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    spdlog::info("Shutting down...");
    syncTimer.cancel();
    server.stop();
    cache->shutdown();
    db->shutdown();
    spdlog::info("LightningServer stopped.");
    return 0;
}
