<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { getHotArticles } from '@/api'
import type { Article } from '@/types'

const router = useRouter()
const articles = ref<Article[]>([])
const loading = ref(true)

async function fetchHot() {
  loading.value = true
  try {
    const res = await getHotArticles(10)
    if (res.code === 0) {
      articles.value = res.data
    }
  } catch (e: any) {
    ElMessage.error(e.message || '加载排行榜失败')
  } finally {
    loading.value = false
  }
}

function goDetail(id: number) {
  router.push(`/articles/${id}`)
}

onMounted(fetchHot)
</script>

<template>
  <div class="hot-articles">
    <h1 class="page-title">🔥 热门文章排行</h1>

    <el-skeleton v-if="loading" :rows="10" animated />

    <el-empty v-else-if="articles.length === 0" description="暂无热门文章" />

    <div v-else class="rank-list">
      <div
        v-for="(article, index) in articles"
        :key="article.id"
        class="rank-item"
        @click="goDetail(article.id)"
      >
        <span class="rank" :class="{ top3: index < 3 }">{{ index + 1 }}</span>
        <div class="info">
          <h3>{{ article.title }}</h3>
          <div class="meta">
            <span>{{ article.author }}</span>
            <span>{{ article.viewCount }} 次浏览</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.page-title {
  font-size: 24px;
  margin-bottom: 20px;
}

.rank-list {
  background: white;
  border-radius: 8px;
  overflow: hidden;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.rank-item {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 16px 20px;
  cursor: pointer;
  border-bottom: 1px solid #f0f0f0;
  transition: background 0.2s;
}

.rank-item:last-child {
  border-bottom: none;
}

.rank-item:hover {
  background: #f5f7fa;
}

.rank {
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  background: #ebeef5;
  font-weight: 700;
  font-size: 14px;
  color: #909399;
  flex-shrink: 0;
}

.rank.top3 {
  background: linear-gradient(135deg, #667eea, #764ba2);
  color: white;
}

.info h3 {
  font-size: 16px;
  margin-bottom: 4px;
}

.meta {
  display: flex;
  gap: 12px;
  font-size: 13px;
  color: #909399;
}
</style>
