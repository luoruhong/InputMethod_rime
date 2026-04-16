#include "keyboard.h"

#include "rimeutils.h"
#include "ui_keyboard.h"
#include <QDebug>
#include <QDir>
#include <QScreen>
#include <QScrollBar>
#include <QTimer>

KeyBoard* KeyBoard::m_instance = nullptr;

KeyBoard* KeyBoard::getInstance()
{
    if (m_instance == nullptr) {
        m_instance = new KeyBoard();
    }
    return m_instance;
}

void KeyBoard::showUI(int mode, bool hasNext)
{
    setNext(hasNext);
    if (mode == 0) {
        ui->pushButton_shift->show(); // 大小写切换按钮
        ui->pushButton_123->show(); // 数字切换按钮
        ui->pushButton_en->show(); // 中英文切换按钮
    } else if (mode == 1) {
        ui->pushButton_shift->show(); // 大小写切换按钮
        ui->pushButton_123->show(); // 数字切换按钮
        ui->pushButton_en->show(); // 中英文切换按钮
        switchState(Type_mark);
    } else if (mode == 2) {
        ui->pushButton_123->show();
        ui->pushButton_shift->show();
        ui->pushButton_en->show();
        switchState(Type_Low);
    } else {
        ui->pushButton_shift->show();
        ui->pushButton_123->show();
        ui->pushButton_en->show();
        switchState(Type_chinese);
    }
    if (this->isHidden()) {
        auto parent = parentWidget();
        this->move((parent->size().width() - this->width()) / 2,
            parent->size().height() - this->height());
        this->show();

        if (m_edit) { // 获取输入光标位置,判断键盘是否挡住光标
            QPoint pos = m_edit->mapToGlobal(QPoint(0, 0));
            int y = pos.y() + m_edit->height();
            int offset = this->y() - y;
            if (offset < 0) {
                emit keyBoardOffset(offset - 10); // 当前输入框的坐标往上移10px
            }
        }
    }
}

void KeyBoard::setNext(bool hasNext)
{
    ui->pushButton_tab->setEnabled(hasNext);
    if (hasNext) {
        ui->pushButton_tab->show();
        ui->pushButton_enter->hide();
    } else {
        ui->pushButton_enter->show();
        ui->pushButton_tab->hide();
    }
}

void KeyBoard::updateInputText(const QString& text)
{
    getCandidateList(text);
}

