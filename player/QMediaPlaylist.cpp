#include <QFile>
#include <QUrl>
#include <QRandomGenerator>
#include "QmediaPlayList.h"
#include "QMediaPlaylist_p.h"
#include "QPlaylistFileParser_p.h"


class QM3uPlaylistWriter
{
public:
	QM3uPlaylistWriter(QIODevice* device) :device_(device), textStream_(new QTextStream(device))
	{

	}
	~QM3uPlaylistWriter()
	{
		delete textStream_;
	}
	bool writeItem(const QUrl& item)
	{
		*textStream_ << item.toString() << Qt::endl;
		return true;
	}
private:
	QIODevice* device_;
	QTextStream* textStream_;
};

int QMediaPlaylistPrivate::nextPosition(int steps) const
{
	if (playlist.count() == 0)
	{
		return -1;
	}
	int next = currentPos + steps;
	switch (playbackMode)
	{
	case QMediaPlaylist::CurrentItemOnce:
		return steps != 0 ? -1 : currentPos;
	case QMediaPlaylist::CurrentItemLoop:
		return currentPos;
	case QMediaPlaylist::Sequential:
		if (next >= playlist.size())
		{
			next = -1;
		}
		break;
	case QMediaPlaylist::Loop:
		next %= playlist.count();
		break;
	}
	return next;
}

int QMediaPlaylistPrivate::prevPosition(int steps) const
{
	if (playlist.count() == 0)
	{
		return -1;
	}
	int next = currentPos;
	if (next < 0)
	{
		next = playlist.size();
	}
	next -= steps;
	switch (playbackMode)
	{
	case QMediaPlaylist::CurrentItemOnce:
		return steps != 0 ? -1 : currentPos;
	case QMediaPlaylist::CurrentItemLoop:
		return currentPos;
	case QMediaPlaylist::Sequential:
		if (next < 0)
		{
			next = -1;
		}
		break;
	case QMediaPlaylist::Loop:
		next %= playlist.size();
		if (next < 0)
		{
			next += playlist.size();
		}
		break;
	}
	return next;
}
QMediaPlaylist::QMediaPlaylist(QObject* parent) :QObject(parent), d_ptr(new QMediaPlaylistPrivate)
{
	Q_D(QMediaPlaylist);
	d->q_ptr = this;
}

QMediaPlaylist::~QMediaPlaylist()
{
	delete d_ptr;
}

QMediaPlaylist::PlaybackMode QMediaPlaylist::playbackMode() const
{
	return d_func()->playbackMode;
}

void QMediaPlaylist::setPlaybackMode(PlaybackMode mode)
{
	Q_D(QMediaPlaylist);
	if (mode == d->playbackMode)
	{
		return;
	}
	d->playbackMode = mode;
	emit playbackModeChanged(mode);
}

int QMediaPlaylist::currentIndex() const
{
	return d_func()->currentPos;
}

QUrl QMediaPlaylist::currentMedia() const
{
	Q_D(const QMediaPlaylist);
	if (d->currentPos < 0 || d->currentPos >= d->playlist.size())
	{
		return QUrl();
	}
	return d_func()->playlist.at(d_func()->currentPos);
}

int QMediaPlaylist::nextIndex(int steps) const
{
	return d_func()->nextPosition(steps);
}

int QMediaPlaylist::previousIndex(int steps) const
{
	return d_func()->prevPosition(steps);
}

QUrl QMediaPlaylist::media(int index) const
{
	Q_D(const QMediaPlaylist);
	if (index < 0 || index >= d->playlist.size())
	{
		return QUrl();
	}
	return d_func()->playlist.at(index);
}

int QMediaPlaylist::mediaCount() const
{
	return d_func()->playlist.count();
}

bool QMediaPlaylist::isEmpty() const
{
	return mediaCount() == 0;
}

void QMediaPlaylist::addMedia(const QUrl& content)
{
	Q_D(QMediaPlaylist);
	int pos = d->playlist.size();
	emit mediaAboutToBeInserted(pos, pos);
	d->playlist.append(content);
	emit mediaInserted(pos, pos);
}

void QMediaPlaylist::addMedia(const QList<QUrl>& items)
{
	if (!items.size())
	{
		return;
	}
	Q_D(QMediaPlaylist);
	int first = d->playlist.size();
	int last = first + items.size() - 1;
	emit mediaAboutToBeInserted(first, last);
	d_func()->playlist.append(items);
	emit mediaInserted(first, last);

}

bool QMediaPlaylist::insertMedia(int index, const QUrl& content)
{
	Q_D(QMediaPlaylist);
	index = qBound(0, index, d->playlist.size());
	emit mediaAboutToBeInserted(index, index);
	d->playlist.insert(index, content);
	emit mediaInserted(index, index);
	return true;
}

