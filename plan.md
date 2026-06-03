LightningServer 全栈高并发项目开发文档
版本： v1.0
状态： 计划中
最后更新： 2026-06-02
1. 项目概述
1.1 项目名称
LightningServer —— 基于 C++ 的高并发全栈 Web 应用
1.2 项目简介
在 C++ 高并发服务器框架基础上，向前接入现代前端单页应用，向后整合 MySQL 持久化存储与 Redis 缓存中间件，构建一个承载高并发读写的实战型全栈 Web 应用。
业务场景： 设定为"实时热门文章讨论区"，支持文章列表、详情查看、评论互动，并通过缓存应对突发流量。
整体架构： 采用 Reactor 多线程模型 + epoll，C++ 负责核心业务逻辑与协议解析，前端采用 Vue 3 + Element Plus 构建动态交互界面，数据层通过 MySQL 存储结构化数据，Redis 提供缓存加速与排行榜功能。
1.3 核心目标
•	实现完整的 C/S 全栈架构，贯通 网络层 → 业务层 → 数据层 → 前端展示
•	C++ 服务器支撑 C10K 并发连接，后端 API 响应时间 P99 < 10ms（缓存命中场景）
•	前端异步加载、乐观更新，用户体验流畅
•	数据库读写分离思想（单机模拟），通过连接池提升数据库交互性能
•	Redis 实现热点文章缓存、浏览计数、评论临时队列，降低 MySQL 压力
•	掌握全栈调试、性能剖析、端到端压测方法
2. 技术选型总览
层次	技术/工具	说明
后端核心	C++17, epoll, Reactor, 线程池	高性能事件驱动，One Loop Per Thread
构建系统	CMake 3.20+	管理 C++ 依赖与编译
前端框架	Vue 3 + Vite + TypeScript + Element Plus	组件化开发，响应式，动态路由
Web 服务器	Nginx（反向代理 + 静态资源分离）	前端静态文件与后端 API 分流
数据库	MySQL 8.0	持久化文章、用户、评论数据
缓存中间件	Redis 7.0	热点数据缓存、计数、简单消息队列
数据库连接	mysql-connector-c++ / libmysqlclient	连接池化，同步调用在线程池执行
Redis 客户端	hiredis + 自封装异步适配	支持 pipeline，连接复用
日志	spdlog	异步落地，分级日志
测试工具	wrk, ab, Redis-benchmark, Jest	分层压力测试
版本控制	Git + GitHub	
3. 系统架构
3.1 整体架构图
 
3.2 交互流程
•	用户浏览器请求 https://domain.com，Nginx 返回前端 SPA 的 index.html。
•	前端异步调用 /api/articles?page=1，Nginx 反代至 C++ 服务器。
•	C++ 服务器查询 Redis 缓存，命中则直接返回 JSON；未命中则从 MySQL 加载，回写 Redis 并返回。
•	用户提交评论，前端 POST /api/comments，C++ 服务器校验后写入 MySQL 并异步更新 Redis 评论计数。
•	热门文章排行榜由 C++ 定时任务（时间轮）从 Redis 有序集合中读取并更新缓存。
3.3 C++ 服务器内部模块
•	DatabaseProxy： 封装 MySQL 连接池与 SQL 执行。
•	CacheProxy： 封装 Redis 连接池，提供 get/set/incr/zadd 等常用操作。
•	ArticleService / CommentService： 业务逻辑层。
•	JsonHelper： 使用 nlohmann/json 库进行 JSON 解析与生成。
•	基础框架模块： EventLoop, ThreadPool, TcpConnection, Buffer, HttpParser, Timer 等。
4. 功能模块详解
模块	功能描述	关键技术点
前端页面	文章列表、详情、评论组件，骨架屏，错误处理	Vue Router, Axios, 组件通信
Nginx 网关	静态资源服务、API 反向代理、gzip 压缩、缓存头设置	前端部署优化
HTTP API 服务	RESTful 接口：文章列表/详情，评论提交/查询，排行榜	状态码规范，参数校验，防 SQL 注入
MySQL 集成	连接池（8-16 连接），参数化查询，事务支持	mysql-connector-c++ 或 SOCI 库
Redis 集成	连接池，缓存策略（Cache-Aside），浏览量原子递增	hiredis 异步，pipeline 批量查询
连接与线程管理	每个工作线程独立 eventloop + 数据库连接	One Loop Per Thread
定时器	定时更新排行榜、清理过期缓存、会话超时	小顶堆或时间轮
优雅关闭	信号处理，依次关闭监听、等待队列排空、释放 DB/缓存连接	
5. 数据库设计
5.1 MySQL 表结构
CREATE TABLE articles (
    id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(200) NOT NULL,
    content TEXT NOT NULL,
    author VARCHAR(50) NOT NULL,
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    update_time DATETIME ON UPDATE CURRENT_TIMESTAMP,
    view_count INT DEFAULT 0,
    INDEX idx_create_time (create_time)
);

