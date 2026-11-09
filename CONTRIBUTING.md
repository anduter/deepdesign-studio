# 贡献指南

感谢你愿意参与灵绘 DeepDesign Studio。

## 开发环境

- Windows 10/11
- Qt 6.11.2 MinGW 64-bit（Qt 6.5+ 理论可用，未逐一验证）
- MinGW 13.1（随 Qt 安装）
- qmake，不需要 CMake

## 构建

```bat
set QTDIR=D:\Qt6.11\6.11.2\mingw_64
set MINGW_BIN=D:\Qt6\Tools\mingw1310_64\bin
build.bat
```

## 运行与测试

```bat
run.bat
build\bin\DeepDesignStudio.exe --selftest
```

自测报告写入 `selftest-report.txt`，提交 PR 前请确保 20 项全部通过。

## 代码结构

- `src/Model.*`：文档、元素、渐变、动画、交互、参考线
- `src/Renderer.*`：QPainter 渲染
- `src/CanvasWidget.*`：画布和编辑交互
- `src/PropertiesPanel.*`：属性、渐变、调色板、交互
- `src/TimelinePanel.*`：时间轴和关键帧
- `src/HtmlExporter.*` / `src/SpecExporter.*`：导出
- `docs/`：格式、AI 指南、用户手册
- `examples/`：示例设计

## 提交规范

建议使用 Conventional Commits：

```text
feat: add text alignment buttons
fix: keep guide position when canvas zooms
docs: explain gradient angle mapping
```

## PR 检查清单

- [ ] 构建通过
- [ ] `--selftest` 20/20
- [ ] 新增字段同步更新 `docs/DESIGN_FILE_FORMAT.md` 和 `docs/ddesign.schema.json`
- [ ] 新增 UI 文字为中文/英文一致风格
- [ ] 截图放入 `screenshots/` 并在文档中引用
- [ ] 不提交 `build/`、`dist/`、`*.zip`
- [ ] 不引入来源不明的图片/字体资源

## Issue 模板建议

提交 Bug 时请附：

- 系统版本、软件版本
- 操作步骤
- 期望 / 实际结果
- `.ddesign` 文件或截图
- 自测报告（如果相关）
