#pragma once

#include <QDebug>
#include <QObject>

class SmartListBase : public QObject {
    Q_OBJECT
public:
    explicit SmartListBase(QObject* parent = nullptr);
    virtual ~SmartListBase();
signals:
    void index_added(int begin, int end_1);
    void index_updated(int begin, int end_1);
    void index_deleted(int begin, int end_1);
};

template <class T>
class SmartList : public SmartListBase {
    QList<T> m_storage;

public:
    explicit SmartList(QObject* parent = nullptr) : SmartListBase(parent) { }

    template <class Iterator>
    SmartList(Iterator begin, Iterator end, QObject* parent = nullptr)
        : SmartListBase(parent), m_storage(begin, end) {
        emit index_added(0, m_storage.size());
    }

    ~SmartList() { clear(); }

    T value(int idx) const { return m_storage.value(idx); }

    void set(int idx, T value) {
        m_storage[idx] = value;
        emit index_updated(idx, idx + 1);
    }

    void append(T const& new_value) {
        qDebug() << Q_FUNC_INFO << m_storage.size();
        int starting_index = m_storage.size();
        int count          = 1;
        m_storage << new_value;
        emit index_added(starting_index, starting_index + count);
    }

    void append(QList<T> new_values) {
        int starting_index = m_storage.size();
        int count          = new_values.size();
        m_storage << new_values;
        emit index_added(starting_index, starting_index + count);
    }

    void replace(QList<T> new_values) {
        clear();
        append(new_values);
    }

    void remove(int idx, int count = 1) {
        m_storage.remove(idx, count);
        emit index_deleted(idx, idx + count);
    }

    void clear() {
        int count = m_storage.size();
        m_storage.clear();
        emit index_deleted(0, count);
    }

    bool find_remove_one(T const& item) {
        auto iter = std::find(m_storage.begin(), m_storage.end(), item);
        if (iter == m_storage.end()) return false;
        auto offset = std::distance(m_storage.begin(), iter);
        remove(offset);
        return true;
    }

    SmartList& operator<<(T const& rhs) {
        append(rhs);
        return *this;
    }

    auto begin() const { return m_storage.begin(); }
    auto end() const { return m_storage.end(); }

    auto size() const { return m_storage.size(); }

    template <class Archive>
    void serialize(Archive& ar) {
        if constexpr (Archive::is_read) { clear(); }

        ar("storage", m_storage);

        if constexpr (Archive::is_read) {
            if (m_storage.size()) { emit index_added(0, m_storage.size()); }
        }
    }
};

template <class T>
using SmartListPtr = std::unique_ptr<SmartList<T>>;

template <class T>
SmartListPtr<T> make_smart_list() {
    return std::make_unique<SmartList<T>>();
}
