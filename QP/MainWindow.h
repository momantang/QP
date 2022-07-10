#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QNetworkReply>

class ElidedTextLabel;
class TaskTableWidget;

class MainWindow:public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent=nullptr);
	~MainWindow();
};

