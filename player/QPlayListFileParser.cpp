#include "QPlaylistFileParser_p.h"
#include <QFileInfo>
#include <QDebug>
#include <QIODevice>
#include <QPointer>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMediaPlayer>
#include <QMediaMetaData>

#include "QMediaPlaylist.h"

namespace
{
	class ParserBase
	{
	public:
		explicit ParserBase(QPlaylistFileParser* parent) :parent_(parent), aborted_(false)
		{
			//Q_ASSERT(parent_ != nullptr);
		}
		bool parseLine(int lineIndex, const QString& line, const QUrl& root)
		{
			if (aborted_)
			{
				return false;
			}
			const bool ok = parseLineImpl(lineIndex, line, root);
			return ok && !aborted_;
		}
		virtual void abort() {
			aborted_ = true;
		}
		virtual ~ParserBase() = default;
	protected:
		virtual  bool parseLineImpl(int lineIndex, const QString& line, const QUrl& root) = 0;
		static QUrl expandToFullPath(const QUrl& root, const QString& line)
		{
			if (line.startsWith(QLatin1String("//")) || line.startsWith(QLatin1String("\\\\")))
			{
				return QUrl::fromLocalFile(line);
			}
			QUrl url(line);
			if (url.scheme().isEmpty())
			{
				if (root.isLocalFile())
				{
					return QUrl::fromUserInput(line, root.adjusted(QUrl::RemoveFilename).toLocalFile(), QUrl::AssumeLocalFile);
				}
				return root.resolved(url);
			}
			if (url.scheme().length() == 1)
			{
				url = QUrl::fromLocalFile(line);
			}
			return url;
		}
		void newItemFound(const QVariant& content)
		{
			Q_EMIT parent_.newItem(content);
		}
		QPlaylistFileParser parent_;
		bool aborted_;
	};
	class M3UParser :public ParserBase
	{
	public:
		explicit M3UParser(QPlaylistFileParser* q) :ParserBase(q), extendedFormat_(false)
		{

		}
		bool parseLineImpl(int lineIndex, const QString& line, const QUrl& root) override
		{
			qDebug() << __func__ << " " << line << " " << root;
			if (line[0] == u'#')
			{
				if (extendedFormat_)
				{
					if (line.startsWith(QLatin1String("#EXTINF:")))
					{
						extraInfo_.clear();
						int artistStart = line.indexOf(QLatin1String(","), 8);
						bool ok = false;
						QStringView lineView{ line };
						int length = lineView.mid(8, artistStart < 8 ? -1 : artistStart - 8).trimmed().toInt(&ok);
						if (ok && length > 0)
						{
							extraInfo_[QMediaMetaData::Duration] = QVariant(length * 1000);
						}
						if (artistStart > 0)
						{
							int titleStart = getSplitIndex(line, artistStart);
							if (titleStart > artistStart)
							{
								extraInfo_[QMediaMetaData::Author] = lineView.mid(artistStart + 1, titleStart - artistStart - 1).trimmed().toString().replace(QLatin1String("--"), QLatin1String("-"));
								extraInfo_[QMediaMetaData::Title] = lineView.mid(titleStart + 1).trimmed().toString().replace(QLatin1String("--"), QLatin1String("-"));
							}
							else
							{
								extraInfo_[QMediaMetaData::Title] = lineView.mid(artistStart + 1).trimmed().toString().replace(QLatin1String("--"), QLatin1String("-"));
							}

						}
					}
				}
				else if (lineIndex == 0 && line.startsWith(QLatin1String("#EXTM3U")))
				{
					extendedFormat_ = true;
				}
			}
			else
			{
				QUrl url = expandToFullPath(root, line);
				extraInfo_[QMediaMetaData::Url] = url;
				parent_.playlist.append(url);
				newItemFound(QVariant::fromValue(extraInfo_));
				extraInfo_.clear();
			}
			return true;
		}
		int getSplitIndex(const QString& line, int startPos)
		{
			if (startPos < 0)
			{
				startPos = 0;
			}
			const QChar* buf = line.data();
			for (int i = startPos; i < line.length(); ++i)
			{
				if (buf[i] == u'-')
				{
					if (i == line.length() - 1)
					{
						return i;
					}
					++i;
					if (buf[i] != u'-')
					{
						return i - 1;
					}
				}
			}
			return -1;
		}
	private:
		QMediaMetaData extraInfo_;
		bool extendedFormat_;
	};