KeyBoard::KeyBoard(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::KeyBoard)
    , m_type(Type_chinese)
{
    ui->setupUi(this);

    this->resize(parent->size().width(), parent->size().height() * 0.4);

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);

    //拼音最大长度
    // ui->lineEdit_textinput->setMaxLength(100);
    // ui->lineEdit_textinput->setEnabled(false);
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScroller::grabGesture(ui->scrollArea, QScroller::LeftMouseButtonGesture);
    engine = new RimeUtils(this);

    if (!engine->init(qApp->applicationDirPath() + "/rime-data", qApp->applicationDirPath() + "/rime-userdata")) {
        qWarning() << "Failed to initialize Rime engine!";
        // 处理初始化失败...
    }

    connect(engine, &RimeUtils::commited, this, [this](const QString& text) {
        if (this->m_edit && !text.isEmpty()) {
            // 将提交的文本作为字符串发送给目标控件
            QKeyEvent keyPress(QEvent::KeyPress, 0, Qt::NoModifier, text);
            QApplication::sendEvent(this->m_edit, &keyPress);
        }
        // 提交后清空本地输入缓存（如有必要）
        m_inputText.clear();
        ui->lineEdit_textinput->clear();
    });

    connect(engine, &RimeUtils::candidatesUpdated, this, [this] {
        // 更新预编辑标签
        ui->lineEdit_textinput->setText(engine->getPreedit());
        auto scrollBar = ui->scrollArea->horizontalScrollBar();
        scrollBar->setValue(scrollBar->maximum());

        QVector<QPair<int, QString>> candidateList;
        const auto& fontMetrics = QApplication::fontMetrics();
        for (const Candidate& cand : engine->getAllCandidates()) {
            if (fontMetrics.inFontUcs4(cand.text.toUcs4().first())) { // 过滤字体库不支持的候选词
                candidateList.append({ cand.rimeIndex, cand.text + (cand.comment.isEmpty() ? "" : " (" + cand.comment + ")") });
            }
        }

        // 更新候选词列表
        ui->listWidget->clear();
        ui->tableWidget->clear();
        if (candidateList.isEmpty()) {
            ui->textinputBtn->hide();
            ui->textinputBtn->setChecked(false);
            return;
        }

        ui->textinputBtn->show();

        ui->tableWidget->scrollToTop(); // 滚动到顶部

        for (const auto& item : candidateList) {
            QListWidgetItem* widgetItem = new QListWidgetItem(item.second);
            widgetItem->setData(Qt::UserRole, item.first);
            ui->listWidget->addItem(widgetItem);
        }
        //        ui->listWidget->addItems(candidateList);
        if (ui->listWidget->count() > 0) { // 滚动到顶部
            ui->listWidget->scrollToItem(ui->listWidget->item(0));
        }
        const int columnCount = 4;
        int size = candidateList.size();
        int rowCount = (size + columnCount - 1) / columnCount; // 向上取整

        QVector<QVector<QPair<int, QString>>> matrix(rowCount);

        for (int i = 0; i < size; ++i) {
            int row = i / columnCount;
            auto& item = candidateList.at(i);
            matrix[row].append(item);
        }

        ui->tableWidget->setRowCount(rowCount);
        ui->tableWidget->setColumnCount(columnCount);

        // 填充数据到网格
        for (int r = 0; r < rowCount; ++r) {
            int currentCols = matrix[r].size();
            for (int c = 0; c < currentCols; ++c) {
                auto& item = matrix[r][c];
                QTableWidgetItem* widgetItem = new QTableWidgetItem(item.second);
                widgetItem->setData(Qt::UserRole, item.first);
                widgetItem->setTextAlignment(Qt::AlignCenter); // item居中
                ui->tableWidget->setItem(r, c, widgetItem);
            }
        }
    });

    /* 设置为列表显示模式 */
    ui->listWidget->setViewMode(QListView::ListMode);
    /* 从左往右排列 */
    ui->listWidget->setFlow(QListView::LeftToRight);
    /* 屏蔽水平滑动条 */
    ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    /* 屏蔽垂直滑动条 */
    ui->listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    /* 设置为像素滚动 */
    ui->listWidget->setHorizontalScrollMode(QListWidget::ScrollPerPixel);
    /* 设置为单选 */
    ui->listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    /* 设置鼠标左键拖动 */
    QScroller::grabGesture(ui->listWidget, QScroller::LeftMouseButtonGesture);

    ui->pushButton_tab->hide();

    /* 屏蔽水平滑动条 */
    ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    /* 屏蔽垂直滑动条 */
    ui->tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QHeaderView* header = ui->tableWidget->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);

    // 隐藏表头
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setVisible(false);
    // 默认item高度为40
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(50);
    ui->tableWidget->setShowGrid(false);
    ui->tableWidget->setSelectionMode(QAbstractItemView::NoSelection); // 不可选中
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); // 不可编辑
    /* 设置为像素滚动 */
    ui->tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    /* 设置鼠标左键拖动 */
    QScroller::grabGesture(ui->tableWidget, QScroller::LeftMouseButtonGesture);

    keyBtnList.append(ui->pushButton_q);
    keyBtnList.append(ui->pushButton_w);
    keyBtnList.append(ui->pushButton_e);
    keyBtnList.append(ui->pushButton_r);
    keyBtnList.append(ui->pushButton_t);
    keyBtnList.append(ui->pushButton_y);
    keyBtnList.append(ui->pushButton_u);
    keyBtnList.append(ui->pushButton_i);
    keyBtnList.append(ui->pushButton_o);
    keyBtnList.append(ui->pushButton_p);
    keyBtnList.append(ui->pushButton_a);
    keyBtnList.append(ui->pushButton_s);
    keyBtnList.append(ui->pushButton_d);
    keyBtnList.append(ui->pushButton_f);
    keyBtnList.append(ui->pushButton_g);
    keyBtnList.append(ui->pushButton_h);
    keyBtnList.append(ui->pushButton_j);
    keyBtnList.append(ui->pushButton_k);
    keyBtnList.append(ui->pushButton_l);
    keyBtnList.append(ui->pushButton_z);
    keyBtnList.append(ui->pushButton_x);
    keyBtnList.append(ui->pushButton_c);
    keyBtnList.append(ui->pushButton_v);
    keyBtnList.append(ui->pushButton_b);
    keyBtnList.append(ui->pushButton_n);
    keyBtnList.append(ui->pushButton_m);
    keyBtnList.append(ui->pushButton_back);
    keyBtnList.append(ui->pushButton_space);
    keyBtnList.append(ui->pushButton_enter);
    keyBtnList.append(ui->pushButton_tab);

    //按键盘顺序的字符
    btnvalueList_small << "q"
                       << "w"
                       << "e"
                       << "r"
                       << "t"
                       << "y"
                       << "u"
                       << "i"
                       << "o"
                       << "p"
                       << "a"
                       << "s"
                       << "d"
                       << "f"
                       << "g"
                       << "h"
                       << "j"
                       << "k"
                       << "l"
                       << "z"
                       << "x"
                       << "c"
                       << "v"
                       << "b"
                       << "n"
                       << "m"
                       << ""
                       << " "
                       << "" << tr("下一步");
    btnvalueList_large << "Q"
                       << "W"
                       << "E"
                       << "R"
                       << "T"
                       << "Y"
                       << "U"
                       << "I"
                       << "O"
                       << "P"
                       << "A"
                       << "S"
                       << "D"
                       << "F"
                       << "G"
                       << "H"
                       << "J"
                       << "K"
                       << "L"
                       << "Z"
                       << "X"
                       << "C"
                       << "V"
                       << "B"
                       << "N"
                       << "M"
                       << ""
                       << " "
                       << "" << tr("下一步");
    btnvalueList_mark << "1"
                      << "2"
                      << "3"
                      << "4"
                      << "5"
                      << "6"
                      << "7"
                      << "8"
                      << "9"
                      << "0"
                      << "!"
                      << "@"
                      << "#"
                      << "%"
                      << "&&"
                      << "*"
                      << "("
                      << ")"
                      << "-"
                      << "_"
                      << ":"
                      << ";"
                      << "/"
                      << "."
                      << ","
                      << "?"
                      << ""
                      << " "
                      << "" << tr("下一步");

    //英文键盘顺序
    keyList_low.append(Qt::Key_Q);
    keyList_low.append(Qt::Key_W);
    keyList_low.append(Qt::Key_E);
    keyList_low.append(Qt::Key_R);
    keyList_low.append(Qt::Key_T);
    keyList_low.append(Qt::Key_Y);
    keyList_low.append(Qt::Key_U);
    keyList_low.append(Qt::Key_I);
    keyList_low.append(Qt::Key_O);
    keyList_low.append(Qt::Key_P);
    keyList_low.append(Qt::Key_A);
    keyList_low.append(Qt::Key_S);
    keyList_low.append(Qt::Key_D);
    keyList_low.append(Qt::Key_F);
    keyList_low.append(Qt::Key_G);
    keyList_low.append(Qt::Key_H);
    keyList_low.append(Qt::Key_J);
    keyList_low.append(Qt::Key_K);
    keyList_low.append(Qt::Key_L);
    keyList_low.append(Qt::Key_Z);
    keyList_low.append(Qt::Key_X);
    keyList_low.append(Qt::Key_C);
    keyList_low.append(Qt::Key_V);
    keyList_low.append(Qt::Key_B);
    keyList_low.append(Qt::Key_N);
    keyList_low.append(Qt::Key_M);
    keyList_low.append(Qt::Key_Backspace);
    keyList_low.append(Qt::Key_Space);
    keyList_low.append(Qt::Key_Enter);
    keyList_low.append(Qt::Key_Tab);

    // 数字键盘顺序
    keyList_mark.append(Qt::Key_1);
    keyList_mark.append(Qt::Key_2);
    keyList_mark.append(Qt::Key_3);
    keyList_mark.append(Qt::Key_4);
    keyList_mark.append(Qt::Key_5);
    keyList_mark.append(Qt::Key_6);
    keyList_mark.append(Qt::Key_7);
    keyList_mark.append(Qt::Key_8);
    keyList_mark.append(Qt::Key_9);
    keyList_mark.append(Qt::Key_0);
    keyList_mark.append(Qt::Key_Exclam);
    keyList_mark.append(Qt::Key_At);
    keyList_mark.append(Qt::Key_NumberSign);
    keyList_mark.append(Qt::Key_Percent);
    keyList_mark.append(Qt::Key_Ampersand);
    keyList_mark.append(Qt::Key_Asterisk);
    keyList_mark.append(Qt::Key_ParenLeft);
    keyList_mark.append(Qt::Key_ParenRight);
    keyList_mark.append(Qt::Key_Minus);
    keyList_mark.append(Qt::Key_Underscore);
    keyList_mark.append(Qt::Key_Colon);
    keyList_mark.append(Qt::Key_Semicolon);
    keyList_mark.append(Qt::Key_Slash);
    keyList_mark.append(Qt::Key_Period);
    keyList_mark.append(Qt::Key_Comma);
    keyList_mark.append(Qt::Key_Question);
    keyList_mark.append(Qt::Key_Backspace);
    keyList_mark.append(Qt::Key_Space);
    keyList_mark.append(Qt::Key_Enter);
    keyList_mark.append(Qt::Key_Tab);

    //大小写切换
    connect(ui->pushButton_shift, &QPushButton::clicked, [this](bool checked) {
        if (checked)
            switchState(Type_high);
        else {
            switchState(Type_Low);
        }
    });
    //中英文切换
    connect(ui->pushButton_en, &QPushButton::clicked, [this](bool checked) {
        if (checked) {
            switchState(Type_chinese);
        } else {
            switchState(Type_Low);
        }
    });
    //字符数字切换
    connect(ui->pushButton_123, &QPushButton::clicked, [this](bool checked) {
        if (checked) {
            switchState(Type_mark);
        } else {
            switchState(Type_Low);
        }
    });
    //中英文切换
    connect(ui->pushButton_en, &QPushButton::toggled, [this](bool checked) {
        if (checked) {
            ui->pushButton_en->setText(tr("中"));
        } else {
            ui->pushButton_en->setText(tr("En"));
        }
    });
    //字符数字切换
    connect(ui->pushButton_123, &QPushButton::toggled, [this](bool checked) {
        if (checked) {
            ui->pushButton_123->setText("?123");
        } else {
            ui->pushButton_123->setText("abc");
        }
    });
    //隐藏
    connect(ui->pushButton_hide, &QPushButton::clicked, [this] {
        if(this->m_edit){
            this->m_edit->clearFocus();
        }
        this->hide(); });
    //按钮绑定槽函数
    for (KeyButton* btn : keyBtnList) {
        connect(btn, &KeyButton::key_pressed, this, &KeyBoard::slot_key_keyEvent);
    }

    switchState(Type_high);
}

