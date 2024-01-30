#pragma once

#include <span>

#include <QDebug>
#include <QObject>

class SmartListBase : public QObject {
    Q_OBJECT
public:
    explicit SmartListBase(QObject* parent = nullptr);
    virtual ~SmartListBase();
signals:
    /// Report when items were added. Indicies are provided from beginning to
    /// end, exclusive
    void index_added(int begin, int end_1);
    /// Report when items were updated. Indicies are provided from beginning to
    /// end, exclusive
    void index_updated(int begin, int end_1);
    /// Report when items were removed. Indicies are provided from beginning to
    /// end, exclusive
    void index_deleted(int begin, int end_1);
};

///
/// \brief The SmartList class is a stripped down model class, which can notify
/// listeners on changes and updates
///
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

    ///
    /// Obtain the value at the list index
    ///
    T value(int idx) const { return m_storage.value(idx); }

    /// Set a value at the list index
    void set(int idx, T value) {
        m_storage[idx] = value;
        emit index_updated(idx, idx + 1);
    }

    /// Append to the list
    void append(T const& new_value) {
        qDebug() << Q_FUNC_INFO << m_storage.size();
        int starting_index = m_storage.size();
        int count          = 1;
        m_storage << new_value;
        emit index_added(starting_index, starting_index + count);
    }

    /// Append a list to this list
    void append(QList<T> new_values) {
        int starting_index = m_storage.size();
        int count          = new_values.size();
        m_storage << new_values;
        emit index_added(starting_index, starting_index + count);
    }

    /// Replace the contents of this list with a different list
    void replace(QList<T> new_values) {
        clear();
        append(new_values);
    }

    /// Remove at an index. Performance negative
    void remove(int idx, int count = 1) {
        m_storage.remove(idx, count);
        emit index_deleted(idx, idx + count);
    }

    /// Clear all values in this list
    void clear() {
        int count = m_storage.size();
        m_storage.clear();
        emit index_deleted(0, count);
    }

    /// Find and remove a single value that matches the given.
    ///
    /// \returns True if a value was removed. False otherwise
    bool find_remove_one(T const& item) {
        auto iter = std::find(m_storage.begin(), m_storage.end(), item);
        if (iter == m_storage.end()) return false;
        auto offset = std::distance(m_storage.begin(), iter);
        remove(offset);
        return true;
    }

    std::span<T const> range(int begin, int end1) const {
        if (end1 <= begin) return {};
        return { m_storage.data(), end1 - begin };
    }

    /// Append to this list
    SmartList& operator<<(T const& rhs) {
        append(rhs);
        return *this;
    }

    auto begin() const { return m_storage.begin(); }
    auto end() const { return m_storage.end(); }

    /// Obtain the size of this list
    auto size() const { return m_storage.size(); }
};

/// As smart lists are QObjects, it can be helpful to have a pointer typedef
template <class T>
using SmartListPtr = std::unique_ptr<SmartList<T>>;

template <class T>
SmartListPtr<T> make_smart_list() {
    return std::make_unique<SmartList<T>>();
}
