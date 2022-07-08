#pragma once
#include <QLabel>

class ElidedTextLabel:public  QLabel
{
	Q_OBJECT
public:
	ElidedTextLabel(QWidget *parent=nullptr);
	ElidedTextLabel(const QString &text,QWidget *parent=nullptr);

};

namespace utils
{
	QString fileExtension(const QString &fileName);
	int numberOfDigit(int num);
	QString paddedNum(int num,int width);
	QString legalizedFileName(QString title);
	std::tuple<int,int ,int> secs2HMS(int sec);
	QString secs2HmsStr(int secs);
	QString secs2HmsStrLocaleStr(int secs);
	QString formattedDataSize(qint64 bytes);
	
};

