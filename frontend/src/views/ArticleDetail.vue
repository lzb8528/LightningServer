<script setup lang="ts">
import { ref, onMounted, computed } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage } from 'element-plus'
import { getArticleDetail } from '@/api'
import type { Article } from '@/types'
import CommentSection from '@/components/CommentSection.vue'

const route = useRoute()
const article = ref<Article | null>(null)
const loading = ref(true)

const articleId = computed(() => Number(route.params.id))

async function fetchArticle() {
  loading.value = true
  try {
    const res = await getArticleDetail(articleId.value)
    if (res.code === 0) {
      article.value = res.data
    } else {
      ElMessage.error(res.message || '文章不存在')
    }
  } catch (e: any) {
    ElMessage.error(e.message || '加载文章失败')
  } finally {
    loading.value = false
  }
}

onMounted(fetchArticle)
</script>

<template>
  <div class="article-detail">
    <el-skeleton v-if="loading" :rows="8" animated />

    <template v-else-if="article">
      <el-page-header @back="$router.push('/')" title="返回列表" />

      <article class="content">
        <h1>{{ article.title }}</h1>
        <div class="meta">
          <el-tag size="small">{{ article.author }}</el-tag>
          <span>{{ article.createTime }}</span>
          <span>{{ article.viewCount }} 次浏览</span>
        </div>
        <div class="body">{{ article.content }}</div>
      </article>

      <CommentSection :article-id="articleId" />
    </template>

    <el-empty v-else description="文章不存在" />
  </div>
</template>

<style scoped>
.content {
  background: white;
  border-radius: 8px;
  padding: 24px;
  margin: 16px 0;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.content h1 {
  font-size: 28px;
  margin-bottom: 16px;
  line-height: 1.4;
}

.meta {
  display: flex;
  align-items: center;
  gap: 16px;
  margin-bottom: 24px;
  font-size: 14px;
  color: #909399;
}

.body {
  font-size: 16px;
  line-height: 1.8;
  color: #606266;
  white-space: pre-wrap;
}
</style>
