import axios from 'axios'
import type { ApiResponse, PaginatedData, Article, CommentListData, Comment } from '@/types'

const api = axios.create({
  baseURL: '/api',
  timeout: 10000,
})

api.interceptors.response.use(
  (response) => response.data,
  (error) => {
    const message = error.response?.data?.message || error.message || '请求失败'
    return Promise.reject(new Error(message))
  }
)

export function getArticles(page = 1, pageSize = 10) {
  return api.get<any, ApiResponse<PaginatedData>>('/articles', {
    params: { page, pageSize },
  })
}

export function getArticleDetail(id: number) {
  return api.get<any, ApiResponse<Article>>(`/articles/${id}`)
}

export function getHotArticles(limit = 10) {
  return api.get<any, ApiResponse<Article[]>>('/articles/hot', {
    params: { limit },
  })
}

export function getComments(articleId: number, limit = 20) {
  return api.get<any, ApiResponse<CommentListData>>(`/articles/${articleId}/comments`, {
    params: { limit },
  })
}

export function createComment(articleId: number, content: string, commenter: string) {
  return api.post<any, ApiResponse<Comment>>(`/articles/${articleId}/comments`, {
    content,
    commenter,
  })
}

export function healthCheck() {
  return api.get('/health')
}
