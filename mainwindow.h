#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
class KeyBoard;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* w, QEvent* e);

private:
    Ui::MainWindow* ui;
    KeyBoard* keyboard;
    QPoint m_lastPos;
    bool offsetFlag = false;
};

#endif // MAINWINDOW_H
