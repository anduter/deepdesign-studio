# 第三方组件与许可说明

本项目源代码使用 MIT License（见 `LICENSE`），但构建和分发时使用了第三方组件。发布二进制前请确认你遵守相应许可。

## Qt 6

- 版本：Qt 6.11.2（开发验证）
- 许可：LGPLv3 / 商业许可（Qt 公司）
- 官网：https://www.qt.io/
- 许可说明：https://www.qt.io/licensing/open-source-lgpl-obligations

本项目以动态链接方式使用 Qt。发布二进制时通常需要：

- 附带 Qt 的 LGPL 许可文本；
- 允许用户替换 Qt 动态库（动态链接已满足主要条件）；
- 不静态链接 Qt（除非取得商业许可）；
- 提供对应 Qt 源码获取方式（LGPL 要求）。

`deploy.bat` 使用 `windeployqt` 复制 Qt 动态库与插件。请在自己的发布包中附上 Qt 的 LICENSE 文件；Qt 安装目录通常包含相关许可文本。

## MinGW / GCC 运行库

- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

这些运行库来自 MinGW-w64 工具链，遵循其对应许可（GCC Runtime Library Exception 等）。以动态库形式随程序分发。

## 字体

软件不捆绑字体，使用系统字体（如 Microsoft YaHei UI、Segoe UI）与用户选择的系统字体。  
导出的 HTML 依赖浏览者系统字体，可能产生渲染差异。

## 图标

所有图标由 `QPainter` 代码手绘，没有使用第三方图片素材。

## 示例设计

`examples/` 中的 `.ddesign` 示例由本项目生成，采用 MIT License。  
示例中的名称、文案仅为演示用途。
