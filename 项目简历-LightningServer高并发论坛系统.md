# LightningServer — 高并发热点文章论坛系统

> **项目类型**：全栈 Web 应用 | **开发周期**：17天 | **代码规模**：后端 3000+ 行 C++ / 前端 1200+ 行 TypeScript+Vue

---

## 一、项目介绍

**LightningServer** 是一个从零构建的**高并发热点文章讨论社区**，基于 C++17 自研多线程 HTTP 服务器，采用 Reactor 事件驱动模型，支持 C10K（万级并发连接），目标 P99 延迟 < 10ms（缓存命中场景）。

系统围绕"热点内容实时排行"这一核心场景，实现了 **Cache-Aside 缓存策略**、**Redis 原子计数器**、**Sorted Set 热度排行**、**MySQL 连接池**、**Redis 连接池**、**定时异步数据同步**等后端核心机制。前端采用 Vue 3 + TypeScript + Element Plus 构建 SPA，支持文章列表分页、文章详情、热门排行、评论互动等功能。

项目支持 **Windows / Linux / macOS** 跨平台编译部署，兼容 MSVC / MinGW / GCC / Clang 四种编译器，提供 Docker Compose 一键部署方案，并在无 MySQL/Redis 环境下自动降级为 Mock 模式运行。

---

## 二、项目技术栈

### 后端

| 分层 | 技术 | 说明 |
|------|------|------|
| 语言标准 | C++17 | 严格模式，禁用编译器扩展 |
| 网络层 | Boost.Asio 1.74+ / standalone Asio 1.30.2 | 跨平台异步 I/O，编译期自动切换 |
| 线程模型 | One Loop Per Thread（Reactor 模式） | 单 `io_context` 多线程竞争消费事件 |
| HTTP 协议 | 自研 HTTP/1.1 解析器 | 状态机解析请求行/头/体，支持 Keep-Alive |
| 序列化 | nlohmann/json v3.11.3 | 请求解析与 JSON 响应构造 |
| 日志 | spdlog v1.14.1 | 异步日志，带时间戳和级别着色 |
| 数据库 | MySQL 8.0 | 连接池（默认 8 个连接），参数化查询 |
| 缓存 | Redis 7.0 | 连接池（默认 4 个连接），Sorted Set / List / String |
| 构建系统 | CMake 3.20+ | 四级依赖回退策略，支持 FetchContent |
| 部署 | Docker / Docker Compose / Nginx | 多阶段构建，4 容器编排 |

### 前端

| 分层 | 技术 | 说明 |
|------|------|------|
| 框架 | Vue 3.4 (Composition API) | TypeScript 严格模式 |
| UI 库 | Element Plus 2.7 | 中文国际化 |
| 路由 | Vue Router 4.3 | 懒加载路由，代码分割 |
| HTTP | Axios 1.7 | 统一拦截器，类型化响应 |
| 构建 | Vite 5.3 | 分包策略（vue-vendor / element-plus 独立 chunk） |

### 基础设施

| 组件 | 技术 |
|------|------|
| 反向代理 | Nginx（gzip，7天静态缓存，SPA fallback） |
| 容器化 | Docker Compose（MySQL + Redis + 后端 + 前端 4 服务） |
| 数据库初始化 | SQL DDL + 种子数据（5篇文章 + 5条评论） |
| IDE 支持 | VS Code（4套 IntelliSense 配置：MinGW / MSVC / GCC / Clang） |

---

## 三、负责的项目内容与职责

### 架构设计
- 设计并实现 **Reactor 多线程 HTTP 服务器架构**，包括 TCP 接收器（`acceptor`）、IO 线程池、异步读写回话管理
- 设计 **分层架构**：网络层 → 数据库/缓存层 → 业务服务层 → API 路由层，实现关注点分离
- 设计 **Redis Key 体系**：String 缓存文章详情、原子计数器、Sorted Set 热点排行、List 评论列表缓存
- 设计 **MySQL 表结构**（articles / comments），包含索引策略（create_time / article_id）和外键约束

