#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QNetworkReply>

class ElidedTextLabel;
class TaskTableWidget;

class MainWindow :public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();
protected:
	void closeEvent(QCloseEvent* event) override;
private:
	void startGetUserInfo();
	void startGetUFace();
private slots:
	void downloadButtonClicked();
	void getUserInfoFinished();
	void getUfaceFinsished();
	void ufaceButtonClicked();
	void logoutActionTriggered();
private:
	bool hasGotUInfo = false;
	bool hasGotUface = false;
	QNetworkReply* uinfoReply = nullptr;
	QString ufaceUrl;

	QToolButton* ufaceButton;
	ElidedTextLabel* unameLabel;
	QLineEdit* urlLineEdit;
	TaskTableWidget* taskTable;
};

