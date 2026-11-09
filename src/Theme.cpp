#include "Theme.h"

#include <QApplication>
#include <QFontDatabase>

namespace Theme {

QFont uiFont(int pointSize, bool bold)
{
    QFont font(QStringLiteral("Segoe UI"));
    if (!QFontDatabase::families().contains(QStringLiteral("Segoe UI"))) {
        font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    }
    font.setPointSize(pointSize);
    font.setBold(bold);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

void apply(QApplication& app)
{
    app.setFont(uiFont(9));
    app.setStyleSheet(styleSheet());
}

QString styleSheet()
{
    return QStringLiteral(R"CSS(
* {
    outline: none;
}
QWidget {
    color: #D6D6D6;
    font-family: "Segoe UI", "Microsoft YaHei UI";
    font-size: 12px;
}
QMainWindow, QDialog {
    background: #1E1E1E;
}
QMenuBar {
    background: #323232;
    color: #D6D6D6;
    border-bottom: 1px solid #1E1E1E;
    padding: 2px 4px;
    spacing: 0px;
}
QMenuBar::item {
    background: transparent;
    padding: 5px 11px;
}
QMenuBar::item:selected {
    background: #4A4A4A;
    color: #FFFFFF;
}
QMenu {
    background: #2B2B2B;
    border: 1px solid #555555;
    padding: 4px;
}
QMenu::item {
    padding: 7px 32px 7px 28px;
    color: #D6D6D6;
}
QMenu::item:selected {
    background: #2D8CEB;
    color: #FFFFFF;
}
QMenu::item:disabled {
    color: #6B6B6B;
}
QMenu::separator {
    height: 1px;
    background: #454545;
    margin: 4px 8px;
}
QMenu::indicator {
    width: 14px;
    height: 14px;
}
QToolTip {
    background: #2B2B2B;
    color: #E8E8E8;
    border: 1px solid #555555;
    padding: 5px 8px;
}
QPushButton {
    background: #3C3C3C;
    color: #D6D6D6;
    border: 1px solid #555555;
    border-radius: 3px;
    padding: 5px 12px;
    min-height: 18px;
}
QPushButton:hover {
    background: #4A4A4A;
    border-color: #666666;
}
QPushButton:pressed {
    background: #333333;
}
QPushButton:disabled {
    color: #6B6B6B;
    background: #2B2B2B;
    border-color: #3A3A3A;
}
QPushButton#PrimaryButton {
    background: #2D8CEB;
    color: #FFFFFF;
    border: 1px solid #2D8CEB;
    font-weight: 600;
}
QPushButton#PrimaryButton:hover {
    background: #3D9CFA;
}
QPushButton#PrimaryButton:pressed {
    background: #1D7CDB;
}
QPushButton#GhostButton {
    background: transparent;
    border: 1px solid transparent;
    color: #C6C6C6;
    padding: 4px 7px;
}
QPushButton#GhostButton:hover {
    background: #4A4A4A;
    color: #FFFFFF;
}
QPushButton#DangerButton {
    background: #3A2626;
    color: #FF9A9A;
    border: 1px solid #5A3434;
}
QPushButton#DangerButton:hover {
    background: #4A2E2E;
}
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QPlainTextEdit, QTextEdit {
    background: #1E1E1E;
    color: #D6D6D6;
    border: 1px solid #4A4A4A;
    border-radius: 3px;
    padding: 4px 6px;
    selection-background-color: #2D8CEB;
    selection-color: #FFFFFF;
}
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus,
QPlainTextEdit:focus, QTextEdit:focus {
    border-color: #2D8CEB;
    background: #232323;
}
QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled {
    color: #6B6B6B;
    background: #262626;
}
QSpinBox::up-button, QDoubleSpinBox::up-button,
QSpinBox::down-button, QDoubleSpinBox::down-button {
    width: 14px;
    border: none;
    background: #3A3A3A;
}
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
    background: #4A4A4A;
}
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
    image: none;
    border-left: 3px solid transparent;
    border-right: 3px solid transparent;
    border-bottom: 4px solid #A0A0A0;
    width: 0;
    height: 0;
}
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
    image: none;
    border-left: 3px solid transparent;
    border-right: 3px solid transparent;
    border-top: 4px solid #A0A0A0;
    width: 0;
    height: 0;
}
QComboBox {
    padding-right: 20px;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 20px;
    border: none;
}
QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #A0A0A0;
    width: 0;
    height: 0;
    margin-right: 6px;
}
QComboBox QAbstractItemView {
    background: #2B2B2B;
    color: #D6D6D6;
    border: 1px solid #555555;
    selection-background-color: #2D8CEB;
    selection-color: #FFFFFF;
    padding: 3px;
    outline: none;
}
QCheckBox {
    color: #C6C6C6;
    spacing: 6px;
}
QCheckBox::indicator {
    width: 14px;
    height: 14px;
    border-radius: 2px;
    border: 1px solid #666666;
    background: #1E1E1E;
}
QCheckBox::indicator:hover {
    border-color: #2D8CEB;
}
QCheckBox::indicator:checked {
    background: #2D8CEB;
    border-color: #2D8CEB;
}
QTabWidget::pane {
    border: none;
}
QTabBar::tab {
    background: #252525;
    color: #A0A0A0;
    padding: 7px 14px;
    border-right: 1px solid #1A1A1A;
}
QTabBar::tab:selected {
    background: #3A3A3A;
    color: #FFFFFF;
}
QScrollArea {
    border: none;
    background: transparent;
}
QScrollBar:vertical {
    background: #262626;
    width: 11px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #5A5A5A;
    min-height: 24px;
    border-radius: 5px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover {
    background: #6E6E6E;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: transparent;
}
QScrollBar:horizontal {
    background: #262626;
    height: 11px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: #5A5A5A;
    min-width: 24px;
    border-radius: 5px;
    margin: 2px;
}
QScrollBar::handle:horizontal:hover {
    background: #6E6E6E;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background: transparent;
}
QListWidget {
    background: transparent;
    border: none;
    outline: none;
}
QListWidget::item {
    border: none;
}
QSplitter::handle {
    background: #1A1A1A;
}
QSplitter::handle:hover {
    background: #2D8CEB;
}
QStatusBar {
    background: #323232;
    color: #A0A0A0;
    border-top: 1px solid #1A1A1A;
}
QStatusBar::item {
    border: none;
}
QToolButton {
    background: transparent;
    border: none;
    border-radius: 3px;
    padding: 5px;
}
QToolButton:hover {
    background: #4A4A4A;
}
QToolButton:checked {
    background: #555555;
}
QToolButton::menu-indicator {
    image: none;
}
QWidget#OptionsBar {
    background: #323232;
    border-bottom: 1px solid #1A1A1A;
}
QFrame#PanelHeader {
    background: #383838;
    border-top: 1px solid #454545;
    border-bottom: 1px solid #1A1A1A;
}
QFrame#PanelBody {
    background: #2B2B2B;
}
QWidget#DocumentBar {
    background: #252525;
    border-bottom: 1px solid #1A1A1A;
}
QLabel#DocumentTab {
    background: #383838;
    color: #E0E0E0;
    border-right: 1px solid #1A1A1A;
    padding: 0 18px;
}
QLabel#PanelTitle {
    color: #D6D6D6;
    font-weight: 600;
}
QLabel#FieldLabel {
    color: #A0A0A0;
}
QFrame#SectionHeader {
    background: #333333;
    border-top: 1px solid #3F3F3F;
    border-bottom: 1px solid #262626;
}
QProgressBar {
    background: #1E1E1E;
    border: none;
    border-radius: 2px;
    height: 4px;
}
QProgressBar::chunk {
    background: #2D8CEB;
    border-radius: 2px;
}
)CSS");
}

} // namespace Theme
