#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_CVEx.h"

class CVEx : public QMainWindow
{
    Q_OBJECT

public:
    CVEx(QWidget *parent = nullptr);
    ~CVEx();

private:
    Ui::CVExClass ui;
};