KeyBoard::~KeyBoard()
{
    delete ui;
}

void KeyBoard::hideEvent(QHideEvent* event)
{
    this->m_edit = nullptr;
    QWidget::hideEvent(event);
    ui->listWidget->clear();
    ui->tableWidget->clear();
    ui->lineEdit_textinput->clear();
    m_inputText.clear();
    emit keyBoardHidden();
}

void KeyBoard::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        // 计算鼠标全局坐标与窗口左上角的偏移（使用 geometry() 代替 frameGeometry()）
        m_dragPosition = event->globalPos() - geometry().topLeft();
        event->accept();
    }
}

void KeyBoard::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        QPoint newPos = event->globalPos() - m_dragPosition;

        // 获取屏幕可用区域（Qt5 使用 QDesktopWidget）

        QScreen* screen = QGuiApplication::screenAt(newPos);
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }
        QRect available = screen->availableGeometry(); // 工作区（不含任务栏）

        // 边界约束
        int maxX = available.right() - width() + 1;
        int maxY = available.bottom() - height() + 1;
        newPos.setX(qBound(available.left(), newPos.x(), maxX));
        newPos.setY(qBound(available.top(), newPos.y(), maxY));

        move(newPos);
        event->accept();
    }
}

void KeyBoard::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

void KeyBoard::setEdit(QWidget* edit)
{
    if (edit == this->m_edit) {
        return;
    }
    this->m_edit = edit;
}