### 后端开发（C++17）
- 独立实现 **HTTP/1.1 协议解析器**：状态机解析请求行、Header、Query String、Body（Content-Length），支持 Keep-Alive 长连接
- 实现 **应用层读写缓冲区**（`Buffer` 类）：生产者-消费者模型，动态扩容（1KB → 64MB），紧凑化整理
- 实现 **MySQL 连接池**（8 连接）：条件变量阻塞等待、超时机制、`utf8mb4` 字符集、连接复用
- 实现 **Redis 连接池**（4 连接）：支持 String / Sorted Set / List / 过期等 8 种操作
- 实现 **API 路由分发器**（`ApiHandler`）：`std::regex` 路径匹配，6 个 RESTful 端点
- 实现 **缓存策略**：文章详情的 Cache-Aside 读缓存、评论区缓存失效写策略
- 实现 **实时热度排行**：Redis Sorted Set + 原子计数器，5 分钟定时同步至 MySQL
- 实现 **跨平台兼容层**（`Platform.h`）：统一 `localtime`、`getenv` 等 系统 API 差异
- 实现 **优雅启停**：异步信号安全的信号处理、工作守卫（work guard）控制事件循环生命周期

### 前端开发（Vue 3 + TypeScript）
- 构建 3 个页面（文章列表、文章详情含评论区、热门排行）+ 1 个可复用评论区组件
- 实现 **骨架屏加载态**、空状态提示、分页逻辑、Top-3 徽章渐变样式
- 封装 Axios 客户端：统一 `ApiResponse<T>` 泛型返回、错误拦截、类型安全

### 部署与工程化
- 编写 **CMakeLists.txt**：四级依赖回退（本地 deps/ → 用户指定路径 → find_package → FetchContent）
- 编写跨平台构建脚本（`build.bat` / `build.sh`）：自动检测编译器、依赖检查、安装提示
- 编写 **PowerShell + Bash 离线依赖下载脚本**：GitHub → Gitee 镜像自动回退
- 构建 **Docker Compose 编排**：4 服务 + 健康检查 + 数据卷 + 多阶段 Dockerfile
- 配置 **Nginx 反向代理**：API 转发、静态资源 7 天缓存、gzip压缩、SPA fallback
- 配置 **VS Code 多编译器 IntelliSense**：Win32 MinGW / Win32 MSVC / Linux GCC / Linux Clang

### 质量与安全
- 修复 **SQL 注入漏洞**：对用户输入字段（标题、内容、作者、评论者）使用 `mysql_real_escape_string()` 安全转义
- 修复 **信号处理器线程安全**：信号处理函数仅执行原子操作，所有清理逻辑移至主循环
- 完善 `.gitignore`、构建输出隔离、Mock 降级模式保证开发体验

---

## 四、项目难点与技术亮点

### 4.1 Reactor 模式下的优雅启停

**难点**：Asio 的 `io_context` 在多线程模式下，信号到达时需要安全地停止所有工作线程，不能丢失正在处理的请求，也不能死锁。

**解决方案**：
- 设计 `workGuard_` 工作守卫：正常运行时持有 `executor_work_guard` 防止 `io_context::run()` 提前返回；关闭时 `reset()` 释放守卫
- 信号处理函数**仅执行原子操作**（`g_running.store(false)`），严格遵循异步信号安全原则——日志、内存分配、锁操作全部禁止
- 主循环检测到 `g_running == false` 后，依次执行：取消定时器 → `acceptor.close()` 阻断新连接 → `workGuard.reset()` → `io_context.stop()` → `join` 所有线程 → 关闭数据库/缓存连接池

### 4.2 Cache-Aside 模式 + 实时热度排行

**难点**：文章浏览量需要实时展示（Redis 原子计数器），同时定期持久化到 MySQL，且排行榜始终反映最新热度。

**解决方案**：
- **读路径**：Redis `article:info:{id}` → 命中直接返回 JSON → 未命中查 MySQL → 原子递增 `article:view:{id}` → 更新 Sorted Set `hot:articles:weekly`（score=浏览量）→ 回写 Redis 缓存（TTL 3600s）
- **写路径**（评论）：MySQL 写入 → **删除** `comment:list:{article_id}` 缓存（而非更新，避免并发写丢失）
- **同步路径**：`steady_timer` 每 5 分钟触发 `syncViewCounts()` → `ZREVRANGE` 取 Top 100 → 逐个获取 Redis 计数器 → `UPDATE articles SET view_count = ?` 回写 MySQL
- **数据合并**：显示时取 `max(MySQL.viewCount, Redis.atomicCount)`，保证界面值永不回退

### 4.3 跨平台兼容（Windows / Linux / macOS）

