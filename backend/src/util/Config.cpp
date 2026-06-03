#include "util/Config.h"
#include "util/Platform.h"

namespace lightning {

ServerConfig ServerConfig::loadFromEnv() {
    ServerConfig cfg;
    cfg.host = platform::safeGetenv("LS_HOST", cfg.host);
    cfg.port = platform::safeGetenvInt("LS_PORT", cfg.port);
    cfg.threadNum = platform::safeGetenvInt("LS_THREAD_NUM", cfg.threadNum);

    cfg.mysqlHost = platform::safeGetenv("MYSQL_HOST", cfg.mysqlHost);
    cfg.mysqlPort = platform::safeGetenvInt("MYSQL_PORT", cfg.mysqlPort);
    cfg.mysqlUser = platform::safeGetenv("MYSQL_USER", cfg.mysqlUser);
    cfg.mysqlPassword = platform::safeGetenv("MYSQL_PASSWORD", cfg.mysqlPassword);
    cfg.mysqlDatabase = platform::safeGetenv("MYSQL_DATABASE", cfg.mysqlDatabase);
    cfg.mysqlPoolSize = platform::safeGetenvInt("MYSQL_POOL_SIZE", cfg.mysqlPoolSize);

    cfg.redisHost = platform::safeGetenv("REDIS_HOST", cfg.redisHost);
    cfg.redisPort = platform::safeGetenvInt("REDIS_PORT", cfg.redisPort);
    cfg.redisPassword = platform::safeGetenv("REDIS_PASSWORD", cfg.redisPassword);
    cfg.redisPoolSize = platform::safeGetenvInt("REDIS_POOL_SIZE", cfg.redisPoolSize);

    return cfg;
}

} // namespace lightning