void KeyBoard::slot_key_keyEvent(int key, QString value)
{
    if (m_type == Type_chinese && key != Qt::Key_Tab) {
        if (key == Qt::Key_Enter || key == Qt::Key_Space) {
            // 没有拼写内容,直接输入按键字符
            if (ui->lineEdit_textinput->text().isEmpty()) {
                if (this->m_edit) {
                    QKeyEvent keyPress(QEvent::KeyPress, key, Qt::NoModifier, value);
                    QApplication::sendEvent(this->m_edit, &keyPress);
                }
            } else { // 有拼写内容时，通过候选词列表提交
                if (key == Qt::Key_Enter) { // 回车：提交当前拼写原文字符串（英文）
                    if (this->m_edit) {
                        QKeyEvent keyPress(QEvent::KeyPress, 0, Qt::NoModifier, ui->lineEdit_textinput->text());
                        QApplication::sendEvent(this->m_edit, &keyPress);
                    }
                } else if (key == Qt::Key_Space) { // 空格：选择当前高亮候选词或第一个候选词
                    QListWidgetItem* item = ui->listWidget->currentItem();
                    if (item) {
                        ui->listWidget->itemClicked(item); // 会触发 selectCandidate -> commited 信号
                    } else { // 没有候选词，提交原始拼写（可选）
                        if (ui->listWidget->count() > 0) {
                            ui->listWidget->itemClicked(ui->listWidget->item(0));
                        } else {
                            QKeyEvent keyPress(QEvent::KeyPress, key, Qt::NoModifier, ui->lineEdit_textinput->text());
                            QApplication::sendEvent(this->m_edit, &keyPress);
                        }
                    }
                }
            }
            m_inputText.clear();
            updateInputText(m_inputText);
            ui->listWidget->clear();
            return;
        }

        if (!value.isEmpty()) {
            m_inputText.append(value);
            updateInputText(m_inputText);
        }

        if (key == Qt::Key_Backspace) {

            if (m_inputText.count() <= 1) {
                ui->listWidget->clear();
                m_inputText.clear();
                updateInputText(m_inputText);
                if (this->m_edit) {
                    QKeyEvent keyPress(QEvent::KeyPress, key, Qt::NoModifier, value);
                    QApplication::sendEvent(this->m_edit, &keyPress);
                }

            } else {
                if (engine->isComposing()) {
                    updateInputText(m_inputText);
                } else {
                    m_inputText = m_inputText.mid(0, m_inputText.count() - 1);
                    updateInputText(m_inputText);
                }
            }
        }
        return;
    }

    //&&->&
    if (value == "&&") {
        value = "&";
    }

    if (this->m_edit) {
        if (key == Qt::Key_Tab) {
            value = "";
        }
        QKeyEvent keyPress(QEvent::KeyPress, key, Qt::NoModifier, value);
        QApplication::sendEvent(this->m_edit, &keyPress);
    }
}

