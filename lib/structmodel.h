#pragma once

#include <QAbstractTableModel>
#include <QDebug>

#include <span>

namespace struct_model_detail {

template <class Tuple, class Function>
auto tuple_for_each(Tuple&& t, Function&& f) {
    std::apply([&](auto&... x) { (..., f(x)); }, t);
}

template <class Tuple, int I, class Function>
struct _get_by_idx {
    static_assert(I >= 1);
    static void check(Tuple&& t, int i, Function&& f) {
        if (I == i) { return f(std::get<I>(t)); }

        _get_by_idx<Tuple, I - 1, Function>::check(t, i, std::move(f));
    }
};

template <class Tuple, class Function>
struct _get_by_idx<Tuple, 0, Function> {
    static void check(Tuple&& t, int i, Function&& f) {
        if (0 == i) { return f(std::get<0>(t)); }

        throw std::runtime_error("Bad index");
    }
};


template <class Tuple, class Function>
void tuple_get(Tuple&& t, int index, Function&& f) {
    using LT = std::remove_cvref_t<Tuple>;

    constexpr static auto S = std::tuple_size_v<LT>;

    _get_by_idx<Tuple, S - 1, Function>::check(t, index, std::move(f));
}


template <class Record>
QStringList get_header() {
    QStringList ret;
    tuple_for_each(Record::meta, [&ret](auto const& m) { ret << m.name; });
    return ret;
}

template <class Record>
QHash<int, QByteArray> const& get_name_map() {
    static QHash<int, QByteArray> ret = []() {
        QHash<int, QByteArray> build;

        int i = 0;

        tuple_for_each(Record::meta, [&build, &i](auto const& thing) {
            build[Qt::UserRole + i] = thing.name;
            i++;
        });
        return build;
    }();
    return ret;
}

template <class Record, class ReturnType>
constexpr int role_for_member(ReturnType Record::*ptr) {
    // Need to find a nice way to have a compile error on missing meta
    // this also does not work with custom meta
    int ret       = -1;
    using PtrType = decltype(ptr);
    int i         = 0;
    tuple_for_each(Record::meta, [&i, &ret, ptr](auto const& thing) {
        using LT = decltype(thing.access);

        if constexpr (std::is_same_v<LT, PtrType>) {
            if (thing.access == ptr) ret = Qt::UserRole + i;
        }
        i++;
    });

    Q_ASSERT(ret >= 0);

    return ret;
}


template <class T>
constexpr bool is_qobject = std::is_base_of_v<QObject, std::remove_cvref_t<T>>;

template <class T>
struct is_shared_qobject {
    static constexpr bool value = false;
};

template <class T>
struct is_shared_qobject<std::shared_ptr<T>> {
    static constexpr bool value = is_qobject<T>;
};

template <class Record>
QVariant record_runtime_get(Record const& r, int i) {
    QVariant ret;
    tuple_get(Record::meta, i, [&r, &ret](auto const& a) {
        using DCL = std::remove_cvref_t<decltype(a.get(r))>;
        if constexpr (is_shared_qobject<DCL>::value) {
            ret = QVariant::fromValue((a.get(r)).get());
        } else {
            ret = QVariant::fromValue(a.get(r));
        }
    });
    return ret;
}

template <class Record>
bool record_runtime_set(Record& r, int i, QVariant const& v) {
    bool ret = false;

    tuple_get(Record::meta, i, [&v, &ret, &r](auto& _value) {
        using DCL = std::remove_cvref_t<decltype(_value.get(r))>;
        if constexpr (is_shared_qobject<DCL>::value) {
            // editing a shared ptr is not supported at this time
            qWarning() << "Attempting to write to a shared pointer qobject";
            ret = false;
        } else {
            if (_value.editable) {
                ret = true;

                using LT = std::remove_cvref_t<decltype(_value.get(r))>;
                _value.set(r, v.value<LT>());
            }
        }
    });

    return ret;
}

} // namespace struct_model_detail

template <class H, class T>
struct MetaMember {
    T H::*      access;
    char const* name;
    bool        editable;

    constexpr MetaMember(T H::*      _access,
                         char const* _name,
                         bool        _editable = false)
        : access(_access), name(_name), editable(_editable) { }

    inline T const& get(H const& h) const { return h.*this->access; }
    inline void     set(H& h, T const& t) const { h.*this->access = t; }
};

template <class H, class T>
struct MetaCustom {
    using Getter = T (*)(H const&);
    using Setter = void (*)(H&, T&&);
    Getter      getter;
    Setter      setter;
    char const* name;
    bool        editable;

    constexpr MetaCustom(Getter      _getter,
                         Setter      _setter,
                         char const* _name,
                         bool        _editable = false)
        : getter(_getter), setter(_setter), name(_name), editable(_editable) { }

    inline T    get(H const& h) const { return std::invoke(getter, h); }
    inline void set(H& h, T&& t) const { std::invoke(setter, h, std::move(t)); }
};

#define SM_MAKE_META(...)                                                      \
    static constexpr inline auto meta = std::tuple(__VA_ARGS__)

#define SM_META(TYPE, NAME) MetaMember(&TYPE ::NAME, #NAME, true)

class StructTableModelBase : public QAbstractTableModel {
    Q_OBJECT
public:
    using QAbstractTableModel::QAbstractTableModel;

    // we used this for some virtual stuff. can still do so in future.
};

template <class Record>
class StructTableModel : public StructTableModelBase {
    QVector<Record> m_records;

    QStringList const m_header;

public:
    explicit StructTableModel(QObject* parent = nullptr)
        : StructTableModelBase(parent),
          m_header(struct_model_detail::get_header<Record>()) { }

