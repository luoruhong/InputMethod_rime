#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QKeyEvent>
#include <keyboard.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    keyboard = new KeyBoard(this);
    keyboard->showUI(3);
    this->hide();;
    connect(keyboard, &KeyBoard::keyBoardOffset, this, [this](int offset) { // 键盘弹出,界面上移
        if (this->isVisible()) {
            m_lastPos = ui->mainView->pos();
            ui->mainView->move(m_lastPos.x(), m_lastPos.y() + offset);
            offsetFlag = true;
        }
    });
    connect(keyboard, &KeyBoard::keyBoardHidden, this, [this] { // 键盘收起,界面还原
        if (this->isVisible() && offsetFlag) {
            ui->mainView->move(m_lastPos);
            offsetFlag = false;
        }
    });
    this->setFocusPolicy(Qt::StrongFocus); // 点击后,吸收焦点
    QList<QWidget*> list = { ui->lineEdit, ui->textEdit, ui->spinBox, ui->dateTimeEdit };
    for (auto item : QList<QWidget*> { ui->lineEdit, ui->textEdit, ui->spinBox, ui->dateTimeEdit }) {
        item->installEventFilter(this);
        item->setFocusPolicy(Qt::StrongFocus); // 设置TabFocus | ClickFocus
    }
    for (int i = 1; i < list.count(); i++) {
        setTabOrder(list[i - 1], list[i]);
    }
}

MainWindow::~MainWindow()
{
    delete keyboard;
    delete ui;
}

bool MainWindow::eventFilter(QObject* w, QEvent* e)
{
    auto type = e->type();

    if (QWidget* p = qobject_cast<QWidget*>(w)) {
        if (type == QEvent::FocusIn) {
            keyboard->setEdit(p);
            if (ui->spinBox == p) {
                keyboard->showUI(1, true);
            } else if (ui->dateTimeEdit == p) {
                keyboard->showUI(1);
            } else {
                keyboard->showUI(3, true);
            }

        } else if (type == QEvent::FocusOut) {
            // keyboard->hide();
        } else if (type == QEvent::KeyPress) {
            QKeyEvent* event = static_cast<QKeyEvent*>(e);
            int key = event->key();
            if (key == Qt::Key_Enter) {
                p->clearFocus();
            }
        }
    }

    return QWidget::eventFilter(w, e);
}
