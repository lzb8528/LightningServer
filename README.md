# LightningServer

基于 C++ 的高并发全栈 Web 应用。

## 架构概览

```
浏览器 → Nginx (静态资源 + API 反代) → C++ Asio 服务器 → MySQL / Redis
                                              ↑
                                        Vue 3 前端 SPA
```

## 技术栈

| 层次 | 技术 |
|------|------|
| 后端 | C++17, Boost.Asio, 多线程 io_context |
| 前端 | Vue 3, Vite, TypeScript, Element Plus |
| 数据库 | MySQL 8.0 |
| 缓存 | Redis 7.0 |
| 网关 | Nginx |
| 构建 | CMake 3.20+, Docker Compose |

## 快速开始（Docker 一键部署）

```bash
cd docker
docker compose up -d --build
```

启动后访问 http://localhost 即可使用。

服务端口：
- 前端：80
- 后端 API：8080
- MySQL：3306
- Redis：6379

## 本地开发

### 后端（Windows / Linux 跨平台）

网络层基于 **Boost.Asio**（未安装 Boost 时自动下载 standalone Asio，API 完全兼容）。

**Windows 依赖：** Visual Studio 2022（含 C++ 桌面开发）、CMake 3.20+

```powershell
cd backend
.\build.bat
# 或手动：
mkdir build; cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
.\Release\lightning_server.exe
```

**Linux 依赖：** g++, cmake, libmysqlclient-dev, libhiredis-dev（可选）

```bash
cd backend
./build.sh
# 或手动：
# mkdir build && cd build
# cmake .. -DCMAKE_BUILD_TYPE=Release
# cmake --build . --config Release
# ./lightning_server
```

环境变量：

| 变量 | 默认值 | 说明 |
|------|--------|------|
| LS_HOST | 0.0.0.0 | 监听地址 |
| LS_PORT | 8080 | 监听端口 |
| LS_THREAD_NUM | 4 | IO 线程数 |
| MYSQL_HOST | 127.0.0.1 | MySQL 地址 |
| MYSQL_PORT | 3306 | MySQL 端口 |
| MYSQL_USER | lightning | 数据库用户 |
| MYSQL_PASSWORD | lightning123 | 数据库密码 |
| MYSQL_DATABASE | lightning_db | 数据库名 |
| REDIS_HOST | 127.0.0.1 | Redis 地址 |
| REDIS_PORT | 6379 | Redis 端口 |

> 未安装 MySQL/Redis 时，后端会自动启用 Mock 模式（内置示例数据），方便前端联调。

### 前端

```bash
cd frontend
npm install
npm run dev
```

开发服务器运行在 http://localhost:5173，API 请求自动代理到后端 8080 端口。

## 项目结构

```
LightningServer/
├── backend/                # C++ 后端
│   ├── include/            # 头文件
│   │   ├── net/            # 网络框架 (Boost.Asio HTTP)
│   │   ├── db/             # MySQL 连接池
│   │   ├── cache/          # Redis 连接池
│   │   ├── service/        # 业务逻辑
│   │   └── handler/        # API 路由处理
│   ├── src/                # 源文件
│   └── CMakeLists.txt
├── frontend/               # Vue 3 前端
│   ├── src/
│   │   ├── views/          # 页面组件
│   │   ├── components/     # 通用组件
│   │   └── api/            # API 封装
│   └── package.json
├── docker/                 # Docker 部署
│   ├── docker-compose.yml
│   ├── mysql/init.sql      # 数据库初始化
│   └── nginx/nginx.conf    # 反向代理配置
├── docs/
│   └── API.md              # REST API 文档
└── plan.md                 # 项目开发计划
```

## API 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | /api/health | 健康检查 |
| GET | /api/articles | 文章列表（分页） |
| GET | /api/articles/:id | 文章详情 |
| GET | /api/articles/hot | 热门排行 |
| GET | /api/articles/:id/comments | 评论列表 |
| POST | /api/articles/:id/comments | 发表评论 |

详细文档见 [docs/API.md](docs/API.md)。

## 性能测试

```bash
# 安装 wrk
# Ubuntu: sudo apt install wrk

# 文章列表压测
wrk -t4 -c256 -d30s http://localhost:8080/api/articles

# 文章详情压测（缓存命中）
wrk -t4 -c256 -d30s http://localhost:8080/api/articles/1
```

## 开发阶段

- [x] 阶段一：C++ Asio 网络框架 + HTTP 服务器（跨平台）
- [x] 阶段二：Vue 3 前端 + Mock 联调
- [x] 阶段三：MySQL / Redis 集成 + Cache-Aside
- [x] 阶段四：评论系统 + 热门排行 + 前端对接
- [ ] 阶段五：性能压测与优化
- [x] 阶段六：Docker Compose 部署 + 文档

## License

MIT