**难点**：四种编译器（MSVC、MinGW GCC、Linux GCC、Clang）的行为差异，以及系统 API 差异（`localtime_s` vs `localtime_r`、`getenv` vs `_dupenv_s`、信号处理、Winsock 链接）。

**解决方案**：
- 编写 `Platform.h` 兼容层：`safeLocaltime()` 统一 `localtime_s/localtime_r` 参数顺序差异，`safeGetenv()` 在 Windows 上使用线程安全的 `_dupenv_s` 分配独立缓冲区
- CMakeLists.txt 按编译器设置编译参数（MSVC 的 `/utf-8`、Clang 的 `-Wpedantic`、GCC 的 `-D_FORTIFY_SOURCE=2`），按平台设置链接库（Windows 的 `ws2_32 wsock32`）
- Asio 编译期抽象：`ASIO_STANDALONE` 宏切换 `boost::asio` 和独立 `asio` 命名空间

### 4.4 离线环境下的依赖管理

**难点**：开发环境可能无网络访问 GitHub，`FetchContent` 无法 `git clone` 第三方库。

**解决方案**：
- CMake 设计**四级回退链**：`backend/deps/` 预下载目录 → 用户 `-DXXX_DIR=` 指定 → `find_package` 系统安装 → `FetchContent`（需显式 `-DFETCH_DEPS=ON`）
- 同级目录放置 `deps/README.md` 说明手动放置路径和下载链接
- 提供 `download_deps.ps1` / `download_deps.sh` 自动化脚本：GitHub 失败自动尝试 Gitee 镜像

### 4.5 Mock 降级模式

**难点**：前端开发联调时可能无 MySQL/Redis 环境。

**解决方案**：编译期宏控制（`MOCK_DB=1`、`MOCK_CACHE=1`），CMake 找不到系统库时自动启用。Mock 模式使用 `static` 内存容器模拟数据库（5 篇文章、3 条评论）和 Redis（`unordered_map<string, string>`），所有 API 正常工作，前端完全无感知。

### 4.6 HTTP 协议解析的健壮性

**难点**：HTTP 请求可能分片到达（TCP 流式传输），解析器需要处理半包和粘包。

**解决方案**：
- `HttpParser::parse()` 返回 `bool` 表示完整与否：不完整时返回 `false`，`HttpSession` 继续 `async_read_some` 追加数据后重试
- 通过搜索 `\r\n\r\n` 定位 Header 结束位置，再通过 `Content-Length` Header 精确读取 Body
- 独立应用层 `Buffer` 类管理已读/未读/可写三个指针区域，避免频繁内存分配

---

## 五、项目成果与核心工作内容

### 核心产出

| 产出 | 说明 |
|------|------|
| **C++ HTTP 服务器** | 自研 Reactor 多线程服务器，3000+ 行，6 个 RESTful API 端点 |
| **Vue 3 SPA 前端** | 3 个页面 + 1 个复用组件，TypeScript 严格模式，1200+ 行 |
| **连接池系统** | MySQL（8 连接）+ Redis（4 连接）双连接池，RAII 管理生命周期 |
| **缓存系统** | Cache-Aside 读缓存 + 写失效 + 热度 Sorted Set + 5 分钟定时同步 |
| **Docker 部署** | 4 容器编排，多阶段构建，健康检查，一键启动 |
| **跨平台构建** | 4 编译器支持，四级依赖回退，离线构建方案 |
| **文档体系** | README / API 文档 / 部署步骤 / 离线依赖指引 / 项目简历 |

### 技术指标（设计目标）

| 指标 | 数值 |
|------|------|
| 最大并发连接 | 10,000（C10K） |
| 缓存命中 P99 延迟 | < 10ms |
| 缓存命中 QPS（预估） | > 50,000 |
| IO 线程数 | 4（可配置） |
| 文章缓存 TTL | 3600s |
| 评论缓存 TTL | 1800s |
| 热度数据同步间隔 | 300s |
| 单请求内存分配 | < 5 次 |

### 关键能力体现

- **系统编程**：C++17 多线程、Asio 异步 I/O、TCP/HTTP 协议实现、RAII 资源管理
- **后端架构**：Reactor 模式、连接池设计、缓存策略（Cache-Aside / Write-Invalidate）、定时任务调度
- **数据库**：MySQL 表设计、索引优化、参数化查询、SQL 注入防护
- **工程化**：CMake 构建系统设计、跨平台兼容、Docker 多阶段构建、Nginx 反向代理
- **前端**：Vue 3 Composition API、TypeScript 类型设计、骨架屏加载态、代码分割
- **安全**：SQL 注入修复、异步信号安全、CORS 配置、输入校验

