#include "utils.h"

#include "MainWindow.h"
#include <QPainter>
#include <QPen>
#include <QFontMetrics>
#include <QHelpEvent>
ElidedTextLabel::ElidedTextLabel(QWidget* parent) :QLabel(parent)
{
}

ElidedTextLabel::ElidedTextLabel(const QString& text, QWidget* parent) :QLabel(text, parent)
{
	setToolTip(text);
}

void ElidedTextLabel::setElideMode(Qt::TextElideMode mode)
{
	elideMode = mode;
}

void ElidedTextLabel::setHintWidthToString(const QString& sample)
{
	hintWidth = fontMetrics().horizontalAdvance(sample);
}

void ElidedTextLabel::setFixedWidthToString(const QString& sample)
{
	this->setFixedWidth(fontMetrics().horizontalAdvance(sample));
}

void ElidedTextLabel::clear()
{
	this->color = QColor();
	QLabel::clear();
}

void ElidedTextLabel::setText(const QString& str, const QColor& color)
{
	QLabel::setText(str);
	this->color = color;
	setToolTip(str);
}

void ElidedTextLabel::setErrText(const QString& str)
{
	QLabel::setText(str);
	this->color = Qt::red;
	setToolTip(str);
}

void ElidedTextLabel::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	if (color.isValid())
	{
		auto pen = painter.pen();
		pen.setColor(color);
		painter.setPen(pen);
	}
	auto fm = fontMetrics();
	auto eledeText = fm.elidedText(text(), elideMode, width());
	painter.drawText(rect(), static_cast<int>(alignment()), eledeText);
	//QLabel::paintEvent(event);
}

QSize ElidedTextLabel::minimumSizeHint() const
{
	if (hintWidth == 0)
	{
		return QLabel::minimumSizeHint();
	}
	else
	{
		return QSize(hintWidth, QLabel::minimumSizeHint().height());
	}
}

QSize ElidedTextLabel::sizeHint() const
{
	if (hintWidth == 0)
	{

		return QLabel::sizeHint();
	}
	else
	{
		return QSize(hintWidth, QLabel::minimumSizeHint().height());
	}
}

bool ElidedTextLabel::event(QEvent* event)
{
	if (event->type() == QEvent::ToolTip)
	{
		auto helpEvent = static_cast<QHelpEvent*>(event);
		auto displayedText = fontMetrics().elidedText(text(), Qt::ElideRight, width());
		if (helpEvent->x() <= fontMetrics().horizontalAdvance(displayedText))
		{
			return QLabel::event(event);
		}
		else
		{
			return true;
		}
	}
	else
	{
		return QLabel::event(event);
	}
}

QString utils::fileExtension(const QString& fileName)
{
	auto dotPos = fileName.lastIndexOf('.');
	if (dotPos < 0)
	{
		return QString();
	}
	return fileName.sliced(dotPos);
}

int utils::numberOfDigit(int num)
{
	if (num == 0)
	{
		return 1;
	}
	int ret = 0;
	while (num != 0)
	{
		num /= 10;
		ret++;
	}
	return ret;
}

QString utils::paddedNum(int num, int width)
{
	auto s = QString::number(num);
	auto padWidth = width - s.size();
	s.prepend(QString(padWidth, '0'));
	return s;
}

QString utils::legalizedFileName(QString title)
{
	return title.simplified()
		.replace('\\', u'＼').replace('/', u'／').replace(':', u'：')
		.replace('*', u'＊').replace('?', u'？').replace('"', u'“')
		.replace('<', u'＜').replace('>', u'＞').replace('|', u'｜');
	// 整个路径的合法性检查可以参考 https://stackoverflow.com/q/62771
}

std::tuple<int, int, int> utils::secs2HMS(int sec)
{
	auto second = sec % 60;
	auto minutes = sec / 60;
	auto minute = minutes % 60;
	auto hour = minutes / 60;
	return { hour,minute,second };
}

QString utils::secs2HmsStr(int secs)
{
	auto [h, m, s] = secs2HMS(secs);
	QLatin1Char fillChar('0');
	return QString("%1:%2:%3").arg(h, 2, 10, fillChar).arg(m, 2, 10, fillChar).arg(s, 2, 10, fillChar);
}

QString utils::formattedDataSize(qint64 bytes)
{
	constexpr qint64 Kilo = 1024;
	constexpr qint64 Mega = 1024 * 1024;
	constexpr qint64 Giga = 1024 * 1024 * 1024;

	if (bytes > Giga) {
		return QString::number(static_cast<double>(bytes) / Giga, 'f', 2) + " GB";
	}
	else if (bytes > Mega) {
		return QString::number(static_cast<double>(bytes) / Mega, 'f', 2) + " MB";
	}
	else if (bytes > Kilo) {
		return QString::number(static_cast<double>(bytes) / Kilo, 'f', 1) + " KB";
	}
	else if (bytes >= 0) {
		return QString::number(bytes) + " B";
	}
	else {
		return "NaN";
	}
}

QString utils::secs2HmsStrLocaleStr(int secs)
{
	auto [h, m, s] = secs2HMS(secs);
	QString ret;
	int fieldWidth = 1;
	QLatin1Char fillChar('0');
	if (h != 0) {
		ret.append(QString::number(h) + "小时");
		fieldWidth = 2;
	}
	if (m != 0) {
		ret.append(QStringLiteral("%1分").arg(m, fieldWidth, 10, fillChar));
		fieldWidth = 2;
	}
	ret.append(QStringLiteral("%1秒").arg(s, fieldWidth, 10, fillChar));
	return ret;
}
