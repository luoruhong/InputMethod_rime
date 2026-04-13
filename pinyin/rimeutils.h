#ifndef RIMEUTILS_H
#define RIMEUTILS_H

#include <QObject>
#include <rime_api.h>

class Candidate {
public:
    QString text; // 候选词
    QString comment; // 注音
    int rimeIndex; // 索引
};

class RimeUtils : public QObject {
    Q_OBJECT
public:
    explicit RimeUtils(QObject* parent = nullptr);
    ~RimeUtils();

    /**
     * @brief 初始化
     * @param 公共配置目录
     * @param 用户配置目录
     * @param 输入方案(可调用getSchemaList()获取)
     * @return
     */
    bool init(const QString sharedDir,
        const QString userDir,
        const QString schema = "luna_pinyin");

    /**
     * @brief 输入拼音
     * @param text
     * @return
     */
    bool processInputString(const QString& text);

    /**
     * @brief 选择候选词
     * @param index
     * @return
     */
    bool selectCandidate(int index);

    /**
     * @brief 获取预编辑文本
     * @return
     */
    QString getPreedit() const;

    /**
     * @brief 获取所有候选词列表
     * @return
     */
    std::vector<Candidate> getAllCandidates() const;

    /**
     * @brief 获取输入方案列表
     * @return
     */
    QStringList getSchemaList();

    /**
     * @brief 判断是否正在组词中
     * @return
     */
    bool isComposing() const;

signals:
    void candidatesUpdated(); // 候选词更新
    void commited(const QString& text); // 提交候选词

private:
    /**
    * @brief 检查并提交候选词
    * @return
    */ 
    void checkAndEmitCommit();

private:
    RimeApi* m_rime = nullptr;
    RimeSessionId m_session_id = 0;

    QString m_preedit;
    QStringList m_candidates;
};

#endif // RIMEUTILS_H
