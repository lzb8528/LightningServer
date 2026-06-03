<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { getComments, createComment } from '@/api'
import type { Comment } from '@/types'

const props = defineProps<{ articleId: number }>()

const comments = ref<Comment[]>([])
const total = ref(0)
const loading = ref(true)
const submitting = ref(false)
const newComment = ref('')
const commenter = ref('')

async function fetchComments() {
  loading.value = true
  try {
    const res = await getComments(props.articleId)
    if (res.code === 0) {
      comments.value = res.data.list
      total.value = res.data.total
    }
  } catch (e: any) {
    ElMessage.error(e.message || '加载评论失败')
  } finally {
    loading.value = false
  }
}

async function handleSubmit() {
  if (!newComment.value.trim()) {
    ElMessage.warning('请输入评论内容')
    return
  }

  submitting.value = true
  try {
    const res = await createComment(
      props.articleId,
      newComment.value.trim(),
      commenter.value.trim() || '匿名用户'
    )
    if (res.code === 0) {
      ElMessage.success('评论发表成功')
      newComment.value = ''
      await fetchComments()
    } else {
      ElMessage.error(res.message || '发表评论失败')
    }
  } catch (e: any) {
    ElMessage.error(e.message || '发表评论失败')
  } finally {
    submitting.value = false
  }
}

onMounted(fetchComments)
</script>

<template>
  <div class="comment-section">
    <h2>评论 ({{ total }})</h2>

    <div class="comment-form">
      <el-input
        v-model="commenter"
        placeholder="昵称（可选）"
        style="margin-bottom: 8px"
        maxlength="50"
      />
      <el-input
        v-model="newComment"
        type="textarea"
        :rows="3"
        placeholder="写下你的评论..."
        maxlength="1000"
        show-word-limit
      />
      <el-button
        type="primary"
        :loading="submitting"
        style="margin-top: 8px"
        @click="handleSubmit"
      >
        发表评论
      </el-button>
    </div>

    <el-skeleton v-if="loading" :rows="4" animated />

    <el-empty v-else-if="comments.length === 0" description="暂无评论，来抢沙发吧" />

    <div v-else class="comment-list">
      <div v-for="comment in comments" :key="comment.id" class="comment-item">
        <div class="comment-header">
          <strong>{{ comment.commenter }}</strong>
          <span class="time">{{ comment.createTime }}</span>
        </div>
        <p class="comment-content">{{ comment.content }}</p>
      </div>
    </div>
  </div>
</template>

<style scoped>
.comment-section {
  background: white;
  border-radius: 8px;
  padding: 24px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.comment-section h2 {
  font-size: 18px;
  margin-bottom: 16px;
}

.comment-form {
  margin-bottom: 24px;
  padding-bottom: 24px;
  border-bottom: 1px solid #ebeef5;
}

.comment-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.comment-item {
  padding: 12px 0;
  border-bottom: 1px solid #f5f5f5;
}

.comment-header {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 8px;
}

.comment-header strong {
  color: #303133;
}

.time {
  font-size: 12px;
  color: #c0c4cc;
}

.comment-content {
  font-size: 14px;
  line-height: 1.6;
  color: #606266;
}
</style>