CREATE TABLE comments (
    id INT AUTO_INCREMENT PRIMARY KEY,
    article_id INT NOT NULL,
    content TEXT NOT NULL,
    commenter VARCHAR(50) NOT NULL,
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (article_id) REFERENCES articles(id),
    INDEX idx_article_id (article_id)
);
5.2 Redis 键设计
键名	类型	说明
article:info:{id}	Hash	文章基本信息缓存
article:view:{id}	String	实时浏览量
hot:articles:weekly	Sorted Set	周热门文章排行榜
comment:list:{article_id}	List	最新 N 条评论缓存（可选）
6. 开发阶段与任务拆分
阶段一：C++ 高并发框架与静态服务器（3 天）
•	实现 TCP 回显/静态文件服务器（单线程 → 多线程 Reactor）
•	支持 HTTP/1.1 GET，返回本地静态文件
阶段二：前端工程搭建与 API 联调基础（2 天）
•	创建 Vue 项目，配置 Vite、Axios、Element Plus
•	设计文章列表页、详情页、评论组件（使用 Mock 数据）
•	编写 API 接口文档（RESTful 规范）
•	后端提供硬编码 JSON 响应，实现前后端分离联调
阶段三：数据库与缓存集成（4 天）
•	后端集成 MySQL 连接池，实现文章、评论的 CRUD
•	实现参数化查询，防止 SQL 注入
•	后端集成 Redis 连接池，实现缓存读/写/失效策略
•	文章详情 API 增加缓存逻辑（Cache-Aside）
•	文章浏览量通过 Redis 原子操作递增，定期回写 MySQL
阶段四：业务功能完善（3 天）
•	实现评论提交 API，写入 MySQL 并维护 Redis 缓存最新评论
•	实现热门文章排行榜定时更新
•	前端对接所有真实 API，处理加载状态与错误
•	完善前端 UI 交互（分页、下拉刷新等）
阶段五：性能测试与优化（3 天）
•	使用 wrk 对核心 API 进行压测
•	测试缓存命中率、数据库 QPS、Redis 性能
•	优化连接池参数、线程数，使用 perf 分析后端热点
•	前端开启代码分割、懒加载，减少首屏时间
•	处理缓存击穿、缓存雪崩等典型问题
阶段六：部署与文档（2 天）
•	编写 Docker Compose 一键部署（Nginx + C++ 服务 + MySQL + Redis）
•	完善 README、API 文档、性能测试报告
•	录制演示视频或截图
总预计时间： 约 17 天（可根据实际情况调整）
7. 测试策略
测试类型	工具	重点验证
单元测试	Google Test (C++), Jest (前端)	Buffer, HttpParser, Timer, Vue 组件渲染
API 功能测试	Postman / curl	接口正确性、参数校验、状态码
端到端测试	Cypress	用户操作流（浏览文章→发表评论→查看结果）
压力测试	wrk, Redis-benchmark	后端吞吐量、缓存集群吞吐、数据库连接池承受能力
稳定性测试	长跑 12 小时 + 随机流量	内存泄漏、连接泄漏、日志文件大小
前端性能	Lighthouse	FCP, LCP, CLS 等指标
8. 风险与应对
风险	应对措施
缓存与数据库数据不一致	Cache Aside 模式 + 过期时间兜底；关键数据先更新数据库再删缓存
Redis 内存溢出	设置 maxmemory，配置 LRU 淘汰策略；监控键数量
数据库连接池耗尽	限制最大连接数，设置获取连接超时；快速失败返回 503，前端提示稍后重试
前端跨域问题	Nginx 反向代理统一域名，或后端设置 CORS 头（开发环境）
C++ 与前端接口联调效率低	先约定 API 文档，后端使用 Postman 模拟测试，前端用 Mock 服务
9. 项目交付物
•	源代码仓库： 后端 C++、前端 Vue、Nginx 配置、数据库初始化脚本
•	Docker Compose 编排： 一键启动所有服务
•	API 文档： 使用 Markdown 描述所有 REST 接口
•	性能测试报告： 包含不同并发下的 QPS、P99 延迟、缓存命中率等
•	部署指南： 环境要求、编译步骤、参数调优建议
•	开发日志（可选）
10. 后续扩展方向
•	引入消息队列（RabbitMQ）处理异步任务，如邮件通知
•	增加 WebSocket 支持，实现评论实时推送
•	将 C++ 业务逻辑替换为协程（C++20）进一步简化异步代码
•	微服务化拆分文章服务、评论服务、用户服务（+ 用户认证模块）
•	前端 SSR（服务端渲染）提升 SEO
本文档基于初始计划迭代生成，作为 LightningServer 全栈高并发项目的正式开发参考，后续可根据实际推进持续修订。
(AI生成)