void KeyBoard::getCandidateList(const QString text)
{
    // 将当前输入的拼音字符串传给 Rime 引擎
    engine->processInputString(text);
    // 引擎处理完后会发出 stateUpdated 信号，自动更新 UI
}

void KeyBoard::on_listWidget_itemClicked(QListWidgetItem* item)
{

    // 选择对应序号的词
    engine->selectCandidate(item->data(Qt::UserRole).toInt());
}

void KeyBoard::on_tableWidget_cellClicked(int row, int column)
{

    // 选择对应序号的词
    engine->selectCandidate(ui->tableWidget->item(row, column)->data(Qt::UserRole).toInt());
}

void KeyBoard::switchState(TYPE type)
{
    ui->pushButton_shift->setChecked(false);
    ui->pushButton_en->setChecked(false);

    ui->pushButton_123->setChecked(true);
    switch (type) {
    case Type_Low: /*小写英文*/
        for (int i = 0; i < keyBtnList.count(); i++) {
            keyBtnList.at(i)->setValue(btnvalueList_small.at(i));
            keyBtnList.at(i)->setKey(keyList_low.at(i));
            m_type = Type_Low;
        }
        ui->pushButton_123->setChecked(false);
        ui->widget_chinese->hide();
        break;
    case Type_high: /*大写英文*/
        for (int i = 0; i < keyBtnList.count(); i++) {
            keyBtnList.at(i)->setValue(btnvalueList_large.at(i));
            keyBtnList.at(i)->setKey(keyList_low.at(i));
            m_type = Type_Low;
        }
        ui->pushButton_123->setChecked(false);
        ui->pushButton_shift->setChecked(true);
        ui->widget_chinese->hide();
        break;
    case Type_mark: /*数字字符*/
        for (int i = 0; i < keyBtnList.count(); i++) {
            keyBtnList.at(i)->setValue(btnvalueList_mark.at(i));
            keyBtnList.at(i)->setKey(keyList_mark.at(i));
            m_type = Type_mark;
        }

        ui->widget_chinese->hide();
        break;
    case Type_chinese: /*中文输入*/
        for (int i = 0; i < keyBtnList.count(); i++) {
            keyBtnList.at(i)->setValue(btnvalueList_small.at(i));
            keyBtnList.at(i)->setKey(keyList_low.at(i));
            m_type = Type_Low;
        }
        ui->pushButton_en->setChecked(true);
        ui->widget_chinese->show();
        m_inputText.clear();
        ui->lineEdit_textinput->clear();
        ui->listWidget->clear();
        ui->textinputBtn->hide();
        ui->textinputBtn->setChecked(false);
        break;
    }

    ui->stackedWidget->setCurrentIndex(0);

    m_type = type;
    return;
}

void KeyBoard::on_textinputBtn_toggled(bool checked)
{
    ui->stackedWidget->setCurrentIndex(checked ? 1 : 0);
}