    // Header:
    QVariant headerData(int             section,
                        Qt::Orientation orientation,
                        int             role = Qt::DisplayRole) const override {
        if (orientation != Qt::Orientation::Horizontal) return {};
        if (role != Qt::DisplayRole) return {};

        return m_header.value(section);
    }

    int rowCount(QModelIndex const& parent = QModelIndex()) const override {
        if (parent.isValid()) return 0;
        return m_records.size();
    }

    int columnCount(QModelIndex const& parent = QModelIndex()) const override {
        if (parent.isValid()) return 0;
        return m_header.size();
    }

    QVariant data(QModelIndex const& index,
                  int                role = Qt::DisplayRole) const override {

        // qDebug() << Q_FUNC_INFO << index << role;

        if (!index.isValid()) return {};
        if (index.row() >= m_records.size()) return {};

        auto const& item = m_records[index.row()];

        if (role == Qt::DisplayRole or role == Qt::EditRole) {
            return struct_model_detail::record_runtime_get(item,
                                                           index.column());
        }

        if (role >= Qt::UserRole) {
            auto local_role = role - Qt::UserRole;

            assert(local_role >= 0);

            if (local_role >= m_header.size()) return {};

            return struct_model_detail::record_runtime_get(item, local_role);
        }

        return {};
    }


    bool setData(QModelIndex const& index,
                 QVariant const&    value,
                 int                role = Qt::EditRole) override {

        // qDebug() << Q_FUNC_INFO << index << value << role;

        if (data(index, role) == value) return false;

        auto& item = m_records[index.row()];

        int location = -1;

        if (role >= Qt::UserRole) {
            location = role - Qt::UserRole;
        } else {
            location = index.column();
        }

        if (location >= m_header.size()) return false;

        bool ok =
            struct_model_detail::record_runtime_set(item, location, value);

        if (!ok) return false;

        emit dataChanged(index, index, QList<int>() << role);
        return true;
    }

    Qt::ItemFlags flags(QModelIndex const& index) const override {
        if (!index.isValid()) return Qt::NoItemFlags;

        bool can_edit = false;

        struct_model_detail::tuple_get(
            Record::meta, index.column(), [&can_edit](auto const& v) {
                can_edit = v.editable;
            });

        if (!can_edit) return Qt::ItemIsEnabled;

        return Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    QHash<int, QByteArray> roleNames() const override {
        static const auto roles = struct_model_detail::get_name_map<Record>();

        return roles;
    }

    bool insertRows(int                row,
                    int                count,
                    QModelIndex const& p = QModelIndex()) override {
        if (row < 0 or count <= 0) return false;

        beginInsertRows(p, row, row + count - 1);
        m_records.insert(row, count, Record {});
        endInsertRows();
        return true;
    }

    bool removeRows(int                row,
                    int                count,
                    QModelIndex const& p = QModelIndex()) override {
        if (row < 0 or count <= 0) return false;
        if (count > m_records.size()) return false;

        beginRemoveRows(p, row, row + count - 1);
        m_records.remove(row, count);
        endRemoveRows();
        return true;
    }

    void reset(QList<Record> new_records = {}) {
        // qDebug() << Q_FUNC_INFO;
        beginResetModel();
        m_records = new_records;
        endResetModel();
    }

    // this emits a remove signal, instead of a reset
    void remove_all() {
        // qDebug() << Q_FUNC_INFO;
        if (m_records.isEmpty()) return;
        beginRemoveRows(QModelIndex(), 0, std::max(rowCount() - 1, 0));
        m_records.clear();
        endRemoveRows();
    }

    Record const* get_at(int i) const {
        if (i < 0) return nullptr;
        if (i >= m_records.size()) return nullptr;
        return &m_records[i];
    }

    auto append(Record const& r) {
        int rc = rowCount();
        beginInsertRows({}, rc, rc);
        m_records << r;
        endInsertRows();
    }

    auto append(QVector<Record> r) {
        if (r.isEmpty()) return;
        int rc = rowCount();
        beginInsertRows({}, rc, rc + r.size() - 1);
        m_records << r;
        endInsertRows();
    }

    auto replace(QVector<Record> r = {}) {
        remove_all();
        append(r);
    }

    auto update(int i, Record const& r) {
        // qDebug() << Q_FUNC_INFO;

        if (i < 0) return;
        if (i >= m_records.size()) return;

        m_records[i] = r;

        auto left  = index(i, 0);
        auto right = index(i, columnCount() - 1);

        emit dataChanged(left, right);
    }

    void remove_at(int index, int count = 1) {
        if (index < 0) return;
        if (index >= m_records.size()) return;

        beginRemoveRows(QModelIndex(), index, index + count - 1);
        m_records.remove(index, count);
        endRemoveRows();
    }

    void insert_at(int index, std::span<Record> records) {
        qDebug() << Q_FUNC_INFO << index << (m_records.size());
        if (records.empty()) return;
        beginInsertRows({}, index, index + records.size() - 1);
        m_records.insert(index, records.size(), Record {});
        for (int i = 0; i < records.size(); i++) {
            m_records[index + i] = records[i];
        }
        endInsertRows();
    }


    // delete by a predicate
    template <class Function>
    void remove_by_predicate(Function&& f) {
        QVector<int> to_remove;

        for (int i = 0; i < m_records.size(); i++) {
            if (f(m_records[i])) to_remove << i;
        }

        // sort to high -> low, so indices are preserved
        std::reverse(to_remove.begin(), to_remove.end());

        for (auto i : to_remove) {
            remove_at(i);
        }
    }

    auto const& vector() const { return m_records; }

    auto begin() const { return m_records.begin(); }
    auto end() const { return m_records.end(); }

    auto cbegin() const { return m_records.begin(); }
    auto cend() const { return m_records.end(); }
};
