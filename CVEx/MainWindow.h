#pragma once
#include <QtWidgets>
#include <opencv2/opencv.hpp>
class MainWindow :public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();
signals:
	void sig_status(QString txt);
private slots:
	void timeToCheck();
	void updateStatus(QString txt);
private:
	void debugShow(QString name, cv::Mat mat);
private:
	QLabel* staus_ = nullptr;
	cv::Mat frame;
	cv::Mat roi,roi_, pre, cur,disp,element;
	cv::VideoCapture capture_;
	QTimer* timer_;
	QTimer* hidetimer_;
	cv::Rect rect;
	std::vector<cv::Mat> mats;
	
};

