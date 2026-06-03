#pragma once

#include <string>

namespace lightning {

struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    int threadNum = 4;

    std::string mysqlHost = "127.0.0.1";
    int mysqlPort = 3306;
    std::string mysqlUser = "lightning";
    std::string mysqlPassword = "lightning123";
    std::string mysqlDatabase = "lightning_db";
    int mysqlPoolSize = 8;

    std::string redisHost = "127.0.0.1";
    int redisPort = 6379;
    std::string redisPassword;
    int redisPoolSize = 4;

    static ServerConfig loadFromEnv();
};

} // namespace lightning
