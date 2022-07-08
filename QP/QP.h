#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QP.h"

class QP : public QMainWindow
{
    Q_OBJECT

public:
    QP(QWidget *parent = nullptr);
    ~QP();

private:
    Ui::QPClass ui;
};