---

## 六、项目架构图

```
┌─────────────────────────────────────────────────────────┐
│                     Browser (User)                      │
└─────────────────────┬───────────────────────────────────┘
                      │ HTTP
                      ▼
┌─────────────────────────────────────────────────────────┐
│              Nginx (port 80)                            │
│  /          → 静态文件 (Vue SPA dist)                   │
│  /api/*     → proxy_pass backend:8080                  │
│  /assets/*  → 7天缓存 (Cache-Control: immutable)       │
└──────────────┬──────────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────────────┐
│         C++ Backend (port 8080)                         │
│  ┌────────────────────────────────────────────────────┐ │
│  │  HttpServer (acceptor + thread pool)               │ │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐          │ │
│  │  │ IO Thread│ │ IO Thread│ │ IO Thread│ ... x N  │ │
│  │  │ io_ctx   │ │ io_ctx   │ │ io_ctx   │          │ │
│  │  │ .run()   │ │ .run()   │ │ .run()   │          │ │
│  │  └──────────┘ └──────────┘ └──────────┘          │ │
│  └────────────────────────────────────────────────────┘ │
│                        │                                │
│  ┌─────────────────────┼──────────────────────────────┐ │
│  │                 ApiHandler                         │ │
│  │  /api/health           /api/articles               │ │
│  │  /api/articles/:id     /api/articles/hot           │ │
│  │  /api/articles/:id/comments (GET/POST)             │ │
│  └──────────┬──────────────────┬──────────────────────┘ │
│             │                  │                         │
│  ┌──────────▼────────┐  ┌─────▼────────────────────┐   │
│  │  ArticleService   │  │  CommentService          │   │
│  │  • listArticles   │  │  • getComments           │   │
│  │  • getArticleDetail│  │  • createComment         │   │
│  │  • getHotArticles  │  │  • cache invalidation     │   │
│  │  • syncViewCounts  │  │                          │   │
│  └──────┬───┬─────────┘  └──────┬───┬───────────────┘   │
│         │   │                   │   │                    │
│  ┌──────▼┐ ┌▼──────────┐  ┌───▼┐ ┌▼──────────┐        │
│  │Database│ │Cache      │  │DB  │ │Cache      │        │
│  │Proxy   │ │Proxy      │  │Proxy│ │Proxy      │        │
│  │(MySQL) │ │(Redis)    │  │     │ │           │        │
│  └───────┘ └───────────┘  └─────┘ └───────────┘        │
└──────────────────────────────────────────────────────────┘
               │                  │
               ▼                  ▼
        ┌──────────┐      ┌──────────────┐
        │ MySQL 8.0│      │  Redis 7.0   │
        │ :3306    │      │  :6379       │
        │ articles │      │  String/List │
        │ comments │      │  /Sorted Set │
        └──────────┘      └──────────────┘
```

---

## 七、API 接口一览

| 方法 | 路径 | 功能 | 参数 |
|------|------|------|------|
| GET | `/api/health` | 健康检查 | — |
| GET | `/api/articles` | 文章列表（分页） | `page`(默认1), `pageSize`(默认10) |
| GET | `/api/articles/:id` | 文章详情（含浏览量） | 路径参数 `id` |
| GET | `/api/articles/hot` | 热门排行 | `limit`(默认10，最大20) |
| GET | `/api/articles/:id/comments` | 文章评论列表 | `limit`(默认20，最大50) |
| POST | `/api/articles/:id/comments` | 发表评论 | Body: `content`(必填≤1000字), `commenter` |

---

## 八、快速开始

```bash
# Docker 一键部署（推荐）
cd docker
docker compose up -d --build
# 访问 http://localhost

# 本地开发
# 终端 1：启动后端（Mock 模式无需 MySQL/Redis）
cd backend
./build.sh        # Linux/macOS
build.bat         # Windows
./build/lightning_server

# 终端 2：启动前端
cd frontend
npm install && npm run dev
# 访问 http://localhost:5173
```

---

> 📎 完整项目文档：[README.md](README.md) | [API 文档](docs/API.md) | [部署步骤](部署步骤.md)
