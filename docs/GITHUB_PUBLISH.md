# 发布到 GitHub 指南

这份文档写给仓库维护者：如何把这个项目整理成 GitHub 仓库，并写清楚“软件是干嘛的 / 怎么用”。

---

## 1. 建议的仓库信息

### 仓库名

```text
deepdesign-studio
```

### 一句话描述（About）

中文：

```text
AI 原生 UI 交互设计软件：Qt6/C++ 画布、形状、渐变、标尺参考线、时间轴动效、交互预览，输出 DeepSeek 可读的 .ddesign 与 AI 交付包。
```

English：

```text
AI-native UI interaction design tool built with Qt6/C++: canvas, shapes, gradients, rulers/guides, timeline animations, interactive preview, and DeepSeek-readable .ddesign + AI handoff packages.
```

### Topics

```text
qt
qt6
cpp
ui-design
design-tool
canvas
animation
timeline
gradient
guides
ai
deepseek
llm
json
svg
html-export
prototype
```

### 建议的社交预览图

直接使用：

```text
screenshots/social-preview.png
```

这是为 GitHub Social Preview 准备的 1280×640 品牌图；也可以在 GitHub 仓库 Settings → Social preview 上传。  
如需自己替换，保留「渐变界面 + 时间轴」的构图即可。

---

## 2. 仓库里应该提交什么

### 建议提交

```text
DeepDesignStudio.pro
build.bat / run.bat / deploy.bat
src/
docs/
examples/
screenshots/
export/              （示例 HTML + AI 交付包，体积不大）
tools/
README.md
LICENSE
THIRD_PARTY_NOTICES.md
CONTRIBUTING.md
.gitignore
.gitattributes
```

### 不要提交

```text
build/               编译中间产物
dist/                已部署二进制（通过 Release 分发）
*.zip                通过 Release 附件分发
selftest-report.txt
selftest-artifacts/
```

`.gitignore` 已经忽略了这些。

> 二进制发布建议走 **GitHub Releases**，不要直接塞进 git 历史。  
> `DeepDesignStudio-win64.zip` 约 16MB，可以作为 Release 附件。

---

## 3. 首次上传命令

在项目根目录（包含 `README.md`）打开终端：

```bat
git init
git add .
git commit -m "feat: initial release of DeepDesign Studio 1.0"
git branch -M main
git remote add origin https://github.com/<your-name>/<repo>.git
git push -u origin main
```

如果你使用 GitHub Desktop：

1. Add Local Repository → 选择 `DeepDesignStudio`
2. 填写 commit message
3. Publish repository

### 3.1 使用 GitHub Desktop 上传（图形界面）

⚠️ **先确认你打开的是正确的仓库。**

如果你看到的是：

```text
Current repository: aster-x86
Current branch: ng
Current worktree: OS_Project
```

那说明 GitHub Desktop 当前打开的是 `D:\LinuxOS_Project` 下的旧仓库，**不是** DeepDesignStudio。  
此时点击 **Publish repository** 上传的是 `OS_Project`，不是本项目。

正确步骤：

1. GitHub Desktop 菜单 **File → Add Local Repository…**。
2. 选择：

```text
E:\ai软件\DeepDesignStudio
```

3. 如果提示不是 Git 仓库，点击 **create a repository**（GitHub Desktop 会初始化 `.git`，仓库里的 `.gitignore` 已准备好）。
4. 左侧 **Changes** 里应该能看到：
   - `src/`
   - `docs/`
   - `examples/`
   - `screenshots/`
   - `export/`
   - `README.md`、`LICENSE` 等
   - **不应该看到** `build/`、`dist/`、`*.zip`
5. 左下角 Summary 填：

```text
feat: initial release of DeepDesign Studio 1.0
```

6. 点 **Commit to main**（如果当前分支是 master，先 Branch → Rename 改成 main）。
7. 顶部点 **Publish repository**：
   - Name：`deepdesign-studio`
   - Description：粘贴前面的中文/英文 About 文案
   - 选择 Public / Private
   - 点 Publish repository
8. 发布后到 GitHub 网页：
   - 仓库 About 填描述和 Topics
   - Settings → Social preview 上传 `screenshots/social-preview.png`
   - Releases → Draft a new release → `v1.0.0` → 上传 `DeepDesignStudio-win64.zip`

> 如果 Changes 里出现 `build/`、`dist/`，说明你选错了目录或 `.gitignore` 没有生效，先不要提交，重新 Add Local Repository。

---

## 4. 创建 Release

