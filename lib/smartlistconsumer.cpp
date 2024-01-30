#include "smartlistconsumer.h"

SmartListConsumerBase::SmartListConsumerBase(QObject* parent)
    : QObject(parent) { }

SmartListConsumerBase::~SmartListConsumerBase() = default;

void SmartListConsumerBase::on_add(int /*begin*/, int /*end1*/) { }
void SmartListConsumerBase::on_update(int /*begin*/, int /*end1*/) { }
void SmartListConsumerBase::on_delete(int /*begin*/, int /*end1*/) { }

void SmartListConsumerBase::set_base_source(SmartListBase* p) {
    if (m_source) {
        disconnect(m_source,
                   &SmartListBase::index_added,
                   this,
                   &SmartListConsumerBase::on_add);
        disconnect(m_source,
                   &SmartListBase::index_updated,
                   this,
                   &SmartListConsumerBase::on_update);
        disconnect(m_source,
                   &SmartListBase::index_deleted,
                   this,
                   &SmartListConsumerBase::on_delete);

        disconnect(m_source,
                   &QObject::destroyed,
                   this,
                   &SmartListConsumerBase::source_destroyed);
    }

    m_source = p;

    if (m_source) {
        connect(m_source,
                &SmartListBase::index_added,
                this,
                &SmartListConsumerBase::on_add);
        connect(m_source,
                &SmartListBase::index_updated,
                this,
                &SmartListConsumerBase::on_update);
        connect(m_source,
                &SmartListBase::index_deleted,
                this,
                &SmartListConsumerBase::on_delete);

        connect(m_source,
                &QObject::destroyed,
                this,
                &SmartListConsumerBase::source_destroyed);
    }
}

void SmartListConsumerBase::source_destroyed(QObject*) {
    set_base_source(nullptr);
}
