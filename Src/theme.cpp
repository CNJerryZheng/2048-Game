#include "theme.h"

namespace Theme
{

    QColor tileBackground(int value)
    {
        switch (value)
        {
        case 2:
            return {236, 228, 219};
        case 4:
            return {232, 217, 186};
        case 8:
            return {231, 178, 125};
        case 16:
            return {232, 152, 107};
        case 32:
            return {230, 127, 98};
        case 64:
            return {227, 99, 66};
        case 128:
            return {236, 208, 105};
        case 256:
            return {237, 212, 115};
        case 512:
            return {239, 202, 87};
        case 1024:
            return {240, 200, 78};
        default:
            return value >= 2048 ? QColor(248, 211, 71)
                                 : QColor(216, 206, 196);
        }
    }

    QColor tileForeground(int value)
    {
        return value >= 8 ? QColor(255, 255, 255)
                          : QColor(114, 101, 84);
    }

    QFont tileFont(int value, const QFont &baseFont)
    {
        QFont font = baseFont;
        font.setWeight(QFont::Black);
        font.setPixelSize(value < 1000 ? 29 : value < 10000 ? 24
                                                            : 20);
        return font;
    }

    QColor currentUserHighlight()
    {
        return QColor("#fff0a6");
    }

    QString applicationStyleSheet()
    {
        return QStringLiteral(
            "QMainWindow{background:#faf8ef;}"
            "QStackedWidget{background:#faf8ef;}"
            "QLabel{color:#776e65;}"
            "QCheckBox{color:#3c3a32;background:transparent;font-size:14px;spacing:7px;}"
            "QCheckBox::indicator{width:15px;height:15px;border:1px solid #3c3a32;"
            "border-radius:3px;background:#fffdf8;}"
            "QCheckBox::indicator:checked{background:#3c3a32;border:1px solid #3c3a32;}"
            "QLabel[role=\"hint\"]{font-size:14px;color:#776e65;}"
            "QDialog,QMessageBox{background:#faf8ef;color:#3c3a32;}"
            "QDialog QLabel,QMessageBox QLabel{color:#3c3a32;background:transparent;}"
              "QLineEdit,QSpinBox,QTextEdit,QComboBox{padding:9px;border:1px solid #bbada0;border-radius:6px;"
              "background:#ffffff;color:#3c3a32;selection-background-color:#8f7a66;"
              "selection-color:#ffffff;}"
              "QLineEdit:focus,QSpinBox:focus,QTextEdit:focus,QComboBox:focus{border:2px solid #8f7a66;}"
              "QComboBox QAbstractItemView{background:#ffffff;color:#3c3a32;"
              "selection-background-color:#edc22e;selection-color:#3c3a32;"
              "border:1px solid #bbada0;outline:0;}"
            "QPushButton{background:#edc22e;color:#3c3a32;border:1px solid #c9a51f;"
            "border-radius:6px;padding:8px 16px;font-weight:700;}"
            "QPushButton:hover{background:#f2cf48;}"
            "QPushButton:pressed{background:#d8ad1f;}"
            "QPushButton:disabled{background:#ddd5cc;color:#776e65;border-color:#c8beb4;}"
            "QTabWidget::pane{border:0;}"
            "QTabBar::tab{color:#3c3a32;background:#fffdf8;border:1px solid #ded6cc;"
            "padding:8px 16px;margin-right:2px;}"
            "QTabBar::tab:selected{color:#3c3a32;background:#f2cf48;border-color:#d8ad1f;}"
            "QTabBar::tab:hover:!selected{background:#f5efe6;}"
            "QTableWidget{background:#fffdf8;color:#3c3a32;border:1px solid #ded6cc;"
            "border-radius:8px;gridline-color:#e8e1d8;alternate-background-color:#f7f3ed;"
            "selection-background-color:#f8e59a;selection-color:#3c3a32;}"
            "QListWidget{background:#fffdf8;color:#3c3a32;border:1px solid #ded6cc;"
            "border-radius:8px;selection-background-color:#f8e59a;selection-color:#3c3a32;}"
            "QListWidget::item{color:#3c3a32;padding:5px;}"
            "QTableWidget::item{color:#3c3a32;padding:5px;}"
            "QHeaderView::section{background:#eee8df;color:#3c3a32;border:0;"
            "border-right:1px solid #ded6cc;border-bottom:1px solid #ded6cc;"
            "padding:7px;font-weight:700;}"
            "QTableCornerButton::section{background:#eee8df;border:0;}");
    }

}
