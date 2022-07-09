#pragma once
#include <QLabel>

class ElidedTextLabel :public  QLabel
{
	Q_OBJECT
public:
	ElidedTextLabel(QWidget* parent = nullptr);
	ElidedTextLabel(const QString& text, QWidget* parent = nullptr);

	void setElideMode(Qt::TextElideMode mode);
	void setHintWidthToString(const QString& sample);
	void setFixedWidthToString(const QString& sample);

	void clear();
	void setText(const QString& str, const QColor& color = QColor());
	void setErrText(const QString& str);
protected:
	void paintEvent(QPaintEvent* event) override;
	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;
	bool event(QEvent* event) override;
private:
	QColor color;
	int hintWidth = 0;
	Qt::TextElideMode elideMode = Qt::ElideRight;
};

namespace utils
{
	QString fileExtension(const QString& fileName);
	int numberOfDigit(int num);
	QString paddedNum(int num, int width);
	QString legalizedFileName(QString title);
	std::tuple<int, int, int> secs2HMS(int sec);
	QString secs2HmsStr(int secs);
	QString secs2HmsStrLocaleStr(int secs);
	QString formattedDataSize(qint64 bytes);

};

