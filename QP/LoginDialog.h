#pragma once
#include <QLabel>
#include <QTimer>
#include <QToolButton>
#include <QNetworkReply>

#include "DownloadDialog.h"

class LoginDialog :public QDialog
{
	Q_OBJECT
public:
	explicit LoginDialog(QWidget* parent = nullptr);
	~LoginDialog();
	void setQrCode(const QString& content);
protected:
	void closeEvent(QCloseEvent* event) override;
private:
	QJsonValue getReplyData();
	void startGetLoginUrl();
private slots:
	void getLoginUrlFinished();
	void pollLoginInfo();
	void getLoginInfoFinished();
private:
	void qrCodeExpired();
	void showRefreshButton();
	void hideRefreshButton();
private:
	QString authKey;
	int polledTimes = 0;
	QTimer* pollTimer;
	QLabel* qrCodeLabel;
	QLabel* tipLabel;
	QToolButton* refreshButton;
	QNetworkReply* httpReply = nullptr;
};

