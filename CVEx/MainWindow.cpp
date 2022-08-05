#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent)
{
	auto widget = new QWidget;
	this->setCentralWidget(widget);
	auto layout = new QVBoxLayout(widget);
	this->staus_ = new QLabel;
	this->staus_->setText("Watch");
	auto font = this->staus_->font();
	font.setPointSize(10);
	this->staus_->setAlignment(Qt::AlignCenter);
	this->staus_->setFont(font);
	this->setStyleSheet("QLabel{background-color:blue;color:white;}");
	layout->addWidget(staus_);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	setWindowTitle("any one here");
	setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

	timer_ = new QTimer(this);
	timer_->setInterval(100);
	connect(timer_, &QTimer::timeout, this, &MainWindow::timeToCheck);
	timer_->start();

	capture_.open(0, cv::CAP_DSHOW);
	capture_.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
	capture_.set(cv::CAP_PROP_FRAME_HEIGHT, 768);
	rect = cv::Rect(0, 0, 240, 500);

	connect(this, &MainWindow::sig_status, this, &MainWindow::updateStatus);
	hidetimer_ = new QTimer(this);
	hidetimer_->setInterval(3000);
	connect(hidetimer_, &QTimer::timeout, this, [this]()
		{
			cv::destroyAllWindows();
			this->staus_->setText("Watch");
			this->staus_->setStyleSheet("QLabel{background-color:blue;color:white;}");
			//this->setFixedSize(100, 50);
			//this->move(1800, 30);
		});
	hidetimer_->setSingleShot(true);
	element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(9, 9));
	this->setFixedSize(50, 20);
	this->move(1800, 30);

}

MainWindow::~MainWindow()
{
}
cv::Mat removeLive(cv::Mat& img, cv::Mat& pattern, int method)
{
	cv::Mat aux;
	//if method is normalization
	if (method == 1)
	{
		cv::Mat img32, pattern32;
		img.convertTo(img32, CV_32F);
		pattern.convertTo(pattern32, CV_32F);
		aux = 1 - (img32 / pattern32);
		aux.convertTo(aux, CV_8U, 255);
	}
	else
	{
		aux = pattern - img;
	}
	return aux;
}

bool count(cv::Mat& img, int number)
{
	int c = 0;
	for (int i = 0; i < img.cols - 1; i++)
	{
		for (int j = 0; j < img.rows - 1; j++)
		{
			int level = (int)img.at<uchar>(j, i);
			if (level > 100)
			{
				c++;
			}
		}
	}
	//std::cout << "c: " <<c<< std::endl;
	if (c > number)
	{
		return true;
	}
	else
	{
		return false;
	}
}
void MainWindow::timeToCheck()
{
	//qDebug() << __func__;
	if (hidetimer_->isActive())
	{
		//qDebug()<<"is Show";
		pre.release();
		cur.release();
		mats.clear();
		return;
	}
	if (!capture_.isOpened())
	{
		return;
	}
	capture_ >> frame;
	roi = frame(rect).clone();
	cv::cvtColor(roi, roi, cv::COLOR_BGR2HSV);
	cv::split(roi, mats);
	if (pre.empty() && mats.size() == 3)
	{
		pre = mats.at(2).clone();
		return;
	}
	cur = mats.at(2);

	roi_ = removeLive(cur, pre, 0);
	pre = cur.clone();
	//cv::imshow("vv", roi_);
	threshold(roi_, roi_, 25, 255, cv::THRESH_BINARY);
	cv::erode(roi_, roi_, element);
	//cv::imshow("bin", roi_);
	bool m = count(roi_, 500);
	if (m)
	{
		//debugShow("object",roi);
		emit sig_status("fond");
		m=false;
	}
	//debugShow("roi", roi_);
}

void MainWindow::updateStatus(QString txt)
{
	if (txt == "fond");
	{
		//qDebug()<<"fond object";
		cv::waitKey(1);
		capture_ >> disp;
		auto path=QTime::currentTime().toString("hhmmss")+".jpg";
		//qDebug()<<path;
		cv::Mat tmp;
		cv::resize(disp(rect),tmp,cv::Size(120,250));
		cv::imshow("found",tmp);
		cv::moveWindow("found",0,0);
		//cv::imwrite(path.toStdString(),tmp);
		this->staus_->setText("Found");
		this->staus_->setStyleSheet("QLabel{background-color:black;color:red;}");
		//this->setFixedSize(190, 100);
		//this->move(0, 0);
		hidetimer_->start();
	}
}

void MainWindow::debugShow(QString name, cv::Mat mat)
{
#ifdef DEBUG
	cv::imshow(name.toStdString(), mat);
#endif

}
