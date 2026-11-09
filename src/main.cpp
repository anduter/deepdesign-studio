#include "Animation.h"
#include "HtmlExporter.h"
#include "Icons.h"
#include "MainWindow.h"
#include "Model.h"
#include "ProjectIO.h"
#include "Renderer.h"
#include "SpecExporter.h"
#include "Theme.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QtMath>
#include <cstdio>
#include <cstdlib>

namespace {

int runSelfTest()
{
    QFile reportFile(QDir::current().filePath(QStringLiteral("selftest-report.txt")));
    if (!reportFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return 2;
    }
    QTextStream out(&reportFile);
    int passed = 0;
    int failed = 0;
    auto check = [&](bool condition, const QString& label) {
        if (condition) {
            ++passed;
            out << "  PASS  " << label << "\n";
        } else {
            ++failed;
            out << "  FAIL  " << label << "\n";
        }
        out.flush();
    };

    out << "DeepDesign Studio 自测\n";

    DesignDocument document;
    document.title = QStringLiteral("自测设计");
    document.canvasSize = QSizeF(960, 600);
    document.background = QColor("#101820");

    auto addElement = [&](Element element) {
        element.id = document.makeId(elementTypeToString(element.type));
        if (element.name.isEmpty()) element.name = elementTypeDisplayName(element.type);
        document.addElement(element);
        return element.id;
    };

    Element rectangle;
    rectangle.type = ElementType::Rectangle;
    rectangle.name = QStringLiteral("主卡片");
    rectangle.frame = QRectF(80, 90, 320, 220);
    rectangle.fill = QColor("#1E2A44");
    rectangle.stroke = QColor("#3C5A8C");
    rectangle.strokeWidth = 2.0;
    rectangle.radius = 18.0;
    rectangle.gradient.enabled = true;
    rectangle.gradient.type = QStringLiteral("linear");
    rectangle.gradient.angle = 135.0;
    rectangle.gradient.stops = {{0.0, QColor("#2D8CEB")},
                                {0.45, QColor("#A47BFF")},
                                {1.0, QColor("#FF6B81")}};
    const QString rectId = addElement(rectangle);

    Element ellipse;
    ellipse.type = ElementType::Ellipse;
    ellipse.name = QStringLiteral("光晕");
    ellipse.frame = QRectF(520, 120, 180, 180);
    ellipse.fill = QColor("#2D8CEB");
    const QString ellipseId = addElement(ellipse);
    Q_UNUSED(ellipseId);

    Element line;
    line.type = ElementType::Line;
    line.name = QStringLiteral("分隔线");
    line.frame = QRectF(90, 340, 260, 2);
    line.stroke = QColor("#3C5A8C");
    line.strokeWidth = 2.0;
    line.lineStart = QPointF(0, 0.5);
    line.lineEnd = QPointF(1, 0.5);
    addElement(line);

    Element polygon;
    polygon.type = ElementType::Polygon;
    polygon.name = QStringLiteral("六边形");
    polygon.frame = QRectF(540, 340, 120, 120);
    polygon.fill = QColor("#A47BFF");
    polygon.sides = 6;
    polygon.gradient.enabled = true;
    polygon.gradient.type = QStringLiteral("radial");
    polygon.gradient.stops = {{0.0, QColor("#A47BFF")}, {1.0, QColor("#0F172A")}};
    addElement(polygon);

    Element star;
    star.type = ElementType::Star;
    star.name = QStringLiteral("星标");
    star.frame = QRectF(700, 340, 120, 120);
    star.fill = QColor("#FFB454");
    star.sides = 5;
    star.gradient.enabled = true;
    star.gradient.type = QStringLiteral("linear");
    star.gradient.angle = 90.0;
    star.gradient.stops = {{0.0, QColor("#FFB454")}, {1.0, QColor("#FF4D8D")}};
    addElement(star);

    Element path;
    path.type = ElementType::Path;
    path.name = QStringLiteral("自由路径");
    path.frame = QRectF(80, 390, 300, 140);
    path.fill = QColor(45, 140, 235, 170);
    path.stroke = QColor("#5AA9F0");
    path.strokeWidth = 2.0;
    path.closed = true;
    path.points = {QPointF(0.05, 0.6), QPointF(0.25, 0.05), QPointF(0.55, 0.25),
                   QPointF(0.85, 0.1), QPointF(0.95, 0.85), QPointF(0.4, 0.95)};
    addElement(path);

    Element text;
    text.type = ElementType::Text;
    text.name = QStringLiteral("标题");
    text.frame = QRectF(110, 130, 260, 60);
    text.text = QStringLiteral("AI 原生设计");
    text.fontFamily = QStringLiteral("Microsoft YaHei UI");
    text.fontSize = 32;
    text.bold = true;
    text.align = QStringLiteral("left");
    text.textColor = QColor("#FFFFFF");
    addElement(text);

    Animation& animation = document.addAnimation(QStringLiteral("入场"));
    animation.duration = 1.2;
    animation.easing = QStringLiteral("linear");
    Track& opacityTrack = animation.ensureTrack(rectId, QStringLiteral("opacity"));
    opacityTrack.keys = {{0.0, 0.0}, {1.2, 1.0}};
    Track& moveTrack = animation.ensureTrack(rectId, QStringLiteral("x"));
    moveTrack.keys = {{0.0, 60.0}, {1.2, 80.0}};
    Interaction interaction;
    interaction.sourceId = rectId;
    interaction.trigger = QStringLiteral("click");
    interaction.action = QStringLiteral("playAnimation");
    interaction.animationId = animation.id;
    document.interactions.append(interaction);
    Interaction loadInteraction;
    loadInteraction.sourceId = rectId;
    loadInteraction.trigger = QStringLiteral("load");
    loadInteraction.action = QStringLiteral("playAnimation");
    loadInteraction.animationId = animation.id;
    document.interactions.append(loadInteraction);

    document.addGuide(QStringLiteral("horizontal"), 240.0);
    document.addGuide(QStringLiteral("vertical"), 420.0);

    check(document.elements.size() == 7, QStringLiteral("文档包含全部 7 种元素类型"));

    const QString testRoot = QDir::current().filePath(QStringLiteral("selftest-artifacts"));
    QDir(testRoot).removeRecursively();
    const bool testDirReady = QDir().mkpath(testRoot);
    check(testDirReady, QStringLiteral("创建临时目录"));
    const QString projectPath = QDir(testRoot).filePath(QStringLiteral("selftest.ddesign"));
    QString error;
    check(ProjectIO::save(document, projectPath, &error), QStringLiteral("保存 .ddesign 文件"));
    if (!error.isEmpty()) out << "        " << error << "\n";

    DesignDocument loaded;
    check(ProjectIO::load(loaded, projectPath, &error), QStringLiteral("读取 .ddesign 文件"));
    check(loaded.elements.size() == document.elements.size(), QStringLiteral("元素数量往返一致"));
    check(loaded.toJson() == document.toJson(), QStringLiteral("完整 JSON 往返一致"));
    check(loaded.animations.size() == 1 && loaded.interactions.size() == 2,
          QStringLiteral("动画与交互数据保留"));
    check(loaded.guides.size() == 2 && loaded.guides.first().orientation == QLatin1String("horizontal"),
          QStringLiteral("参考线数据保留"));
    const Element* loadedRect = loaded.findElement(rectId);
    check(loadedRect && loadedRect->hasGradient() && loadedRect->gradient.stops.size() == 3,
          QStringLiteral("渐变填充数据保留"));

    const auto sample = loaded.sampleAnimation(animation.id, 0.6);
    const double sampledOpacity = sample.value(rectId).value(QStringLiteral("opacity")).toDouble();
    check(qAbs(sampledOpacity - 0.5) < 0.01, QStringLiteral("动画在 0.6s 采样不透明度为 0.5"));

    const QImage image = ProjectIO::renderToImage(loaded, 1.0, false);
    check(!image.isNull() && image.width() == 960 && image.height() == 600,
          QStringLiteral("离屏渲染画板为 960×600"));

    const QString exportDir = QDir(testRoot).filePath(QStringLiteral("handoff"));
    check(HtmlExporter::exportPackage(loaded, exportDir, &error), QStringLiteral("导出 AI / HTML 交付包"));
    check(QFileInfo::exists(exportDir + QStringLiteral("/index.html")), QStringLiteral("生成 index.html"));
    check(QFileInfo::exists(exportDir + QStringLiteral("/design.json")), QStringLiteral("生成 design.json"));
    check(QFileInfo::exists(exportDir + QStringLiteral("/UI_SPEC.md")), QStringLiteral("生成 UI_SPEC.md"));
    check(QFileInfo::exists(exportDir + QStringLiteral("/AI_HANDOFF.md")), QStringLiteral("生成 AI_HANDOFF.md"));

    QFile htmlFile(exportDir + QStringLiteral("/index.html"));
    if (htmlFile.open(QIODevice::ReadOnly)) {
        const QString html = QString::fromUtf8(htmlFile.readAll());
        check(html.contains(QStringLiteral("playAnimation")) && html.contains(QStringLiteral("AI 原生设计")),
              QStringLiteral("HTML 包含交互运行时与文本内容"));
        check(html.contains(QStringLiteral("const DESIGN =")) && html.contains(QStringLiteral("easeOutElastic")),
              QStringLiteral("HTML 内嵌设计数据与完整缓动函数"));
        check(html.contains(QStringLiteral("linear-gradient")) && html.contains(QStringLiteral("linearGradient")),
              QStringLiteral("HTML 导出包含 CSS 与 SVG 渐变"));
    } else {
        check(false, QStringLiteral("读取 index.html"));
    }

    QFile specFile(exportDir + QStringLiteral("/UI_SPEC.md"));
    if (specFile.open(QIODevice::ReadOnly)) {
        const QString spec = QString::fromUtf8(specFile.readAll());
        check(spec.contains(QStringLiteral("设计令牌")) && spec.contains(QStringLiteral("动效时间线")) &&
                  spec.contains(QStringLiteral("主卡片")) && spec.contains(QStringLiteral("参考线")),
              QStringLiteral("UI_SPEC 包含令牌 / 动画 / 图层表 / 参考线"));
    } else {
        check(false, QStringLiteral("读取 UI_SPEC.md"));
    }

    out << QStringLiteral("\n结果：%1 通过，%2 失败\n").arg(passed).arg(failed);
    out.flush();
    reportFile.close();
    return failed == 0 ? 0 : 1;
}

void printUsage()
{
    QTextStream out(stdout);
    out << "灵绘 DeepDesign Studio\n"
           "用法：DeepDesignStudio [文件.ddesign]\n"
           "      DeepDesignStudio --selftest\n"
           "      DeepDesignStudio --render <输入.ddesign> <输出.png> [缩放]\n"
           "      DeepDesignStudio --export <输入.ddesign> <输出目录>\n"
           "      DeepDesignStudio --validate <输入.ddesign>\n";
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("DeepDesign Studio"));
    QApplication::setOrganizationName(QStringLiteral("DeepDesign"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setStyle(QStringLiteral("Fusion"));
    Theme::apply(application);
    QApplication::setWindowIcon(QIcon(Icons::pixmap(QStringLiteral("sparkle"), 48,
                                                    QColor("#2D8CEB"), 2.0)));

    const QStringList arguments = QApplication::arguments();
    auto finishCli = [](int code) {
        std::fflush(stdout);
        std::fflush(stderr);
        std::_Exit(code);
    };
    if (arguments.contains(QStringLiteral("--help")) || arguments.contains(QStringLiteral("-h"))) {
        printUsage();
        finishCli(0);
    }
    if (arguments.contains(QStringLiteral("--selftest"))) {
        const int result = runSelfTest();
        finishCli(result);
    }

    auto argumentAfter = [&](const QString& option) -> QString {
        const int index = arguments.indexOf(option);
        if (index >= 0 && index + 1 < arguments.size()) return arguments.at(index + 1);
        return {};
    };

    if (arguments.contains(QStringLiteral("--render"))) {
        const QString input = argumentAfter(QStringLiteral("--render"));
        const int optionIndex = arguments.indexOf(QStringLiteral("--render"));
        if (input.isEmpty() || optionIndex + 2 >= arguments.size()) {
            printUsage();
            finishCli(1);
        }
        const QString output = arguments.at(optionIndex + 2);
        double scale = 2.0;
        if (optionIndex + 3 < arguments.size()) {
            bool ok = false;
            const double parsed = arguments.at(optionIndex + 3).toDouble(&ok);
            if (ok && parsed > 0.0) scale = parsed;
        }
        DesignDocument document;
        QString error;
        if (!ProjectIO::load(document, input, &error)) {
            QTextStream(stderr) << error << "\n";
            finishCli(1);
        }
        const QImage image = ProjectIO::renderToImage(document, scale, false);
        if (!image.save(output, "PNG")) {
            QTextStream(stderr) << "无法写入 " << output << "\n";
            finishCli(1);
        }
        QTextStream(stdout) << "已渲染 " << output << " (" << image.width() << "×" << image.height() << ")\n";
        finishCli(0);
    }

    if (arguments.contains(QStringLiteral("--export"))) {
        const int optionIndex = arguments.indexOf(QStringLiteral("--export"));
        if (optionIndex + 2 >= arguments.size()) {
            printUsage();
            finishCli(1);
        }
        const QString input = arguments.at(optionIndex + 1);
        const QString output = arguments.at(optionIndex + 2);
        DesignDocument document;
        QString error;
        if (!ProjectIO::load(document, input, &error)) {
            QTextStream(stderr) << error << "\n";
            finishCli(1);
        }
        if (!HtmlExporter::exportPackage(document, output, &error)) {
            QTextStream(stderr) << error << "\n";
            finishCli(1);
        }
        QTextStream(stdout) << "已导出到 " << output << "\n";
        finishCli(0);
    }

    if (arguments.contains(QStringLiteral("--validate"))) {
        const QString input = argumentAfter(QStringLiteral("--validate"));
        DesignDocument document;
        QString error;
        if (input.isEmpty() || !ProjectIO::load(document, input, &error)) {
            QTextStream(stderr) << error << "\n";
            finishCli(1);
        }
        QTextStream(stdout) << "标题：" << document.title << "\n"
                            << "画布：" << document.canvasSize.width() << "×"
                            << document.canvasSize.height() << "\n"
                            << "元素：" << document.elements.size() << "\n"
                            << "动画：" << document.animations.size() << "\n"
                            << "交互：" << document.interactions.size() << "\n"
                            << "参考线：" << document.guides.size() << "\n";
        finishCli(0);
    }

    MainWindow window;
    QString initialFile;
    for (int i = 1; i < arguments.size(); ++i) {
        const QString argument = arguments.at(i);
        if (!argument.startsWith(QLatin1Char('-'))) {
            initialFile = argument;
            break;
        }
    }
    if (!initialFile.isEmpty()) window.openDocument(initialFile);
    window.show();
    return application.exec();
}
