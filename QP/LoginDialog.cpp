#include "LoginDialog.h"
#include "Network.h"
#include "QrCode.h"
#include <QtWidgets>
#include <QtNetwork>
#include <QFont>
#include <QDataStream>

static constexpr int QrCodeExpireTime = 180; // seconds
static constexpr int PollInterval = 2000; // ms
static constexpr int MaxPollTimes = (QrCodeExpireTime * 1000 / PollInterval) - 3;
static constexpr QColor QrCodeColor = QColor(251, 114, 153); // B站粉

static constexpr auto ScanToLoginTip = "请使用B站客户端<br>扫描二维码登录";

LoginDialog::LoginDialog(QWidget* parent) :QDialog(parent)
{
	auto mainLayout = new QVBoxLayout(this);
	qrCodeLabel = new QLabel(this);
	qrCodeLabel->setFixedSize(123, 123);
	qrCodeLabel->setAlignment(Qt::AlignCenter);
	tipLabel = new QLabel(ScanToLoginTip, this);
	tipLabel->setAlignment(Qt::AlignCenter);
	QFont font = tipLabel->font();
	font.setPointSize(11);
	tipLabel->setFont(font);

	refreshButton = new QToolButton(qrCodeLabel);
	refreshButton->setStyleSheet("background-color:white;border:2px solid #cccccc;");
	refreshButton->setCursor(Qt::PointingHandCursor);
	refreshButton->setIconSize(QSize(32, 32));
	refreshButton->setIcon(QIcon(":/icons/refresh.png"));
	refreshButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	refreshButton->setHidden(true);
	refreshButton->setText("点击刷新");


	connect(refreshButton, &QToolButton::clicked, this, [this]()
		{
			hideRefreshButton();
			startGetLoginUrl();
		});

	//mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->addWidget(qrCodeLabel, 0, Qt::AlignCenter);
	mainLayout->addWidget(tipLabel, 0, Qt::AlignCenter);

	setLayout(mainLayout);
	setWindowTitle("B站登录");

	pollTimer = new QTimer(this);
	pollTimer->setInterval(PollInterval);
	pollTimer->setSingleShot(true);
	connect(pollTimer, &QTimer::timeout, this, &LoginDialog::pollLoginInfo);



	startGetLoginUrl();



}

LoginDialog::~LoginDialog()
{
}

void LoginDialog::setQrCode(const QString& content)
{
	using namespace qrcodegen;
	QrCode qr = QrCode::encodeText(content.toUtf8(), QrCode::Ecc::MEDIUM);
	int n = qr.getSize();

	QPixmap pixmap(n * 3, n * 3);
	QPainter painter(&pixmap);
	QPen pen(QrCodeColor);

	pen.setWidth(3);
	painter.setPen(pen);

	pixmap.fill();
	for (int row = 0; row < n; row++)
	{
		for (int col = 0; col < n; col++)
		{
			auto val = qr.getModule(col, row);
			if (val)
			{
				painter.drawPoint(row * 3 + 1, col * 3 + 1);
			}

		}
	}
	qrCodeLabel->setFixedSize(pixmap.size());
	qrCodeLabel->setPixmap(pixmap);
}

void LoginDialog::closeEvent(QCloseEvent* event)
{
	if (httpReply != nullptr)
	{
		httpReply->abort();
		httpReply = nullptr;
	}
	QDialog::closeEvent(event);
}

QJsonValue LoginDialog::getReplyData()
{
	auto reply = httpReply;
	httpReply = nullptr;
	reply->deleteLater();
	if (reply->error() == QNetworkReply::OperationCanceledError)
	{
		return QJsonValue();
	}
	const auto [json, errorString] = Network::Bili::parseReply(reply, "data");
	if (!errorString.isNull())
	{
		tipLabel->setText(errorString);
		showRefreshButton();
		return QJsonValue();
	}
	return json["data"];
}

void LoginDialog::startGetLoginUrl()
{
	httpReply = Network::Bili::get("http://passport.bilibili.com/qrcode/getLoginUrl");
	connect(httpReply, &QNetworkReply::finished, this, &LoginDialog::getLoginUrlFinished);
}

void LoginDialog::getLoginUrlFinished()
{
	qDebug() << __func__;
	auto data = getReplyData().toObject();
	qDebug() << data;
	if (data.isEmpty())
	{
		return;
	}
	QString url = data["url"].toString();
	oauthKey = data["oauthKey"].toString();
	setQrCode(url);
	tipLabel->setText(ScanToLoginTip);
	polledTimes = 0;
	pollTimer->start();

}

void LoginDialog::pollLoginInfo()
{
	auto postData = QString("oauthKey=%1").arg(oauthKey).toUtf8();
	httpReply = Network::Bili::postUrlEncoded("https://passport.bilibili.com/qrcode/getLoginInfo", postData);

	connect(httpReply, &QNetworkReply::finished, this, &LoginDialog::getLoginInfoFinished);

}

void LoginDialog::getLoginInfoFinished()
{
	qDebug()<<__func__;
	polledTimes++;
	auto data=getReplyData();
	if (data.isNull()||data.isUndefined())
	{
		return;
	}
	bool isPollEnded=false;
	if (data.isDouble())
	{
		switch (data.toInt())
		{
		case -1://oauthkey is wrong,should never by this case
			QMessageBox::critical(this,"","oauthkey error");
			break;
		case  -2: //login url(qrcode) is expired
			isPollEnded=true;
			break;
		case  -4://qrcode not scanned
			break;
		case  -5://scanned but not confirmed
			tipLabel->setText("✅扫描成功<br>请在手机上确认");
			break;
		default:
				QMessageBox::warning(this,"Poll Warning",QString("unkown code: %1").arg(data.toInteger()));
		}
	}else
	{
		isPollEnded=true;
		accept();
	}
	if(!isPollEnded)
	{
		if (polledTimes==MaxPollTimes)
		{
			qrCodeExpired();
		}else
		{
			pollTimer->start();
		}
	}
}

void LoginDialog::qrCodeExpired()
{
	QPixmap pixmap=qrCodeLabel->pixmap();
	QPainter painter(&pixmap);
	painter.setPen(Qt::NoPen);
	painter.setBrush(QBrush(QColor(255,255,255,196)));
	painter.drawRect(QRect(QPoint(0,0),pixmap.size()));
	qrCodeLabel->setPixmap(pixmap);
	tipLabel->setText("二维码已失效");
	showRefreshButton();
}

void LoginDialog::showRefreshButton()
{
	refreshButton->setHidden(false);
	refreshButton->move(qrCodeLabel->rect().center() - refreshButton->rect().center());
}

void LoginDialog::hideRefreshButton()
{
}