	class PLSParser :public ParserBase
	{

	public:
		explicit PLSParser(QPlaylistFileParser* q) :ParserBase(q) {}
		bool parseLineImpl(int lineIndex, const QString& line, const QUrl& root) override
		{
			qDebug() << " " << __func__;
			if (!line.startsWith(QLatin1String("File")))
			{
				return true;
			}
			QString value = getValue(line);
			if (value.isEmpty())
			{
				return true;
			}
			QUrl path = expandToFullPath(root, value);
			parent_.playlist.append(path);
			newItemFound(path);
			return true;
		}
		QString getValue(QStringView line)
		{
			int start = line.indexOf(u'=');
			if (start < 0)
			{
				return QString();
			}
			return line.mid(start + 1).trimmed().toString();
		}
	};
}

class QPlaylistFileParserPrivate
{
	Q_DECLARE_PUBLIC(QPlaylistFileParser)
public:
	QPlaylistFileParserPrivate(QPlaylistFileParser* q) :q_ptr(q), stream_(nullptr), type_(QPlaylistFileParser::UNKNOWN), scanIndex_(0), lineIndex_(-1), utf8_(false), aborted_(false) {}
	void handleData();
	void handleParserFinished();
	void abort();
	void reset();

	QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> source_;
	QScopedPointer<ParserBase> currentParser_;

	QByteArray buffer_;
	QUrl root_;
	QNetworkAccessManager mgr_;
	QString mimeType_;
	QPlaylistFileParser* q_ptr;
	QPointer<QIODevice> stream_;
	QPlaylistFileParser::FileType type_;

	struct ParserJob
	{
		QIODevice* m_stream;
		QUrl m_media;
		QString m_mimeType;
		bool isValid() const {
			return m_stream || !m_media.isEmpty();
		}
		void reset() {
			m_stream = nullptr; m_media = QUrl(); m_mimeType = QString();
		}
	} pendingJob_;
	int scanIndex_;
	int lineIndex_;
	bool utf8_;
	bool aborted_;
private:
	bool processLine(int startIndex, int length);
};

#define LINE_LIMIT 4000
#define READ_LIMIT 64

