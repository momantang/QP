#pragma once
#include <QWidget>
#include <QToolBar>
#include <QActionGroup>
#include <QStackedWidget>

class MyTabWidget:public QWidget
{
public:
	MyTabWidget(QWidget *parent=nullptr);
	void setTabToolButtonStyle(Qt::ToolButtonStyle);
	void addTab(QWidget *page,const QIcon &icon,const QString &label);
private:
	QToolBar *bar;
	QActionGroup *actGroup;
	QStackedWidget *stack;
};

