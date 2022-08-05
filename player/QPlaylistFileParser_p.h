#pragma once
#include "qtmultimediaglobal.h"
#include "QMediaPlaylist.h"
#include <QObject>
#include <QIODevice>
#include <QUrl>
#include <QNetworkRequest>
class QPlaylistFileParserPrivate;

class QPlaylistFileParser :public QObject
{
	Q_OBJECT
public:
	QPlaylistFileParser(QObject* parent = nullptr);
	~QPlaylistFileParser();

	enum FileType
	{
		UNKNOWN,
		M3U,
		M3U8,
		PLS
	};
	void start(const QUrl& media, QIODevice* stream = nullptr, const QString& mimeType = QString());
	void start(const QUrl& request, const QString& mimeType = QString());
	void start(QIODevice* stream, const QString& mimeType = QString());
	void abort();

	QList<QUrl> playlist;

signals:
	void newItem(const QVariant& content);
	void finished();
	void error(QMediaPlaylist::Error err, const QString& errorMsg);

private slots:
	void handleData();
	void handleError();

private:
	static FileType findByMimeType(const QString& mime);
	static FileType findBySuffixType(const QString& suffix);
	static FileType findByDataHeader(const char* data, quint32 size);
	static FileType findPlaylistType(QIODevice* device, const QString& mime);
	static FileType findPlaylistType(const QString& suffix, const QString& mime, const char* data = nullptr, quint32 size = 0);

	Q_DISABLE_COPY(QPlaylistFileParser);
	Q_DECLARE_PRIVATE(QPlaylistFileParser);
	QScopedPointer<QPlaylistFileParserPrivate> d_ptr;
};