bool QPlaylistFileParserPrivate::processLine(int startIndex, int length)
{
	Q_Q(QPlaylistFileParser);
	lineIndex_++;

	if (!currentParser_)
	{
		const QString urlString = root_.toString();
		const QString& suffix = !urlString.isEmpty() ? QFileInfo(urlString).suffix() : urlString;
		QString mimeType;
		if (source_)
		{
			mimeType = source_->header(QNetworkRequest::ContentTypeHeader).toString();
		}
		type_ = QPlaylistFileParser::findPlaylistType(suffix, !mimeType.isEmpty() ? mimeType : mimeType_, buffer_.constData(), quint32(buffer_.size()));
		switch (type_)
		{
		case QPlaylistFileParser::UNKNOWN:
			emit q->error(QMediaPlaylist::FormatError, QMediaPlaylist::tr("%1 playlist type is unkown").arg(root_.toString()));
			q->abort();
			break;
		case QPlaylistFileParser::M3U:
			currentParser_.reset(new M3UParser(q));
			break;
		case QPlaylistFileParser::M3U8:
			currentParser_.reset(new M3UParser(q));
			utf8_ = true;
			break;
		case QPlaylistFileParser::PLS:
			currentParser_.reset(new PLSParser(q));
			break;
		}
		Q_ASSERT(!currentParser_.isNull());
	}
	QString line;
	if (utf8_)
	{
		line = QString::fromUtf8(buffer_.constData() + startIndex, length).trimmed();
	}
	else
	{
		line = QString::fromLatin1(buffer_.constData() + startIndex, length).trimmed();
	}
	if (line.isEmpty())
	{
		return true;
	}
	Q_ASSERT(currentParser_);
	return currentParser_->parseLine(lineIndex_, line, root_);
	return false;
}
void QPlaylistFileParserPrivate::handleData()
{
	Q_Q(QPlaylistFileParser);
	while (stream_->bytesAvailable() && !aborted_)
	{
		int expectedBytes = qMin(READ_LIMIT, int(qMin(stream_->bytesAvailable(), qint64(LINE_LIMIT - buffer_.size()))));

		buffer_.push_back(stream_->read(expectedBytes));
		int processBytes = 0;
		while (scanIndex_ < buffer_.length() && !aborted_)
		{
			char s = buffer_[scanIndex_];
			if (s == '\r' || s == '\n')
			{
				int l = scanIndex_ - processBytes;
				if (l > 0)
				{
					if (!processLine(processBytes, l))
					{
						break;
					}
				}
				processBytes = scanIndex_ + 1;
				if (!stream_)
				{
					return;
				}
			}
			scanIndex_++;
		}
		if (aborted_)
		{
			break;
		}
		if (buffer_.length() - processBytes >= LINE_LIMIT)
		{
			emit q->error(QMediaPlaylist::FormatError, QMediaPlaylist::tr("invalid line in playlist file"));
			q->abort();
			break;
		}
		if (!stream_->bytesAvailable() && (!source_ || !source_->isFinished()))
		{
			//last line
			processLine(processBytes, -1);
			break;
		}
		Q_ASSERT(buffer_.length() == scanIndex_);
		if (processBytes == 0)
		{
			continue;
		}
		int copyLength = buffer_.length() - processBytes;
		if (copyLength > 0)
		{
			Q_ASSERT(copyLength <= READ_LIMIT);
			buffer_ = buffer_.right(copyLength);
		}
		else
		{
			buffer_.clear();
		}
		scanIndex_ = 0;
	}
	handleParserFinished();
}
void QPlaylistFileParserPrivate::handleParserFinished()
{
	Q_Q(QPlaylistFileParser);
	const bool isParserValid = !currentParser_.isNull();
	if (!isParserValid && !aborted_)
	{
		emit q->error(QMediaPlaylist::FormatNotSupportedError, QMediaPlaylist::tr("Empty file provided"));
	}
	if (isParserValid && !aborted_)
	{
		currentParser_.reset();
		emit q->finished();
	}
	if (!aborted_)
	{
		q->abort();
	}
	if (!source_.isNull())
	{
		source_.reset();
	}
	if (pendingJob_.isValid())
	{
		q->start(pendingJob_.m_media, pendingJob_.m_stream, pendingJob_.m_mimeType);
	}
}

void QPlaylistFileParserPrivate::abort()
{
	aborted_ = true;
	if (!currentParser_.isNull())
	{
		currentParser_->abort();
	}
}

void QPlaylistFileParserPrivate::reset()
{
	Q_ASSERT(currentParser_.isNull());
	Q_ASSERT(source_.isNull());

	buffer_.clear();
	root_.clear();
	mimeType_.clear();
	stream_ = nullptr;
	type_ = QPlaylistFileParser::UNKNOWN;
	scanIndex_ = 0;
	lineIndex_ = -1;
	utf8_ = false;
	aborted_ = false;
	pendingJob_.reset();
}


