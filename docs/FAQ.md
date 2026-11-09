# FAQ / 常见问题

## 运行与构建

### Q1：下载发布包后双击 exe 没反应 / 报缺少 DLL

优先使用 `dist/DeepDesignStudio/` 目录，或 GitHub Releases 中的完整压缩包。  
发布版已包含：

- Qt6Core / Qt6Gui / Qt6Widgets / Qt6Svg / Qt6Network
- `platforms/qwindows.dll`
- MinGW 运行库
- styles / imageformats / iconengines 插件

如果自己从源码构建，`build/bin` 只是 exe，需要 Qt 的 DLL 和插件。

### Q2：构建报 `qmake: could not find a Qt installation`

设置正确的 `QTDIR` 后重新运行 `build.bat`：

```bat
set QTDIR=D:\Qt6.11\6.11.2\mingw_64
set MINGW_BIN=D:\Qt6\Tools\mingw1310_64\bin
build.bat
```

`QTDIR` 和 `MINGW_BIN` 必须来自同一套 Qt/MinGW 安装。

### Q3：构建时 qmake 报 `failed to parse default include paths from compiler output`

通常是 qmake 无法启动/读取 g++。检查：

- `MINGW_BIN` 是否正确且包含 `g++.exe`；
- 是否被安全软件拦截；
- 是否在受限的沙箱环境中运行（需要允许子进程）。

### Q4：中文路径下 moc 警告

qmake/moc 在中文路径下可能输出：

```text
Warning: Failed to resolve include ".../moc_predefs.h"
```

本项目在 `E:\ai软件\...` 路径下实际构建、链接、运行均通过；该警告通常不影响产物。

### Q5：支持 macOS / Linux 吗？

代码基于 Qt Widgets，理论上可跨平台，但当前构建脚本和发布包只针对 Windows + MinGW。  
跨平台需要自行调整 `.pro`/构建脚本，并验证字体、DPI 和平台插件。

---

## 使用

### Q6：怎么从标尺拉参考线？

- 从**上方标尺**按住左键向下拖 → 横向参考线。
- 从**左侧标尺**按住左键向右拖 → 纵向参考线。
- 拖动时显示坐标；拖出画板外删除。
- `Ctrl+;` 显隐；视图 → 参考线 可新建/清除。

### Q7：为什么参考线没有出现在导出的 HTML / PNG 里？

参考线是**设计辅助**，只写入 `design.json` 的 `guides` 字段，不参与最终界面渲染。PNG/HTML 导出都不画参考线。

### Q8：渐变在哪里设置？

选中形状 → 右侧「外观」→ 填充方式 → **渐变**。  
支持线性/径向、角度、多色标、预设和调色板。

文字和直线不支持渐变填充：文字用文字颜色，直线用描边色。

### Q9：怎么给文字改颜色？

选中文字 → 右侧「文字」区改文字颜色；或点「调色板」中的颜色。

### Q10：动画关键帧记录哪些属性？

一次记录：

- X、Y、宽度、高度、旋转、不透明度；
- 非文本对象还会记录填充颜色。

描边宽度可以手动在时间轴中编辑，但「记录关键帧」默认不写入它。

### Q11：为什么修改了属性但动画没变化？

- 需要先把播放头放到目标时间，再点「记录关键帧」；
- 时间轴里的关键帧值是绝对值，不是偏移；
- 如果动画正在播放，编辑器可能显示预览帧，按停止/暂停后再编辑。

### Q12：交互怎么触发？

右侧「交互」：

- 触发：点击 / 悬停 / 移出 / 载入时；
- 动作：播放动画 / 停止全部动画；
- 选择动画 → 添加。

按 F5 进入交互预览；`load` 交互在预览打开时自动执行。

### Q13：F5 预览里没有动画

- 检查动画是否存在关键帧；
- 检查交互是否绑定了正确的动画；
- `click` 交互需要点击对象；
- 动画时长不能为 0。

### Q14：导出的 HTML 打开后动画没动

- 导出 HTML 默认会执行 `load` 交互；
- 如果没有 `load` 交互，需要点击/悬停对象；
- 可以点右下角「播放全部」或「重播」；
- 静态截图可用 `index.html?noanim=1`。

### Q15：为什么 HTML 与编辑器有 1–2px 差异？

浏览器与 Qt 的字体度量、换行、行高略有差异；布局坐标、渐变和动画值是同一份数据。  
如果要求像素级一致，建议直接使用 `.ddesign` 让 AI/程序按坐标生成，而不是比较截图。

### Q16：怎么给 DeepSeek 使用？

导出 AI 包后，把整个目录或以下文件给它：

```text
design.json
AI_HANDOFF.md
UI_SPEC.md
index.html（参考）
```

推荐开场：

```text
请阅读 design.json 和 AI_HANDOFF.md，按设计数据实现一个 HTML/React 界面。
严格使用绝对坐标、CSS/SVG 图形、渐变、时间轴动画与 click/hover/leave/load 交互。
```

### Q17：`.ddesign` 和 `design.json` 有什么区别？

内容基本一致：

- `.ddesign` 是设计软件保存的文件；
- `design.json` 是导出包中的同名数据；
- AI 读哪个都可以，二者结构相同。

### Q18：图层锁定后为什么点不中？

锁定的对象不能在画布直接选择/移动，但仍可在图层列表中选中并修改属性。  
这是为了防止误操作。

### Q19：能导入图片吗？

当前不支持图片/SVG 文件导入；所有图形由形状、路径、文字和渐变组成。  
可以把图片转成 SVG path 后人工转换为路径点（后续版本计划支持导入）。

### Q20：怎么反馈问题？

请在 GitHub Issues 中提供：

- 操作系统 + 软件版本；
- 操作步骤；
- 期望结果 / 实际结果；
- 相关 `.ddesign` 文件或截图（注意不要包含敏感信息）。
