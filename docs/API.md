# LightningServer API 文档

版本：v1.0  
基础路径：`/api`  
数据格式：JSON

## 通用响应格式

```json
{
  "code": 0,
  "message": "optional message",
  "data": {}
}
```

| code | 说明 |
|------|------|
| 0    | 成功 |
| 400  | 请求参数错误 |
| 404  | 资源不存在 |
| 500  | 服务器内部错误 |

---

## 1. 健康检查

**GET** `/api/health`

响应示例：
```json
{"code": 0, "message": "ok"}
```

---

## 2. 文章列表

**GET** `/api/articles`

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| page | int | 否 | 1 | 页码 |
| pageSize | int | 否 | 10 | 每页数量（最大50） |

响应示例：
```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "id": 1,
        "title": "高并发服务器架构设计",
        "author": "张三",
        "createTime": "2026-06-01 10:00:00",
        "updateTime": "2026-06-01 10:00:00",
        "viewCount": 1280
      }
    ],
    "total": 5,
    "page": 1,
    "pageSize": 10
  }
}
```

---

## 3. 文章详情

**GET** `/api/articles/{id}`

路径参数：
- `id` — 文章 ID

响应示例：
```json
{
  "code": 0,
  "data": {
    "id": 1,
    "title": "高并发服务器架构设计",
    "content": "Reactor 模式是...",
    "author": "张三",
    "createTime": "2026-06-01 10:00:00",
    "updateTime": "2026-06-01 10:00:00",
    "viewCount": 1281
  }
}
```

> 每次访问详情会自动递增 Redis 浏览计数，并更新热门排行榜。

---

## 4. 热门文章排行

**GET** `/api/articles/hot`

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| limit | int | 否 | 10 | 返回数量（最大20） |

响应示例：
```json
{
  "code": 0,
  "data": [
    {
      "id": 5,
      "title": "epoll 与 select 性能对比",
      "author": "张三",
      "createTime": "2026-06-02 15:30:00",
      "viewCount": 1105
    }
  ]
}
```

---

## 5. 获取评论

**GET** `/api/articles/{articleId}/comments`

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| limit | int | 否 | 20 | 返回数量（最大50） |

响应示例：
```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "id": 1,
        "articleId": 1,
        "content": "写得很详细！",
        "commenter": "读者A",
        "createTime": "2026-06-01 12:00:00"
      }
    ],
    "total": 2
  }
}
```

---

## 6. 发表评论

**POST** `/api/articles/{articleId}/comments`

请求体：
```json
{
  "content": "评论内容",
  "commenter": "昵称"
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| content | string | 是 | 评论内容（最大1000字符） |
| commenter | string | 否 | 昵称，默认"匿名用户" |

响应示例（201 Created）：
```json
{
  "code": 0,
  "message": "Comment created",
  "data": {
    "id": 0,
    "articleId": 1,
    "content": "评论内容",
    "commenter": "昵称",
    "createTime": "2026-06-03 10:00:00"
  }
}
```

---

## Redis 键设计

| 键名 | 类型 | 说明 |
|------|------|------|
| `article:info:{id}` | String (JSON) | 文章详情缓存，TTL 3600s |
| `article:view:{id}` | String (计数) | 实时浏览量 |
| `hot:articles:weekly` | Sorted Set | 周热门文章排行榜 |
| `comment:list:{article_id}` | List | 最新评论缓存 |

---

## 压测示例

```bash
# 文章列表
wrk -t4 -c100 -d30s http://localhost:8080/api/articles

# 文章详情（缓存命中）
wrk -t4 -c100 -d30s http://localhost:8080/api/articles/1

# 发表评论
wrk -t2 -c20 -d10s -s post_comment.lua http://localhost:8080/api/articles/1/comments
```
