#include "MainWindow.h"
#include "utils.h"
#include "Network.h"
#include "Settings.h"
#include "TaskTable.h"
#include "MyTabWidget.h"
#include "AboutWidget.h"
#include "LoginDialog.h"
#include <QApplication>
#include <QtWidgets>
#include <QtNetwork>

#include "utils.h"

#define APP_VERSION "0.1"
static constexpr  int GetUserInfoRetryInterval = 10000;//ms
static constexpr  int GetUserInfoTimeout = 10000;//ms

MainWindow::MainWindow(QWidget* parent) :QMainWindow(parent)
{
#ifdef APP_VERSION
	QApplication::setApplicationVersion(APP_VERSION);
#endif
	Network::accessManager()->setCookieJar(Settings::inst()->getCookieJar());
	this->setWindowTitle("B23Downloader");
	setCentralWidget(new QWidget);
	auto mainLaout = new QVBoxLayout(centralWidget());
	auto topLayout = new QHBoxLayout;

	//set up user infor widgets
	ufaceButton = new QToolButton;
	ufaceButton->setText("登录");
	ufaceButton->setFixedSize(32, 32);
	ufaceButton->setIconSize(QSize(32, 32));
	ufaceButton->setCursor(Qt::PointingHandCursor);

	QFont loginTextFont = font();
	loginTextFont.setBold(true);
	loginTextFont.setPointSize(font().pointSize() + 1);
	ufaceButton->setFont(loginTextFont);
	ufaceButton->setPopupMode(QToolButton::InstantPopup);
	ufaceButton->setStyleSheet(R"(
        QToolButton {
            color: #00a1d6;
            background-color: white;
            border: none;
        }
        QToolButton::menu-indicator { image: none; }
)");
	connect(ufaceButton, &QToolButton::clicked, this, &MainWindow::ufaceButtonClicked);

	unameLabel = new ElidedTextLabel;
	unameLabel->setHintWidthToString("晚安玛卡巴卡！やさしい夢見てね");
	topLayout->addWidget(ufaceButton);
	topLayout->addWidget(unameLabel, 1);

	//set up download url lineEdit
	auto downloadUrlLayout = new QHBoxLayout;
	downloadUrlLayout->setSpacing(0);
	urlLineEdit = new QLineEdit;
	urlLineEdit->setFixedHeight(32);
	urlLineEdit->setClearButtonEnabled(true);
	urlLineEdit->setPlaceholderText("bilibili 直播/视频/漫画 URL");

	auto downloadButton = new QPushButton;
	downloadButton->setToolTip("下载");
	downloadButton->setFixedSize(QSize(32, 32));
	downloadButton->setIconSize(QSize(28, 28));
	downloadButton->setIcon(QIcon(":/icons/download.svg"));
	downloadButton->setCursor(Qt::PointingHandCursor);
	downloadButton->setStyleSheet(
		"QPushButton{border:1px solid gray; border-left:0px; background-color:white;}"
		"QPushButton:hover{background-color:rgb(229,229,229);}"
		"QPushButton:pressed{background-color:rgb(204,204,204);}"
	);
	connect(urlLineEdit, &QLineEdit::returnPressed, this, &MainWindow::downloadButtonClicked);
	connect(downloadButton, &QPushButton::clicked, this, &MainWindow::downloadButtonClicked);
	downloadUrlLayout->addWidget(urlLineEdit, 1);
	downloadUrlLayout->addWidget(downloadButton);
	topLayout->addLayout(downloadUrlLayout, 2);
	mainLaout->addLayout(topLayout);

	taskTable = new TaskTableWidget;
	//QTimer::singleShot(0, this, [this] {taskTable->load(); });
	auto tabs = new MyTabWidget;
	tabs->addTab(taskTable, QIcon(":/icons/download.svg"), "正在下载");
	tabs->addTab(new AboutWidget, QIcon(":/icons/about.svg"), "关于");
	mainLaout->addWidget(tabs);

	setStyleSheet("QMainWindow{backgroud-color:white;}QTableWidget{border:none;}");
	setMinimumSize(650, 360);
	QTimer::singleShot(0, this, [this] {resize(minimumSize()); });

	urlLineEdit->setFocus();
	urlLineEdit->setText("https://www.bilibili.com/bangumi/play/ep480428?from_spmid=666.25.episode.0");
	startGetUserInfo();

}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	auto dlg = QMessageBox(QMessageBox::Warning, "退出", "是否退出？", QMessageBox::NoButton, this);
	dlg.addButton("确定", QMessageBox::AcceptRole);
	dlg.addButton("取消", QMessageBox::RejectRole);
	auto ret = dlg.exec();
	if (ret == QMessageBox::AcceptRole)
	{
		taskTable->stopAll();
		taskTable->save();
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void MainWindow::startGetUserInfo()
{
	if (!Settings::inst()->hasCookies())
	{
		return;
	}
	if (hasGotUInfo || uinfoReply != nullptr)
	{
		return;
	}
	unameLabel->setText("登录中...", Qt::gray);
	auto rqst = Network::Bili::Request(QUrl("https://api.bilibili.com/nav"));
	rqst.setTransferTimeout(GetUserInfoTimeout);
	uinfoReply = Network::accessManager()->get(rqst);
	connect(uinfoReply, &QNetworkReply::finished, this, &MainWindow::getUserInfoFinished);
}

void MainWindow::startGetUFace()
{
	qDebug() << __func__;
	if (ufaceUrl.isNull())
	{
		qDebug() << "uface Url is Null";
		return;
	}
	if (hasGotUface || uinfoReply != nullptr)
	{
		return;;
	}
	auto rqst = Network::Bili::Request(ufaceUrl);
	rqst.setTransferTimeout(GetUserInfoTimeout);
	uinfoReply = Network::accessManager()->get(rqst);
	connect(uinfoReply, &QNetworkReply::finished, this, &MainWindow::getUfaceFinsished);
}

void MainWindow::downloadButtonClicked()
{
	auto trimmed=urlLineEdit->text().trimmed();
	if (trimmed.isEmpty())
	{
		urlLineEdit->clear();
		return;
	}
	auto dlg=new DownloadDialog(trimmed,this);
	connect(dlg,&QDialog::finished,this,[this,dlg](int result)
	{
		dlg->deleteLater();
		if (result==QDialog::Accepted)
		{
			qDebug()<<__func__<<" unfinished";
			//taskTable->addTasks(dlg.getDownloadTasks());
		}
	});
}

void MainWindow::getUserInfoFinished()
{
	qDebug() << __func__;
	auto reply = uinfoReply;
	uinfoReply->deleteLater();
	uinfoReply = nullptr;

	if (reply->error() == QNetworkReply::OperationCanceledError)
	{
		unameLabel->setErrText("网络请求超市");
		QTimer::singleShot(GetUserInfoRetryInterval, this, &MainWindow::startGetUserInfo);
		return;
	}
	const auto [json, errorString] = Network::Bili::parseReply(reply, "data");
	if (!json.empty() && !errorString.isNull())
	{
		unameLabel->clear();
		Settings::inst()->removeCookies();
	}
	else if (!errorString.isNull())
	{
		unameLabel->setErrText(errorString);
		QTimer::singleShot(GetUserInfoRetryInterval, this, &MainWindow::startGetUserInfo);
	}
	else
	{
		//success
		hasGotUInfo = true;
		auto data = json["data"];
		auto uname = data["uname"].toString();
		ufaceUrl = data["face"].toString() + "@64w_64h.png";
		if (data["vipStatus"].toInt())
		{
			unameLabel->setText(uname, B23Style::Pink);
		}
		else
		{
			unameLabel->setText(uname);
		}

		auto logoutAction = new QAction(QIcon(":/icons/logout.svg"), "退出");
		ufaceButton->addAction(logoutAction);
		ufaceButton->setIcon(QIcon(":/icons/akkarin.png"));
		connect(logoutAction, &QAction::triggered, this, &MainWindow::logoutActionTriggered);

		startGetUFace();
	}
}

void MainWindow::getUfaceFinsished()
{
	auto reply = uinfoReply;
	uinfoReply->deleteLater();
	uinfoReply = nullptr;

	if (!hasGotUInfo && reply->error() == QNetworkReply::OperationCanceledError)
	{
		return;;
	}
	if (reply->error() != QNetworkReply::NoError)
	{
		QTimer::singleShot(GetUserInfoRetryInterval, this, &MainWindow::startGetUFace);
		return;
	}
	hasGotUface = true;
	QPixmap pixmap;
	pixmap.loadFromData(reply->readAll());
	ufaceButton->setIcon(QIcon(pixmap));
}
// login
void MainWindow::ufaceButtonClicked()
{
	auto settings = Settings::inst();
	if (hasGotUInfo)
	{
		return;
	}
	else if (settings->hasCookies())
	{
		if (uinfoReply != nullptr)
		{
			return;
		}
		startGetUserInfo();
	}
	else
	{
		settings->getCookieJar()->clear();
		auto dlg = new LoginDialog(this);
		connect(dlg, &QDialog::finished, this, [=](int result)
			{
				dlg->deleteLater();
				if (result == QDialog::Accepted)
				{
					settings->saveCookies();
					startGetUserInfo();
				}
				else
				{
					settings->getCookieJar()->clear();
				}
			});
		dlg->open();
	}
}

void MainWindow::logoutActionTriggered()
{
	hasGotUface = false;
	hasGotUInfo = false;
	ufaceUrl.clear();

	unameLabel->clear();
	ufaceButton->setIcon(QIcon());

	auto actions = ufaceButton->actions();
	if (!actions.isEmpty())
	{
		ufaceButton->removeAction(actions.first());
	}
	if (uinfoReply != nullptr)
	{
		uinfoReply->abort();
	}

	auto settings = Settings::inst();
	auto exitPostData = "biliCSRF=" + settings->getCookieJar()->getCookie("bili_jct");
	auto exitReply = Network::Bili::postUrlEncoded("https://passport.bilibili.com/login/exit/v2", exitPostData);
	connect(exitReply, &QNetworkReply::finished, this, [=] {exitReply->deleteLater(); });
	settings->removeCookies();
}
