#include "smartlist.h"

/// To reduce linker pain, we anchor virtual here.

SmartListBase::SmartListBase(QObject* parent) : QObject(parent) { }

SmartListBase::~SmartListBase() = default;
