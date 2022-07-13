#include "DownloadTask.h"
#include "Extractor.h"
#include "Network.h"
#include "utils.h"
//#include "FLV.h"
#include <QtNetwork>


// 127: 8K 超高清
// 126: 杜比视界
// 125: HDR 真彩
static QMap<int, QString> videoQnDescMap{
	  {120, "4K 超清"},
	{116, "1080P 60帧"},
	{112, "1080P 高码率"},
	{80, "1080P 高清"},
	{74, "720P 60帧"},
	{64, "720P 高清"},
	{32, "480P 清晰"},
	{16, "360P 流畅"},
};
static QMap<int, QString> liveQnDescMap{
	{10000, "原画"},
	{400, "蓝光"},
	{250, "超清"},
	{150, "高清"},
	{80, "流畅"}
};

inline bool jsonValue2Bool(const QJsonValue& val, bool defaultVal = false)
{
	if (val.isNull() || val.isUndefined())
	{
		return defaultVal;
	}
	if (val.isBool())
	{
		return val.toBool();
	}
	if (val.isDouble())
	{
		return static_cast<bool>(val.toInt());
	}
	if (val.isString())
	{
		auto str = val.toString().toLower();
		if (str == "1" || str == "true")
		{
			return true;
		}
		if (str == "0" || str == "false")
		{
			return false;
		}
	}
	return defaultVal;
}
AbstractDownloadTask* AbstractDownloadTask::fromJsonObj(const QJsonObject& json)
{
	int type = json["type"].toInt(-1);
	switch (type)
	{
	case static_cast<int>(ContentType::PGC):
		return new PgcDownloadTask(json);
		//case static_cast<int>(ContentType::PUGV):
		//	return new PugvDownloadTask(json);
		//case static_cast<int>(ContentType::UGC):
		//	return new UgcDownloadTask(json);
		/*case static_cast<int>(ContentType::COMIC):
			return new ComicDownloadTask(json);*/
	}
	return new PgcDownloadTask(json);


}





QJsonValue AbstractDownloadTask::getReplyJson(const QString& dataKey)
{
	auto reply = this->httpReply;
	this->httpReply = nullptr;
	reply->deleteLater();

	//abourt() is called
	if (reply->error() == QNetworkReply::OperationCanceledError)
	{
		return QJsonValue();
	}
	const auto [json, errorString] = Network::Bili::parseReply(reply, dataKey);
	if (!errorString.isNull())
	{
		emit errorOccurred(errorString);
		return QJsonValue();
	}
	if (dataKey.isEmpty())
	{
		return json;
	}
	else
	{
		return json[dataKey];
	}
}

AbstractDownloadTask::~AbstractDownloadTask()
{
	if (httpReply != nullptr)
	{
		httpReply->abort();
	}
}

QString AbstractDownloadTask::getTitle() const
{
	return QFileInfo(path).baseName();
}

QString AbstractDownloadTask::getProgressStr() const
{
	return QString();
}

QString AbstractDownloadTask::getQnDescription() const
{
	return QString();
}

AbstractVideoDownloadTask::~AbstractVideoDownloadTask()
{
}

qint64 AbstractVideoDownloadTask::getDownloadedBytesCnt() const
{
	return downloadedBytesCnt;
}

void AbstractVideoDownloadTask::startDownload()
{
	httpReply = getPlayUrlInfo();
	connect(httpReply, &QNetworkReply::finished, this, [this]
		{
			auto data = getReplyJson(getPlayUrlInfoDataKey()).toObject();
			if (data.isEmpty())
			{
				return;
			}
			parsePlayUrlInfo(data);
		});
}

void AbstractVideoDownloadTask::stopDownload()
{
	if (httpReply != nullptr)
	{
		httpReply->abort();
	}
}



void VideoDownloadTask::removeFile()
{
	QFile::remove(path);
}

int VideoDownloadTask::estimateRemainingSeconds(qint64 downBytesPerSec) const
{
	return 0;
}

double VideoDownloadTask::getProgress() const
{
	return 0.0;
}

QString VideoDownloadTask::getProgressStr() const
{
	return QString();
}

QString VideoDownloadTask::getQnDescription() const
{
	return QString();
}

QnList VideoDownloadTask::getAllPossibleQn()
{
	QnList list;
	return list;
}

QString VideoDownloadTask::getQnDescription(int qn)
{
	return QString();
}

QnInfo VideoDownloadTask::getQnInfoFromPlayUrlInfo(const QJsonObject&)
{
	QnInfo info;
	return  info;
}

QJsonObject VideoDownloadTask::toJsonObj() const
{
	return QJsonObject{
		{"path", path},
		{"qn", qn},
		{"bytes", downloadedBytesCnt},
		{"total", totalBytesCnt}
	};
}

VideoDownloadTask::VideoDownloadTask(const QJsonObject& json) :AbstractVideoDownloadTask(json["path"].toString(), json["qn"].toInt())
{
}

std::unique_ptr<QFile> VideoDownloadTask::openFileForWrite()
{
	std::unique_ptr<QFile> a;
	return a;
}

void VideoDownloadTask::parsePlayUrlInfo(const QJsonObject& data)
{
}

void VideoDownloadTask::startDownloadStream(const QUrl& url)
{
}

void VideoDownloadTask::onStreamReadyRead()
{
}

void VideoDownloadTask::onStreamFinished()
{
}

bool VideoDownloadTask::checkQn(int qnFromReply)
{
	return false;
}

bool VideoDownloadTask::checkSzie(qint64 sizeFromReply)
{
	return false;
}

QJsonObject PgcDownloadTask::toJsonObj() const
{
	auto json = VideoDownloadTask::toJsonObj();
	json.insert("type", static_cast<int>(ContentType::PGC));
	json.insert("ssid", ssId);
	json.insert("epid", epId);
	return json;
}

PgcDownloadTask::PgcDownloadTask(const QJsonObject& json) :VideoDownloadTask(json), ssId(json["ssid"].toInteger()), epId(json["epid"].toInteger())
{
}

QNetworkReply* PgcDownloadTask::getPlayUrlInfo(qint64 epId, int qn)
{
	auto api = "https://api.bilibili.com/pgc/player/web/playurl";
	auto query = QString("ep_id=%1&qn=%2&fourk=1").arg(epId).arg(qn);
	return Network::Bili::get(api + query);
}

QNetworkReply* PgcDownloadTask::getPlayUrlInfo() const
{
	return getPlayUrlInfo(epId,qn);
}
const QString PgcDownloadTask::playUrlInfoDataKey="result";
QString PgcDownloadTask::getPlayUrlInfoDataKey() const
{
	return playUrlInfoDataKey;
}

