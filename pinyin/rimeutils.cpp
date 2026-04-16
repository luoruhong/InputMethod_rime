#include "rimeutils.h"
#include <QDebug>
#include <QDir>

RimeUtils::RimeUtils(QObject* parent)
    : QObject(parent)
    , m_rime(nullptr)
    , m_session_id(0)
{
}

RimeUtils::~RimeUtils()
{
    if (m_rime && m_session_id) {
        m_rime->destroy_session(m_session_id);
        m_rime->finalize();
    }
}

bool RimeUtils::init(const QString sharedDir, const QString userDir, const QString schema)
{
    // 1. 获取 RimeApi 入口
    m_rime = rime_get_api();
    if (!m_rime) {
        qWarning() << "Failed to get RimeApi!";
        return false;
    }

    // 2. 设置Rime配置参数(宏定义)
    RIME_STRUCT(RimeTraits, traits);
    traits.app_name = "my_ime";
    // 设置Rime数据目录，请根据实际情况调整
    auto sharedData_ = sharedDir.toUtf8();
    traits.shared_data_dir = sharedData_.constData();
    auto userDir_ = userDir.toUtf8();
    traits.user_data_dir = userDir_.constData();

    // 3. 初始化Rime
    m_rime->setup(&traits);
    m_rime->initialize(nullptr);
    // 4. 检查是否初始化成功并创建会话
    if (m_rime->start_maintenance(true)) {
        m_rime->join_maintenance_thread();
    }
    m_session_id = m_rime->create_session();
    if (!m_session_id) {
        qWarning() << "Failed to create Rime session!";
        return false;
    }

    auto schemaList = getSchemaList();
    if (schemaList.contains(schema)) { // 判断是否存在输入方案,选中输入方案(默认为default.yaml配置的第一个词库)
        auto schema_ = schema.toUtf8();
        m_rime->select_schema(m_session_id, schema_.constData());
    }

    m_rime->set_option(m_session_id, "simplification", True); // 设置简体(有些输入方案默认有可能是繁体,如luna-pinyin)

    bool flag = (m_rime->get_option(m_session_id, "simplification") == True);
#ifdef QT_DEBUG
    qDebug() << QString("Switch simplified Chinese : %1").arg(flag ? "successfully" : "Failed");

    qDebug() << "RimeEngine initialized successfully!";
#endif
    return true;
}

QString RimeUtils::getPreedit() const
{
    if (!m_rime || !m_session_id)
        return QString();

    RIME_STRUCT(RimeContext, context);
    if (!m_rime->get_context(m_session_id, &context))
        return QString();

    QString preedit = QString::fromUtf8(context.composition.preedit);
    m_rime->free_context(&context);
    return preedit;
}

std::vector<Candidate> RimeUtils::getAllCandidates() const
{
    std::vector<Candidate> allCandidates;
    if (!m_rime || !m_session_id)
        return allCandidates;

    // 1. 初始化上下文
    RIME_STRUCT(RimeContext, context);

    const int kMaxCandidates = 1024; // 数量上限
    int index = 0; // 候选词序号

    // 2.迭代器获取所有候选词
    RimeCandidateListIterator iter = { 0 };
    if (m_rime->candidate_list_begin(m_session_id, &iter)) {
        while ((kMaxCandidates > index) && m_rime->candidate_list_next(&iter)) {
            Candidate cand;
            cand.text = QString::fromUtf8(iter.candidate.text);
            cand.comment = QString::fromUtf8(iter.candidate.comment);
            cand.rimeIndex = iter.index;
            allCandidates.push_back(cand);
            index++;
        }
        m_rime->candidate_list_end(&iter);
    }

    return allCandidates;
}

bool RimeUtils::selectCandidate(int index)
{
    if (!m_rime || !m_session_id) {
        return false;
    }
    if (m_rime->select_candidate(m_session_id, index) != True) {
        return false;
    }
    // 选择候选词后可能产生提交
    checkAndEmitCommit();
    // 同时更新候选词界面（因为提交后候选窗会关闭）
    emit candidatesUpdated();
    return true;
}

QStringList RimeUtils::getSchemaList()
{
    QStringList schema_ids;
    QStringList schema_names;
    RimeSchemaList schema_list;
    if (!m_rime->get_schema_list(&schema_list)) {
        qWarning() << "Failed to get schema list";
        return {};
    }

    for (size_t i = 0; i < schema_list.size; ++i) {
        schema_ids.append(QString::fromUtf8(schema_list.list[i].schema_id));
        schema_names.append(QString::fromUtf8(schema_list.list[i].name));
    }

    /// 官方输入方案
    /// luna_pinyin
    /// luna_pinyin_simp
    /// luna_pinyin_fluency
    /// bopomofo
    /// bopomofo_tw
    /// cangjie5
    /// stroke
    /// terra_pinyin

    m_rime->free_schema_list(&schema_list);
    return schema_ids;
}

bool RimeUtils::isComposing() const
{
    if (!m_rime || !m_session_id)
        return false;

    RIME_STRUCT(RimeContext, context);
    if (!m_rime->get_context(m_session_id, &context))
        return false;

    bool composing = (context.composition.sel_start != 0); // 光标位置不等于0为组词中
    m_rime->free_context(&context);
    return composing;
}

void RimeUtils::checkAndEmitCommit()
{
    if (!m_rime || !m_session_id)
        return;

    RIME_STRUCT(RimeCommit, commit);
    if (m_rime->get_commit(m_session_id, &commit)) { // 判断是否组词完毕
        QString text = QString::fromUtf8(commit.text);
        if (!text.isEmpty()) {
            emit commited(text);
        }
        m_rime->free_commit(&commit);
    }
}

bool RimeUtils::processInputString(const QString& text)
{
    if (!m_rime || !m_session_id) {
        return false;
    }

    // 1. 清空当前未提交的编码（模拟按下 Escape 键）
    m_rime->clear_composition(m_session_id);

    // 2. 将字符串通过 simulate_key_sequence 喂给 Rime
    //    这个函数会像真实按键一样逐个处理字符
    auto text_ = text.toUtf8();
    if (!m_rime->set_input(m_session_id, text_.constData())) {
        return false;
    }
    emit candidatesUpdated();

    return true;
}
