<script setup lang="ts">
import { RouterView } from 'vue-router'
</script>

<template>
  <div id="app-container">
    <header class="app-header">
      <div class="header-inner">
        <router-link to="/" class="logo">
          <span class="logo-icon">⚡</span>
          LightningServer
        </router-link>
        <nav>
          <router-link to="/">文章列表</router-link>
          <router-link to="/hot">热门排行</router-link>
        </nav>
      </div>
    </header>
    <main class="app-main">
      <RouterView />
    </main>
    <footer class="app-footer">
      LightningServer 高并发全栈 Web 应用
    </footer>
  </div>
</template>

<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background: #f5f7fa;
  color: #303133;
}

#app-container {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.app-header {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 0 24px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
}

.header-inner {
  max-width: 960px;
  margin: 0 auto;
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 60px;
}

.logo {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 20px;
  font-weight: 700;
  color: white;
  text-decoration: none;
}

.logo-icon {
  font-size: 24px;
}

nav {
  display: flex;
  gap: 24px;
}

nav a {
  color: rgba(255, 255, 255, 0.85);
  text-decoration: none;
  font-size: 15px;
  transition: color 0.2s;
}

nav a:hover,
nav a.router-link-active {
  color: white;
}

.app-main {
  flex: 1;
  max-width: 960px;
  width: 100%;
  margin: 0 auto;
  padding: 24px 16px;
}

.app-footer {
  text-align: center;
  padding: 16px;
  color: #909399;
  font-size: 13px;
  border-top: 1px solid #ebeef5;
}
</style>