bool QMediaPlaylist::insertMedia(int index, const QList<QUrl>& items)
{
	if (!items.size())
	{
		return true;
	}
	Q_D(QMediaPlaylist);
	index = qBound(0, index, d->playlist.size());
	int last = index + items.size() - 1;
	auto newList = d->playlist.mid(0, index);
	newList += items;
	newList += d->playlist.mid(index);
	d->playlist = newList;
	emit mediaInserted(index, last);
	return true;;
}

bool QMediaPlaylist::moveMedia(int from, int to)
{
	Q_D(QMediaPlaylist);
	if (from<0 || from>d->playlist.count() || to<0 || to>d->playlist.count())
	{
		return false;
	}
	d->playlist.move(from, to);
	emit mediaChanged(from, to);
	return true;
}

bool QMediaPlaylist::removeMedia(int pos)
{
	return removeMedia(pos, pos);
}

bool QMediaPlaylist::removeMedia(int start, int end)
{
	Q_D(QMediaPlaylist);
	if (end < start || end < 0 || start >= d->playlist.count())
	{
		return false;
	}
	start = qBound(0, start, d->playlist.size() - 1);
	end = qBound(0, end, d->playlist.size() - 1);

	emit mediaAboutToBeRemoved(start, end);
	d->playlist.remove(start, end - start + 1);
	emit mediaRemove(start, end);
	return true;
}

void QMediaPlaylist::clear()
{
	Q_D(QMediaPlaylist);
	int size = d->playlist.size();
	emit mediaAboutToBeRemoved(0, size - 1);
	d->playlist.clear();
	emit mediaRemove(0, size - 1);
}

void QMediaPlaylist::load(const QUrl& location, const char* format)
{
	Q_D(QMediaPlaylist);

	d->error = NoError;
	d->errorString.clear();

	d->ensureParser();
	d->parser->start(location, QString::fromUtf8(format));

}

void QMediaPlaylist::load(QIODevice* device, const char* format)
{
	Q_D(QMediaPlaylist);
	d->error = NoError;
	d->errorString.clear();

	d->ensureParser();
	d->parser->start(device, QString::fromUtf8(format));
}

bool QMediaPlaylist::save(const QUrl& location, const char* format) const
{
	Q_D(const QMediaPlaylist);
	d->error = NoError;
	d->errorString.clear();

	if (!d->checkFormat(format))
	{
		return false;
	}
	QFile file(location.toLocalFile());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		d->error = AccessDeniedError;
		d->errorString = tr("The file could not be accessed.");
		return false;
	}
	return save(&file, format);
}

bool QMediaPlaylist::save(QIODevice* device, const char* format) const
{
	Q_D(const QMediaPlaylist);

	d->error = NoError;
	d->errorString.clear();

	if (!d->checkFormat(format))
	{
		return false;
	}
	QM3uPlaylistWriter writer(device);
	for (const auto& entry : d->playlist)
	{
		writer.writeItem(entry);
	}
	return true;
}

QMediaPlaylist::Error QMediaPlaylist::error() const
{
	return d_func()->error;
}

QString QMediaPlaylist::errorString() const
{
	return d_func()->errorString;
}

void QMediaPlaylist::shuffle()
{
	Q_D(QMediaPlaylist);
	QList<QUrl> playlist;

	QUrl current;
	if (d->currentPos != -1)
	{
		current = d->playlist.takeAt(d->currentPos);
	}
	while (!d->playlist.isEmpty())
	{
		playlist.append(d->playlist.takeAt(QRandomGenerator::global()->bounded(int(d->playlist.size()))));
	}
	if (d->currentPos!=-1)
	{
		playlist.insert(d->currentPos,current);
	}
	d->playlist=playlist;
	emit mediaChanged(0,d->playlist.count());
}

void QMediaPlaylist::next()
{
	Q_D(QMediaPlaylist);
	d->currentPos=d->nextPosition(1);
	emit currentIndexChanged(d->currentPos);
	emit currentMediaChanged(currentMedia());
}

void QMediaPlaylist::previous()
{
	Q_D(QMediaPlaylist);
	d->currentPos=d->prevPosition(1);
	emit currentIndexChanged(d->currentPos);
	emit currentMediaChanged(currentMedia());
}

void QMediaPlaylist::setCurrentIndex(int index)
{
	Q_D(QMediaPlaylist);
	if (index<0||index>=d->playlist.size())
	{
		index=-1;
	}
	d->currentPos=index;
	emit currentIndexChanged(d->currentPos);
	emit currentMediaChanged(currentMedia());
}
