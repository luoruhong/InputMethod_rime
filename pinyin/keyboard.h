#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "keybutton.h"
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QPointer>
#include <QScroller>
#include <QShowEvent>
#include <QWidget>
class RimeUtils;

namespace Ui {
class KeyBoard;
}

class KeyBoard : public QWidget {
    Q_OBJECT

signals:
    // 键盘隐藏
    void keyBoardHidden();

    // 键盘光标离键盘高度
    void keyBoardOffset(int height);

public:
    enum TYPE {
        Type_Low = 1,
        Type_high,
        Type_mark,
        Type_chinese
    };
    explicit KeyBoard(QWidget* parent = nullptr);
    ~KeyBoard();
    static KeyBoard* getInstance();
    void setEdit(QWidget* edit);

    /**
   * @brief 显示键盘
   * @param mode=1，纯数字加字符。mode=2，英文加数字。mode=3，中文。
   * @param hasNext
   */
    void showUI(int mode, bool hasNext = false);

protected:
    /**
     * @brief 切换键盘
     * @param type
     */
    void switchState(TYPE type);

    /**
     * @brief 获取字符匹配的中文字符组
     * @param text
     */
    void getCandidateList(const QString text);

    void hideEvent(QHideEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;
protected slots:

    //发送键盘事件
    void slot_key_keyEvent(int key, QString value);

private slots:
    void on_listWidget_itemClicked(QListWidgetItem* item);

    void on_tableWidget_cellClicked(int row, int column);

    void on_textinputBtn_toggled(bool checked);

private:
    /**
   * @brief 设置下一步
   * @param hasNext
   */
    void setNext(bool hasNext);
    void updateInputText(const QString& text);

private:
    static KeyBoard* m_instance;

    Ui::KeyBoard* ui;
    TYPE m_type;

    RimeUtils* engine;

    QPointer<QWidget> m_edit;

    QList<KeyButton*> keyBtnList;

    QStringList btnvalueList_small;
    QStringList btnvalueList_large;
    QStringList btnvalueList_mark;

    QList<Qt::Key> keyList_low;
    QList<Qt::Key> keyList_mark;
    QString m_inputText;
    bool m_dragging = false;
    QPoint m_dragPosition;
};

#endif // KEYBOARD_H
