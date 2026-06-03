CREATE DATABASE IF NOT EXISTS lightning_db DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE lightning_db;

CREATE TABLE IF NOT EXISTS articles (
    id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(200) NOT NULL,
    content TEXT NOT NULL,
    author VARCHAR(50) NOT NULL,
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    update_time DATETIME ON UPDATE CURRENT_TIMESTAMP,
    view_count INT DEFAULT 0,
    INDEX idx_create_time (create_time)
);

CREATE TABLE IF NOT EXISTS comments (
    id INT AUTO_INCREMENT PRIMARY KEY,
    article_id INT NOT NULL,
    content TEXT NOT NULL,
    commenter VARCHAR(50) NOT NULL,
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (article_id) REFERENCES articles(id) ON DELETE CASCADE,
    INDEX idx_article_id (article_id)
);

INSERT INTO articles (title, content, author, view_count) VALUES
('高并发服务器架构设计', 'Reactor 模式是高性能网络编程的核心设计模式。本文深入探讨 One Loop Per Thread 模型，分析 epoll 事件驱动机制，以及如何通过线程池实现 C10K 并发连接。

核心要点：
1. 主 Reactor 负责 accept 新连接
2. 子 Reactor 负责 IO 读写
3. 业务逻辑在线程池中异步执行
4. 连接池化减少资源开销', '张三', 1280),

('Redis 缓存最佳实践', 'Cache-Aside 模式是 Web 应用中最常用的缓存策略。读操作先查缓存，未命中则查数据库并回写缓存；写操作先更新数据库，再删除缓存。

本文介绍：
- 缓存键命名规范
- TTL 设置策略
- 缓存击穿/雪崩/穿透的应对方案
- Pipeline 批量操作优化', '李四', 856),

('Vue 3 组合式 API 入门', 'Vue 3 的 Composition API 提供了更灵活的逻辑复用方式。相比 Options API，组合式 API 更适合大型项目的代码组织。

主要内容：
- setup() 函数与 ref/reactive
- computed 与 watch
- 自定义 composable
- 与 TypeScript 的类型推导', '王五', 642),

('MySQL 连接池优化指南', '数据库连接池是后端性能的关键环节。合理的连接池大小、超时设置和慢查询优化可以显著提升系统吞吐量。

优化方向：
- 连接池大小 = CPU核数 * 2 + 磁盘数
- 连接获取超时与快速失败
- 预编译语句防 SQL 注入
- 读写分离架构设计', '赵六', 423),

('epoll 与 select 性能对比', 'Linux 下 I/O 多路复用有三种实现：select、poll 和 epoll。在高并发场景下，epoll 的性能优势非常明显。

对比分析：
- select: O(n) 遍历，fd 数量限制 1024
- poll: O(n) 遍历，无 fd 数量限制
- epoll: O(1) 事件通知，边缘触发模式
- 实际压测数据对比', '张三', 1105);

INSERT INTO comments (article_id, content, commenter) VALUES
(1, '写得很详细，Reactor 模式讲解清晰！', '读者A'),
(1, 'One Loop Per Thread 的设计思路很有启发', '读者B'),
(2, 'Cache-Aside 模式在实际项目中确实好用', '读者C'),
(3, 'Composition API 比 Options API 灵活多了', '前端开发者'),
(5, 'epoll 边缘触发模式需要注意读写完整性', '系统程序员');
