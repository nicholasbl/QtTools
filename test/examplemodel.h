#pragma once

#include "lib/structmodel.h"

struct ExampleModelRecord {
    QString name;
    int     value  = 1;
    float   thingy = 43.0;

    SM_MAKE_META(SM_META(ExampleModelRecord, name),
                 SM_META(ExampleModelRecord, value),
                 SM_META(ExampleModelRecord, thingy));
};

class ExampleModel : public StructTableModel<ExampleModelRecord> {
public:
    ExampleModel();
};
