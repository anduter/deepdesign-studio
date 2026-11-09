#include "ProjectIO.h"
#include "Renderer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QPainter>
#include <QSaveFile>

namespace ProjectIO {

bool save(const DesignDocument& document, const QString& path, QString* error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = QStringLiteral("无法写入文件：%1").arg(file.errorString());
        return false;
    }
    const QJsonDocument json(document.toJson());
    if (file.write(json.toJson(QJsonDocument::Indented)) < 0) {
        if (error) *error = QStringLiteral("写入文件失败：%1").arg(file.errorString());
        return false;
    }
    if (!file.commit()) {
        if (error) *error = QStringLiteral("保存文件失败：%1").arg(file.errorString());
        return false;
    }
    if (error) error->clear();
    return true;
}

bool load(DesignDocument& document, const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = QStringLiteral("无法打开文件：%1").arg(file.errorString());
        return false;
    }
    const QByteArray data = file.readAll();
    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) *error = QStringLiteral("JSON 解析错误（偏移 %1）：%2")
                                .arg(parseError.offset).arg(parseError.errorString());
        return false;
    }
    if (!json.isObject()) {
        if (error) *error = QStringLiteral("设计文件根节点必须是 JSON 对象");
        return false;
    }
    QString message;
    if (!document.fromJson(json.object(), &message)) {
        if (error) *error = message;
        return false;
    }
    if (error) error->clear();
    return true;
}

QImage renderToImage(const DesignDocument& document, double scale, bool grid)
{
    const int width = qMax(1, qRound(document.canvasSize.width() * scale));
    const int height = qMax(1, qRound(document.canvasSize.height() * scale));
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.scale(scale, scale);
    painter.setClipRect(QRectF(QPointF(0, 0), document.canvasSize));
    Renderer::paintScene(painter, document, {}, grid, scale);
    painter.end();
    return image;
}

QString fileFilter()
{
    return QStringLiteral("DeepDesign 设计文件 (*.ddesign *.json);;所有文件 (*)");
}

QString defaultSuffix()
{
    return QStringLiteral("ddesign");
}

} // namespace ProjectIO