QPlaylistFileParser::QPlaylistFileParser(QObject* parent) :QObject(parent), d_ptr(new QPlaylistFileParserPrivate(this))
{

}
QPlaylistFileParser::~QPlaylistFileParser()
{
}
QPlaylistFileParser::FileType QPlaylistFileParser::findByMimeType(const QString& mime)
{
	if (mime == QLatin1String("text/uri-list") || mime == QLatin1String("audio/x-mpegurl") || mime == QLatin1String("audio/mpegurl"))
	{
		return QPlaylistFileParser::M3U;
	}
	if (mime == QLatin1String("application/x-mpegURL") || mime == QLatin1String("application/vnd.apple.mpegurl"))
	{
		return QPlaylistFileParser::M3U8;
	}
	if (mime == QLatin1String("audio/x-scpls"))
	{
		return QPlaylistFileParser::PLS;
	}
	return QPlaylistFileParser::UNKNOWN;
}
QPlaylistFileParser::FileType QPlaylistFileParser::findBySuffixType(const QString& suffix)
{
	const QString& s = suffix.toLower();
	if (s == QLatin1String("m3u"))
	{
		return QPlaylistFileParser::M3U;
	}
	if (s == QLatin1String("m3u8"))
	{
		return QPlaylistFileParser::M3U8;
	}
	if (s == QLatin1String("pls"))
	{
		return QPlaylistFileParser::PLS;
	}
	return QPlaylistFileParser::UNKNOWN;
}
QPlaylistFileParser::FileType QPlaylistFileParser::findByDataHeader(const char* data, quint32 size)
{
	if (!data || size == 0)
	{
		return QPlaylistFileParser::UNKNOWN;
	}
	if (size > 7 && strncmp(data, "#EXTM3U", 7) == 0)
	{
		return QPlaylistFileParser::M3U;
	}
	if (size > 10 && strncmp(data, "[playlist]", 10) == 0)
	{
		return QPlaylistFileParser::PLS;
	}
	return QPlaylistFileParser::UNKNOWN;
}
QPlaylistFileParser::FileType QPlaylistFileParser::findPlaylistType(const QString& suffix, const QString& mime, const char* data, quint32 size)
{
	FileType dataHeaderType = findByDataHeader(data, size);
	if (dataHeaderType != UNKNOWN)
	{
		return dataHeaderType;
	}
	FileType mimeType = findByMimeType(mime);
	if (mimeType != UNKNOWN)
	{
		return mimeType;
	}
	FileType suffixType = findBySuffixType(suffix);
	if (suffixType != UNKNOWN)
	{
		return suffixType;
	}
	return UNKNOWN;
}


void QPlaylistFileParser::start(const QUrl& media, QIODevice* stream, const QString& mimeType)
{
	if (stream)
	{
		start(stream, mimeType);
	}
	else
	{
		start(media, mimeType);
	}
}
void QPlaylistFileParser::start(QIODevice* stream, const QString& mimeType)
{
	Q_D(QPlaylistFileParser);
	const bool validStream = stream ? (stream->isOpen() && stream->isReadable()) : false;
	if (!validStream)
	{
		Q_EMIT error(QMediaPlaylist::AccessDeniedError, QMediaPlaylist::tr("Invalid stream"));
	}
	if (!d->currentParser_.isNull())
	{
		abort();
		d->pendingJob_ = { stream,QUrl(),mimeType };
		return;
	}
	playlist.clear();
	d->reset();
	d->mimeType_ = mimeType;
	d->stream_ = stream;
	connect(d->stream_, &QIODevice::readyRead, this, &QPlaylistFileParser::handleData);
	d->handleData();
}
void QPlaylistFileParser::start(const QUrl& request, const QString& mimeType)
{
	Q_D(QPlaylistFileParser);
	const QUrl& url = request.url();

	if (url.isLocalFile() && !QFile::exists(url.toLocalFile()))
	{
		emit error(QMediaPlaylist::AccessDeniedError, QString(QMediaPlaylist::tr("%1 does not exist").arg(url.toString())));
		return;
	}
	if (!d->currentParser_.isNull())
	{
		abort();
		d->pendingJob_ = { nullptr,request,mimeType };
		return;
	}
	d->reset();
	d->root_ = url;
	d->mimeType_ = mimeType;
	d->source_.reset(d->mgr_.get(QNetworkRequest(request)));
	d->stream_ = d->source_.get();

	connect(d->source_.data(), &QNetworkReply::readyRead, this, &QPlaylistFileParser::handleData);
	connect(d->source_.data(), &QNetworkReply::finished, this, &QPlaylistFileParser::handleData);
	connect(d->source_.data(), SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this, SLOT(handleError()));

	if (url.isLocalFile())
	{
		d->handleData();
	}
}

void QPlaylistFileParser::abort()
{
	Q_D(QPlaylistFileParser);
	d->abort();

	if (d->source_)
	{
		d->source_->disconnect();
	}
	if (d->stream_)
	{
		disconnect(d->stream_,SIGNAL(readyRead()),this,SLOT(handleData()));
	}
	playlist.clear();
}
void QPlaylistFileParser::handleData()
{
	Q_D(QPlaylistFileParser);
	d->handleData();
}

void QPlaylistFileParser::handleError()
{
	Q_D(QPlaylistFileParser);
	const QString &errorString=d->source_->errorString();
	Q_EMIT error(QMediaPlaylist::NetworkError,errorString);
	abort();
}

