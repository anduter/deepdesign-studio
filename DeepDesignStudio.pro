QT += core gui widgets

CONFIG += c++17 release
TEMPLATE = app
TARGET = DeepDesignStudio

DESTDIR = bin
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc

SOURCES += \
    src/main.cpp \
    src/Model.cpp \
    src/Animation.cpp \
    src/ProjectIO.cpp \
    src/Theme.cpp \
    src/Icons.cpp \
    src/Widgets.cpp \
    src/Renderer.cpp \
    src/CanvasWidget.cpp \
    src/ToolPalette.cpp \
    src/OptionsBar.cpp \
    src/LayersPanel.cpp \
    src/PropertiesPanel.cpp \
    src/TimelinePanel.cpp \
    src/PreviewDialog.cpp \
    src/HtmlExporter.cpp \
    src/SpecExporter.cpp \
    src/MainWindow.cpp

HEADERS += \
    src/Model.h \
    src/Animation.h \
    src/ProjectIO.h \
    src/Theme.h \
    src/Icons.h \
    src/Widgets.h \
    src/Renderer.h \
    src/CanvasWidget.h \
    src/ToolPalette.h \
    src/OptionsBar.h \
    src/LayersPanel.h \
    src/PropertiesPanel.h \
    src/TimelinePanel.h \
    src/PreviewDialog.h \
    src/HtmlExporter.h \
    src/SpecExporter.h \
    src/MainWindow.h

win32: QMAKE_CXXFLAGS += -Wall -Wextra -Wno-unused-parameter
