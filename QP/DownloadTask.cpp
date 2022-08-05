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
	qDebug() << dataKey;
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
	if (downBytesPerSec == 0 || totalBytesCnt == 0)
	{
		return -1;
	}
	qint64 ret = (totalBytesCnt - downloadedBytesCnt) / downBytesPerSec;
	return (ret > INT32_MAX ? -1 : static_cast<int>(ret));
}

double VideoDownloadTask::getProgress() const
{
	if (totalBytesCnt == 0)
	{
		return 0;
	}
	return static_cast<double>(downloadedBytesCnt) / totalBytesCnt;
}

QString VideoDownloadTask::getProgressStr() const
{
	if (totalBytesCnt == 0)
	{
		return QString();
	}
	return QStringLiteral("%1/%2").arg(utils::formattedDataSize(downloadedBytesCnt), utils::formattedDataSize(totalBytesCnt));
}

QString VideoDownloadTask::getQnDescription() const
{
	return videoQnDescMap.value(qn);
}

QnList VideoDownloadTask::getAllPossibleQn()
{
	return videoQnDescMap.keys();
}

QString VideoDownloadTask::getQnDescription(int qn)
{
	return videoQnDescMap.value(qn);
}

QnInfo VideoDownloadTask::getQnInfoFromPlayUrlInfo(const QJsonObject& data)
{
	QnInfo info;
	for (auto&& fmtValR : data["support_formats"].toArray())
	{
		auto fmtObj = fmtValR.toObject();
		auto qn = fmtObj["quality"].toInt();
		auto desc = fmtObj["new_description"].toString();
		info.qnList.append(qn);
		if (videoQnDescMap.value(qn) != desc)
		{
			videoQnDescMap.insert(qn, desc);
		}
	}
	info.currentOn = data["quality"].toInt();
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

VideoDownloadTask::VideoDownloadTask(const QJsonObject& json)
	:AbstractVideoDownloadTask(json["path"].toString(), json["qn"].toInt())
{
	downloadedBytesCnt = json["bytes"].toInteger(0);
	totalBytesCnt = json["total"].toInteger();
}

std::unique_ptr<QFile> VideoDownloadTask::openFileForWrite()
{

	auto dir = QFileInfo(path).absolutePath();
	if (!QFileInfo::exists(dir))
	{
		if (!QDir().mkpath(dir))
		{
			emit errorOccurred(" create dir false");
			return nullptr;
		}
	}
	auto file = std::make_unique<QFile>(path);
	if (!file->open(QIODevice::ReadWrite))
	{
		emit errorOccurred("open file error");
		return  nullptr;
	}
	auto fileSize = file->size();
	if (fileSize < downloadedBytesCnt)
	{
		qDebug() << QString("filesize(%1) < bytes(%2)").arg(fileSize).arg(downloadedBytesCnt);
		downloadedBytesCnt = fileSize;
	}
	file->seek(downloadedBytesCnt);
	return file;
}

void VideoDownloadTask::parsePlayUrlInfo(const QJsonObject& data)
{
	auto qnInfo = getQnInfoFromPlayUrlInfo(data);
	if (!checkQn(qnInfo.currentOn))
	{
		return;
	}
	if (data["has_paid"].toInt(1) == 0)
	{
		emit errorOccurred("该视频需要大会员/付费");
		return;
	}
	auto durl = data["durl"].toArray();
	if (durl.size() == 0)
	{
		emit errorOccurred("请求错误：durl为空");
		return;
	}
	else if (durl.size() > 1)
	{
		emit errorOccurred("该视频当前画质有分段（不支持）");
		return;
	}
	auto durlObj = durl.first().toObject();
	if (!checkSzie(durlObj["size"].toInt()))
	{
		return;
	}
	durationInMSec = durlObj["length"].toInt();
	startDownloadStream(durlObj["url"].toString());
}

void VideoDownloadTask::startDownloadStream(const QUrl& url)
{
	emit getUrlfoFinished();

	auto ext = utils::fileExtension(url.fileName());
	if (downloadedBytesCnt == 0 && !path.endsWith(ext, Qt::CaseInsensitive))
	{
		path.append(ext);
	}
	file = openFileForWrite();
	if (!file)
	{
		return;
	}
	auto request = Network::Bili::Request(url);

	if (downloadedBytesCnt != 0)
	{
		request.setRawHeader("Range", "bytes" + QByteArray::number(downloadedBytesCnt) + "-");
	}
	httpReply = Network::accessManager()->get(request);
	connect(httpReply, &QNetworkReply::readyRead, this, &VideoDownloadTask::onStreamReadyRead);
	connect(httpReply, &QNetworkReply::finished, this, &VideoDownloadTask::onStreamFinished);

}

void VideoDownloadTask::onStreamReadyRead()
{
	auto tmp = downloadedBytesCnt + httpReply->bytesAvailable();
	Q_ASSERT(file != nullptr);
	if (-1 == file->write(httpReply->readAll()))
	{
		emit errorOccurred("文件写入失败:" + file->errorString());
		httpReply->abort();
	}
	else
	{
		downloadedBytesCnt = tmp;
	}
}

void VideoDownloadTask::onStreamFinished()
{
	auto reply = httpReply;
	httpReply->deleteLater();
	httpReply = nullptr;

	file.reset();

	if (reply->error() == QNetworkReply::OperationCanceledError)
	{
		return;
	}
	if (reply->error() != QNetworkReply::NoError)
	{
		emit errorOccurred("网络请求错误");
		return;
	}
	emit downloadFinished();
}

bool VideoDownloadTask::checkQn(int qnFromReply)
{
	if (qnFromReply != qn)
	{
		if (downloadedBytesCnt == 0)
		{
			qn = qnFromReply;
		}
		else
		{
			emit errorOccurred("获取到画质与已下载部分不同. 请确定登录/会员状态");
			return false;
		}
	}
	return true;
}

bool VideoDownloadTask::checkSzie(qint64 sizeFromReply)
{
	if (totalBytesCnt != sizeFromReply)
	{
		if (downloadedBytesCnt > 0)
		{
			emit errorOccurred("获取到文件大小于先前不一致");
			return  false;
		}
		else
		{
			totalBytesCnt = sizeFromReply;
		}
	}
	return true;
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
	auto query = QString("?ep_id=%1&qn=%2&fourk=1").arg(epId).arg(qn);
	qDebug() << "getPlayUrlInfo " << api + query;
	return Network::Bili::get(api + query);
}

QNetworkReply* PgcDownloadTask::getPlayUrlInfo() const
{
	return getPlayUrlInfo(epId, qn);
}
const QString PgcDownloadTask::playUrlInfoDataKey = "result";
QString PgcDownloadTask::getPlayUrlInfoDataKey() const
{
	return playUrlInfoDataKey;
}

