#pragma once

#include "smartlist.h"

#include <QObject>
#include <QPointer>

class SmartListConsumerBase : public QObject {
protected:
    QPointer<SmartListBase> m_source;

    void set_base_source(SmartListBase*);

protected slots:
    virtual void on_add(int begin, int end1);
    virtual void on_update(int begin, int end1);
    virtual void on_delete(int begin, int end1);

public:
    explicit SmartListConsumerBase(QObject* parent = nullptr);
    virtual ~SmartListConsumerBase();

private slots:
    void disconnect_all();
    void source_destroyed(QObject*);
};


template <class T>
class SmartListConsumer : public SmartListConsumerBase {
public:
    SmartListConsumer();
    ~SmartListConsumer() = default;

    void set_source(SmartList<T>* source) { set_base_source(source); }

protected slots:
};
