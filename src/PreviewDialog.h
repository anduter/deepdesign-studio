#pragma once

#include "Model.h"

#include <QDialog>

class PreviewCanvas;
class QLabel;

class PreviewDialog : public QDialog {
    Q_OBJECT
public:
    explicit PreviewDialog(const DesignDocument& document, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void buildUi();

    DesignDocument m_document;
    PreviewCanvas* m_canvas = nullptr;
    QLabel* m_statusLabel = nullptr;
};
