export interface Article {
  id: number
  title: string
  content?: string
  author: string
  createTime: string
  updateTime?: string
  viewCount: number
}

export interface Comment {
  id: number
  articleId: number
  content: string
  commenter: string
  createTime: string
}

export interface ApiResponse<T> {
  code: number
  message?: string
  data: T
}

export interface PaginatedData {
  list: Article[]
  total: number
  page: number
  pageSize: number
}

export interface CommentListData {
  list: Comment[]
  total: number
}
