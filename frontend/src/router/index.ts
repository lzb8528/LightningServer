import { createRouter, createWebHistory } from 'vue-router'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      name: 'ArticleList',
      component: () => import('@/views/ArticleList.vue'),
    },
    {
      path: '/articles/:id',
      name: 'ArticleDetail',
      component: () => import('@/views/ArticleDetail.vue'),
    },
    {
      path: '/hot',
      name: 'HotArticles',
      component: () => import('@/views/HotArticles.vue'),
    },
  ],
})

export default router
