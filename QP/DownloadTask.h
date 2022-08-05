#pragma once
#include <QObject>
#include <memory>
#include <QFile>
#include <QSaveFile>
#include <QNetworkReply>

using QnList = QList<int>;

struct QnInfo
{
	QnList qnList;
	int currentOn;
};
class AbstractDownloadTask :public QObject
{
	Q_OBJECT
public:
	static constexpr  qint64 Unkown = -1;
	static AbstractDownloadTask* fromJsonObj(const QJsonObject& json);
	virtual QJsonObject toJsonObj()const =0;
	virtual void startDownload()=0;
	virtual void stopDownload()=0;
	virtual void removeFile()=0;
signals:
	void downloadFinished();
	void errorOccurred(const QString& errorString);
	void getUrlfoFinished();
protected:
	QString path;
	QNetworkReply* httpReply = nullptr;
	QJsonValue getReplyJson(const QString& dataKey = QString());

	AbstractDownloadTask(const QString& path) :path(path) {}
public:
	virtual ~AbstractDownloadTask();
	QString getPath() const { return  path; }
	virtual QString getTitle() const;
	virtual qint64 getDownloadedBytesCnt() const =0;
	virtual int estimateRemainingSeconds(qint64 downBytesPerSec) const=0;
	virtual  double getProgress() const=0;
	virtual QString getProgressStr() const=0;
	virtual QString getQnDescription() const=0;
};
class AbstractVideoDownloadTask :public AbstractDownloadTask
{
	Q_OBJECT
protected:
	int durationInMSec = 0;
	int qn = 0;
	qint64 downloadedBytesCnt = 0;
	AbstractVideoDownloadTask(const QString& path, int qn) :AbstractDownloadTask(path), qn(qn) {}
public:
	~AbstractVideoDownloadTask();
	qint64 getDownloadedBytesCnt() const override;
	void startDownload() override;
	void stopDownload() override;
	virtual  QNetworkReply* getPlayUrlInfo() const=0;
	virtual QString getPlayUrlInfoDataKey() const=0;
protected:
	virtual void parsePlayUrlInfo(const QJsonObject& data)=0;
};
//class FlvLiveDownloadDelegate;
//class LiveDownloadTask :public AbstractVideoDownloadTask
//{
//	Q_OBJECT
//private:
//	QString basePath;
//	std::unique_ptr<FlvLiveDownloadDelegate> dldDelegate;
//public:
//	const qint64 roomId;
//	LiveDownloadTask(qint64 roomId, int qn, const QString& path);
//	~LiveDownloadTask();
//
//	QJsonObject toJsonObj() const override;
//	QString getTitle() const override;
//	void removeFile() override;
//	int estimateRemainingSeconds(qint64 downBytesPerSec) const override;
//	double getProgress() const override;
//	QString getProgressStr() const override;
//	QString getQnDescription() const override;
//
//	static QnList getAllPossibleQn();
//	static QString getQnDescription(int qn);
//	static QnInfo getQnInfoFromPlayUrlInfo(const QJsonObject&);
//	static QString getPlayUrlFromPlayUrlInfo(const QJsonObject&);
//	static QNetworkReply* getPlayUrlInfo(qint64 roomId, int qn);
//	QNetworkReply* getPlayUrlInfo() const override;
//	static const QString playUrlInfoDataKey;
//	QString getPlayUrlInfoDataKey() const override;
//protected:
//	void parsePlayUrlInfo(const QJsonObject& data) override;
//};
class VideoDownloadTask :public AbstractVideoDownloadTask
{
	Q_OBJECT
		qint64 totalBytesCnt = 0;
public:
	void removeFile() override;
	int estimateRemainingSeconds(qint64 downBytesPerSec) const override;
	double getProgress() const override;
	QString getProgressStr() const override;
	QString getQnDescription() const override;

	static QnList getAllPossibleQn();
	static QString getQnDescription(int qn);
	static QnInfo getQnInfoFromPlayUrlInfo(const QJsonObject&);

	QJsonObject toJsonObj() const override;
protected:
	VideoDownloadTask(const QJsonObject& json);
	using AbstractVideoDownloadTask::AbstractVideoDownloadTask;//ctor

	std::unique_ptr<QFile> file;
	std::unique_ptr<QFile> openFileForWrite();

	void parsePlayUrlInfo(const QJsonObject& data) override;

	void startDownloadStream(const QUrl& url);
	void onStreamReadyRead();
	void onStreamFinished();

	bool checkQn(int qnFromReply);
	bool checkSzie(qint64 sizeFromReply);
};

class PgcDownloadTask :public VideoDownloadTask
{
	Q_OBJECT
public:
	const qint64 ssId;
	const qint64 epId;
	PgcDownloadTask(qint64 ssId,qint64 epId,int qn,const QString &path):VideoDownloadTask(path,qn),ssId(ssId),epId(epId){}
	QJsonObject toJsonObj() const override;
	PgcDownloadTask(const QJsonObject& json);

	static QNetworkReply * getPlayUrlInfo(qint64 epId,int qn);
	QNetworkReply* getPlayUrlInfo() const override;

	static const QString playUrlInfoDataKey;
	QString getPlayUrlInfoDataKey() const override;
};

//class PugvDownloadTask :public VideoDownloadTask
//{
//	Q_OBJECT
//public:
//	PugvDownloadTask(const QJsonObject& json);
//	QNetworkReply* getPlayUrlInfo() const override;
//	QString getPlayUrlInfoDataKey() const override;
//};
//class UgcDownloadTask :public VideoDownloadTask
//{
//	Q_OBJECT
//public:
//	UgcDownloadTask(const QJsonObject& json);
//	QNetworkReply* getPlayUrlInfo() const override;
//	QString getPlayUrlInfoDataKey() const override;
//};

 /*
  *
  *
class ComicDownloadTask :public AbstractDownloadTask
{
	Q_OBJECT
public:
	const qint64 comicId;
	const qint64 epId;
	ComicDownloadTask(qint64 comicId,qint64 epId,const QString &path):AbstractDownloadTask(path),comicId(comicId),epId(epId){}
	~ComicDownloadTask();
	QJsonObject toJsonObj() const override;
	ComicDownloadTask(const QJsonObject& json);

	void startDownload() override;
	void stopDownload() override;
	void removeFile() override;

	qint64 getDownloadedBytesCnt() const override;
	int estimateRemainingSeconds(qint64 downBytesPerSec) const override;
	double getProgress() const override;
	QString getProgressStr() const override;
	QString getQnDescription() const override;
private slots:
	void getImgInfoFinished();
	void getImgToketFinieshed();
	void onImgReadyRead();
	void downloadImgFinished();
private:
	void downloadNextImg();
	void abortCurrentImg();
	std::unique_ptr<QSaveFile> openFileForWrite(const QString &filename);
private:
	int totalImgCnt=0;
	int finishedImgCnt=0;
	int curImgRecvBytesCnt=0;
	int curImagTotalBytesCnt=0;
	qint64 bytesCntTillLastImg=0;

	QVector<QString> imgRqStPahs;
	std::unique_ptr<QSaveFile> file;

};
*/
class DownloadTask
{
};

