#pragma once
#include <QString>
#include <QDateTime>
class Entry
{
	QString name = "";
	QString title;
	QString summer;
	QString url;
	QDateTime add_time;
	int size=0;
	QString magent;
};
class comic {};
class cartoon
{
	int count=0;
	QString summer;
	QList<Entry> entries_;

};
class TV
{

};

