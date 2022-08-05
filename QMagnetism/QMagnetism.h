#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QMagnetism.h"

class QMagnetism : public QMainWindow
{
    Q_OBJECT

public:
    QMagnetism(QWidget *parent = nullptr);
    ~QMagnetism();

private:
    Ui::QMagnetismClass ui;
};
