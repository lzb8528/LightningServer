LightningServer 离线依赖指引
=============================

当前网络无法访问 GitHub/Gitee，需要手动下载 3 个依赖（都是纯头文件，无需编译）。

操作步骤：
---------

1. 在能上网的机器下载以下文件：

  [1] spdlog v1.14.1
      https://github.com/gabime/spdlog/archive/refs/tags/v1.14.1.zip
      解压后把 include/spdlog/ 整个文件夹放到：
      backend\deps\spdlog\include\spdlog\

  [2] nlohmann/json v3.11.3  (单文件！)
      https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
      把这个文件放到：
      backend\deps\json\include\nlohmann\json.hpp

  [3] Asio v1.30.2
      https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.zip
      解压后把 asio/include/ 文件夹放到：
      backend\deps\asio\asio\include\

2. 复制整个 backend\deps\ 文件夹到你的开发机器上

3. 运行:
      cd backend
      build.bat

最终目录结构：
-------------

backend\deps\
├── spdlog\
│   └── include\
│       └── spdlog\
│           ├── spdlog.h
│           ├── logger.h
│           ├── ... (很多 .h 文件)
├── json\
│   └── include\
│       └── nlohmann\
│           └── json.hpp          ← 单文件
└── asio\
    └── asio\
        └── include\
            ├── asio.hpp
            ├── ... (很多 .hpp 文件)
