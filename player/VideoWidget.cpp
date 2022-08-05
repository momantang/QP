#include "VideoWidget.h"

VideoWidget::VideoWidget(QWidget* parent)
{
}

void VideoWidget::keyPressEvent(QKeyEvent* event)
{
	QVideoWidget::keyPressEvent(event);
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	QVideoWidget::mouseDoubleClickEvent(event);
}

void VideoWidget::mousePressEvent(QMouseEvent* event)
{
	QVideoWidget::mousePressEvent(event);
}
