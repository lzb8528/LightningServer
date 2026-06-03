<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { getArticles } from '@/api'
import type { Article } from '@/types'

const router = useRouter()
const articles = ref<Article[]>([])
const loading = ref(true)
const total = ref(0)
const page = ref(1)
const pageSize = ref(10)

async function fetchArticles() {
  loading.value = true
  try {
    const res = await getArticles(page.value, pageSize.value)
    if (res.code === 0) {
      articles.value = res.data.list
      total.value = res.data.total
    }
  } catch (e: any) {
    ElMessage.error(e.message || '加载文章失败')
  } finally {
    loading.value = false
  }
}

function handlePageChange(p: number) {
  page.value = p
  fetchArticles()
}

function goDetail(id: number) {
  router.push(`/articles/${id}`)
}

onMounted(fetchArticles)
</script>

<template>
  <div class="article-list">
    <h1 class="page-title">文章列表</h1>

    <div v-if="loading" class="skeleton-list">
      <el-skeleton v-for="i in 5" :key="i" :rows="3" animated class="skeleton-item" />
    </div>

    <template v-else>
      <el-empty v-if="articles.length === 0" description="暂无文章" />

      <div v-else class="articles">
        <el-card
          v-for="article in articles"
          :key="article.id"
          class="article-card"
          shadow="hover"
          @click="goDetail(article.id)"
        >
          <h2 class="article-title">{{ article.title }}</h2>
          <div class="article-meta">
            <span>{{ article.author }}</span>
            <span>{{ article.createTime }}</span>
            <span>{{ article.viewCount }} 次浏览</span>
          </div>
        </el-card>
      </div>

      <div v-if="total > pageSize" class="pagination">
        <el-pagination
          v-model:current-page="page"
          :page-size="pageSize"
          :total="total"
          layout="prev, pager, next"
          @current-change="handlePageChange"
        />
      </div>
    </template>
  </div>
</template>

<style scoped>
.page-title {
  font-size: 24px;
  margin-bottom: 20px;
  color: #303133;
}

.skeleton-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.skeleton-item {
  padding: 16px;
  background: white;
  border-radius: 8px;
}

.articles {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.article-card {
  cursor: pointer;
  transition: transform 0.2s;
}

.article-card:hover {
  transform: translateY(-2px);
}

.article-title {
  font-size: 18px;
  margin-bottom: 8px;
  color: #303133;
}

.article-meta {
  display: flex;
  gap: 16px;
  font-size: 13px;
  color: #909399;
}

.pagination {
  display: flex;
  justify-content: center;
  margin-top: 24px;
}
</style>