1. 打开仓库 → Releases → Draft a new release
2. Tag：`v1.0.0`
3. Title：`灵绘 DeepDesign Studio 1.0`
4. 上传附件：

```text
DeepDesignStudio-win64.zip
```

5. Release notes 示例：

```markdown
## 灵绘 DeepDesign Studio 1.0

首个公开版本。

### 功能
- Qt6/C++ 画布设计：矩形/椭圆/直线/多边形/星形/自由路径/文字
- 选择、多选、框选、缩放、旋转、吸附、对齐、分布
- 图层、属性、颜色/渐变面板、调色板
- 标尺 + 横竖参考线（手动测量/对齐）
- 时间轴关键帧、缓动、循环、播放
- 点击/悬停/移出/载入交互
- F5 交互预览
- `.ddesign` 保存/读取
- HTML + design.json + UI_SPEC + AI_HANDOFF 导出
- CLI：--render / --export / --validate / --selftest

### 运行
下载 `DeepDesignStudio-win64.zip`，解压后双击 `run.bat`。

### 验证
- 构建：Qt 6.11.2 + MinGW 13.1，Release
- 自测：20 / 20
```

---

## 5. README 中的 Release 入口

README 的「方式一：直接运行 Windows 版本」没有硬编码仓库地址，而是让用户打开仓库右侧 **Releases** 下载。  
这样仓库改名/转移后不需要改 README。

如果你想在 README 中加一个固定入口，可以写：

```markdown
[下载最新版](https://github.com/<your-name>/<repo>/releases/latest)
```

发布前把 `<your-name>/<repo>` 替换成真实地址即可。

---

## 6. 可选：GitHub Pages 在线预览

导出的 HTML 是自包含的，可以直接发布成在线 Demo。

最简单的方式：

1. 新建 `gh-pages` 分支；
2. 把 `export/login-package/` 的内容复制到分支根目录；
3. 添加空文件 `.nojekyll`；
4. 在仓库 Settings → Pages 选择 `gh-pages`；
5. 访问 `https://<your-name>.github.io/<repo>/`。

注意：

- `index.html` 里的资源都是内联的，不依赖仓库其他文件；
- 想展示多个示例，可建 `/login/`、`/music/` 子目录；
- 动画截图可用 `?noanim=1` 静态查看。

---

## 7. README 结构建议

本仓库的 `README.md` 已经按这个结构写：

1. 一句话定位 + Hero 截图 + badges
2. **这个软件是做什么的**（面向谁、解决什么问题、能/不能做什么）
3. 界面速览（截图表格）
4. 快速开始（直接运行 / 源码构建）
5. **5 分钟上手**（画界面、参考线、渐变、动画、交互、导出）
6. 功能清单
7. `.ddesign` 示例
8. 给 DeepSeek 的用法
9. 项目结构
10. 构建与测试
11. 快捷键
12. 示例
13. FAQ
14. Roadmap
15. 贡献 / License

如果想让仓库首页更短，可以把“功能清单”“快捷键”下沉到 `docs/USER_GUIDE.md`，README 保留 5 分钟上手和链接即可。

---

## 8. 发布前检查清单

- [ ] `README.md` 中所有图片路径存在（相对路径大小写正确）
- [ ] 如需固定下载入口，替换 `your-name/your-repo` 占位链接（README 默认使用 Releases 页面，不需要）
- [ ] `LICENSE` 年份/署名改成你的名字
- [ ] 不提交 `build/`、`dist/`、`*.zip`
- [ ] Release 已上传 Windows 压缩包
- [ ] 仓库 About 填写一句话描述和 topics
- [ ] 社交预览图已设置
- [ ] 示例 `.ddesign` 可以打开
- [ ] `DeepDesignStudio.exe --selftest` 20/20
- [ ] 在干净的 Windows 环境（或另一台机器）解压 zip 运行一次
- [ ] 如果公开仓库，确认 Qt LGPL 合规（动态链接、附许可说明）
- [ ] 在 README 末尾确认 License 与第三方说明链接正确

---

## 9. 后续维护建议

- 每个版本更新 `README` 的 Roadmap / Release notes；
- 功能截图统一放 `screenshots/`，命名用 `功能-场景.png`；
- 每次修改 `.ddesign` 字段时同步更新：
  - `docs/DESIGN_FILE_FORMAT.md`
  - `docs/ddesign.schema.json`
  - `docs/AI_UI_SPEC.md`
- 新增交互/动画能力时同步更新 `HtmlExporter` 与 `AI_HANDOFF` 映射；
- 对 AI 生成错误的案例，补充到 `FAQ.md` 或 `AI_UI_SPEC.md` 的“常见错误”。
