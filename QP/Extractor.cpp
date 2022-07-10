#include "Extractor.h"
#include "utils.h"
#include "Network.h"
#include <QJsonObject>

#include <QtNetwork>

using QRegExp = QRegularExpression;
using std::make_unique;
using std::move;

Extractor::Extractor()
{
	focusItemId = 0;
}

Extractor::~Extractor()
{
	if (httpReply != nullptr)
	{
		httpReply->abort();
	}
}

void Extractor::start(QString url)
{
	url = url.trimmed();
	QRegularExpressionMatch match;
	match = QRegExp("^(?:BV|bv)([a-zA-Z0-9]+)$").match(url);
	if (match.hasMatch())
	{
		return startUgcByBvId("BV" + match.captured(1));
	}
	match = QRegExp(R"(^av(\d+)$)").match(url);
	if (match.hasMatch())
	{
		return startUgcByAvId(match.captured(1).toLongLong());
	}
	match = QRegExp(R"(^(ss|ep)(\d+)$)").match(url);
	if (match.hasMatch())
	{
		auto type = match.captured(1) == "SS" ? PgcIdType::SeasonId : PgcIdType::EpisodeId;
		return startPgc(type, match.captured(2).toLongLong());
	}
	match = QRegExp(R"(^live(\d+)$)").match(url);
	if (match.hasMatch())
	{
		return; startLive(match.captured(1).toLongLong());
	}
	if (!url.startsWith("http://") && !url.startsWith("https://"))
	{
		parseUrl("http://" + url);
	}
	else
	{
		parseUrl(url);
	}

}

void Extractor::abort()
{
	if (httpReply != nullptr)
	{
		httpReply->abort();
	}
}

std::unique_ptr<Extractor::Result> Extractor::getResult()
{
	result->focusItemId = this->focusItemId;
	return move(result);
}

QJsonObject Extractor::getReplyJsonObj(const QString& requiredKey)
{
	auto reply = this->httpReply;
	this->httpReply = nullptr;
	reply->deleteLater();
	if (reply->error() == QNetworkReply::OperationCanceledError)
	{
		return QJsonObject();
	}
	const auto [json, errorString] = Network::Bili::parseReply(reply, requiredKey);
	if (!errorString.isNull())
	{
		emit errorOccured(errorString);
		return QJsonObject();
	}
	if (requiredKey.isEmpty())
	{
		return json;
	}
	else
	{
		auto ret = json[requiredKey].toObject();
		if (ret.isEmpty())
		{
			emit errorOccured("请求错误：内容为空");
			return QJsonObject();
		}
		return ret;
	}

}

QString Extractor::getReplyText()
{
	auto reply=httpReply;
	httpReply=nullptr;
	reply->deleteLater();
	if (reply->error()==QNetworkReply::OperationCanceledError)
	{
		return  QString();
	}
	if (reply->error()!=QNetworkReply::NoError)
	{
		emit errorOccured("网络请求错误");
		return QString();
	}
	return QString::fromUtf8(reply->readAll());
}

void Extractor::parseUrl(QUrl url)
{
	auto host = url.authority().toLower();
	auto path = url.path();
	auto query = QUrlQuery(url);
	QRegularExpressionMatch m;
	if (QRegExp(R"(^(?:www\.|m\.)?bilibili\.com$)").match(host).hasMatch()) {
		if ((m = QRegExp(R"(^/bangumi/play/(ss|ep)(\d+)/?$)").match(path)).hasMatch()) {
			auto idType = (m.captured(1) == "ss" ? PgcIdType::SeasonId : PgcIdType::EpisodeId);
			return startPgc(idType, m.captured(2).toLongLong());
		}
		if ((m = QRegExp(R"(^/bangumi/media/md(\d+)/?$)").match(path)).hasMatch()) {
			return startPgcByMdId(m.captured(1).toLongLong());
		}
		if ((m = QRegExp(R"(^/cheese/play/(ss|ep)(\d+)/?$)").match(path)).hasMatch()) {
			auto idType = (m.captured(1) == "ss" ? PugvIdType::SeasonId : PugvIdType::EpisodeId);
			return startPugv(idType, m.captured(2).toLongLong());
		}

		focusItemId = query.queryItemValue("p").toLongLong();
		if ((m = QRegExp(R"(^/(?:(?:s/)?video/)?(?:BV|bv)([a-zA-Z0-9]+)/?$)").match(path)).hasMatch()) {
			return startUgcByBvId("BV" + m.captured(1));
		}
		if ((m = QRegExp(R"(^/(?:(?:s/)?video/)?av(\d+)/?$)").match(path)).hasMatch()) {
			return startUgcByAvId(m.captured(1).toLongLong());
		}
		//        if ((m = QRegExp(R"(^$)").match(path)).hasMatch()) {
		//        }
		return urlNotSupported();
	}

	if (host == "bangumi.bilibili.com") {
		if ((m = QRegExp(R"(^/anime/(\d+)/?$)").match(path)).hasMatch()) {
			return startPgc(PgcIdType::SeasonId, m.captured(1).toLongLong());
		}
		return urlNotSupported();
	}

	if (host == "live.bilibili.com") {
		if ((m = QRegExp(R"(^/(?:h5/)?(\d+)/?$)").match(path)).hasMatch()) {
			return startLive(m.captured(1).toLongLong());
		}
		if ((m = QRegExp(R"(^/blackboard/activity-.*\.html$)").match(path)).hasMatch()) {
			return startLiveActivity(url);
		}

		return urlNotSupported();
	}

	if (host == "b23.tv") {
		if ((m = QRegExp(R"(^/(ss|ep)(\d+)$)").match(path)).hasMatch()) {
			auto idType = (m.captured(1) == "ss" ? PugvIdType::SeasonId : PugvIdType::EpisodeId);
			return startPgc(idType, m.captured(2).toLongLong());
		}
		else {
			return tryRedirect(url);
		}
	}

	if (host == "manga.bilibili.com") {
		if ((m = QRegExp(R"(^/(?:m/)?detail/mc(\d+)/?$)").match(path)).hasMatch()) {
			focusItemId = query.queryItemValue("epId").toLongLong();
			return startComic(m.captured(1).toLongLong());
		}
		if ((m = QRegExp(R"(^/(?:m/)?mc(\d+)/(\d+)/?$)").match(path)).hasMatch()) {
			focusItemId = m.captured(2).toLongLong();
			return startComic(m.captured(1).toLongLong());
		}
		return urlNotSupported();
	}

	if (host == "b22.top") {
		return tryRedirect(url);
	}

	//    auto tenkinokoWebAct = "www.bilibili.com/blackboard/topic/activity-jjR1nNRUF.html";
	//    auto tenkinokoMobiAct = "www.bilibili.com/blackboard/topic/activity-4AL5_Jqb3";

	return urlNotSupported();
}

void Extractor::tryRedirect(const QUrl& url)
{
	auto rqst = QNetworkRequest(url);
	rqst.setMaximumRedirectsAllowed(0);
	httpReply = Network::accessManager()->get(rqst);
	connect(httpReply, &QNetworkReply::finished, this, [this]()
		{
			auto reply = httpReply;
			httpReply = nullptr;
			reply->deleteLater();
			if (reply->hasRawHeader("Location"))
			{
				auto redirect = QString::fromUtf8(reply->rawHeader("Location"));
				if (redirect.contains("bilibili.com"))
				{
					parseUrl(redirect);
				}
				else
				{
					emit errorOccured("重定向目标非B站");
				}
			}
			else if (reply->error() != QNetworkReply::NoError)
			{
				emit errorOccured("网络错误");
			}
			else
			{
				emit errorOccured("未知错误");
			}

		});
}

void Extractor::startUgc(const QString& query)
{
}

void Extractor::startUgcByBvId(const QString& bvid)
{
}

void Extractor::startUgcByAvId(qint64 avid)
{
}

void Extractor::startPgcByMdId(qint64 mdId)
{
}

void Extractor::startPgc(PgcIdType idType, qint64 id)
{
	if (idType == PgcIdType::EpisodeId)
	{
		focusItemId = id;
	}
	auto api = "https://api.bilibili.com/pgc/view/web/season";
	auto query = QString("?%1=%2").arg(idType == PgcIdType::SeasonId ? "season_id" : "ep_id").arg(id);
	httpReply = Network::Bili::get(api + query);
	connect(httpReply, &QNetworkReply::finished, this, &Extractor::pgcFinished);
}

static int epFlags(int epStatus)
{
	switch (epStatus)
	{
	case 2:
		return ContentItemFlag::NoFlags;
	case 13:
		return ContentItemFlag::VipOnly;
	case 6:
	case 7:
	case 8:
	case 9:
	case 12:
		return ContentItemFlag::PayOnly;
	default:
		qDebug() << "unknown ep status" << epStatus;
		return ContentItemFlag::NoFlags;
	}
}
static QString epIndexedTitle(const QString& title, int filedWidth, const QString& indexSuffix)
{
	bool isNum;
	title.toDouble(&isNum);
	if (!isNum)
	{
		return title;
	}
	auto& unpadNumStr = title;
	auto dotPos = unpadNumStr.indexOf('.');
	auto padLen = filedWidth - (dotPos == -1 ? unpadNumStr.size() : dotPos);
	return QString("第%1%2").arg(QString(padLen, '0') + unpadNumStr, indexSuffix);
}
static QString epTitle(const QString& title, const QString& longTitle)
{
	if (longTitle.isEmpty())
	{
		return title;
	}
	else
	{
		return title + " " + longTitle;
	}
}
void Extractor::startPugv(PugvIdType idType, qint64 id)
{
}

void Extractor::startLive(qint64 roomId)
{
	auto api="https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByRoom";
	auto query="?room_id="+QString::number(roomId);
	httpReply=Network::Bili::get(api+query);
	connect(httpReply,&QNetworkReply::finished,this,&Extractor::liveFinished);
}

void Extractor::startLiveActivity(const QUrl& url)
{
}

void Extractor::startComic(int comicId)
{
}

void Extractor::urlNotSupported()
{
	static const QString supportedUrlTip =
		"输入错误或不支持. 支持的输入有: <ul>"
		"<li>B站 剧集/视频/直播/课程/漫画 链接</li>"
		"<li>剧集(番剧,电影等)ss或ep号, 比如《招魂》: <em>ss28341</em> 或 <em>ep281280</em></li>"
		"<li>视频BV或av号, 比如: <em>BV1A2b3X4y5Z</em> 或 <em>av123456</em></li>"
		"<li>live+房间号, 比如LOL赛事直播: <em>live6</em></li>"
		"</ul>"
		"<p align=\"right\">by: <a href=\"http://github.com/vooidzero/B23Downloader\">github.com/vooidzero</a></p>";
	emit errorOccured(supportedUrlTip);
}

void Extractor::ugcFinished()
{
}

void Extractor::pgcFinished()
{
	auto res = getReplyJsonObj("result");
	if (res.isEmpty())
	{
		return;
	}
	auto type = res["type"].toInt();
	QString indexSuffix = (type == 1 || type == 4) ? "话" : "集";

	auto ssid = res["season_id"].toInteger();
	auto title = res["title"].toString();
	auto pgcRes = make_unique<SectionListResult>(ContentType::PGC, ssid, title);
	auto mainSecEps = res["episodes"].toArray();
	auto totalEps = res["total"].toInt();

	if (totalEps <= 0)
	{
		totalEps = mainSecEps.size();
	}

	auto indexFieldWidth = QString::number(totalEps).size();
	pgcRes->setions.emplaceBack("正片");
	auto& mainSection = pgcRes->setions.first();
	for (auto epValR : mainSecEps)
	{
		auto epObj = epValR.toObject();
		auto title = epObj["title"].toString();
		auto longTitle = epObj["long_title"].toString();
		mainSection.episodes.emplaceBack(
			epObj["id"].toInteger(),
			epTitle(epIndexedTitle(title, indexFieldWidth, indexSuffix), longTitle),
			qRound(epObj["duration"].toDouble() / 1000.0),
			epFlags(epObj["status"].toInt())

		);
	}
	if (focusItemId == 0 && mainSection.episodes.size() == 1)
	{
		focusItemId = mainSection.episodes.first().id;
	}
	for (auto&& secValR : res["section"].toArray())
	{
		auto secObj = secValR.toObject();
		auto eps = secObj["episodes"].toArray();
		if (eps.size() == 0)
		{
			continue;
		}
		pgcRes->setions.emplaceBack(secObj["title"].toString());
		auto& sec = pgcRes->setions.last();
		for (auto&& epValR : eps)
		{
			auto epObj = epValR.toObject();
			sec.episodes.emplaceBack(
				epObj["id"].toInteger(),
				epTitle(epObj["title"].toString(), epObj["long_title"].toString()),
				qRound(epObj["duration"].toDouble() / 1000.0),
				epFlags(epObj["status"].toInt())
			);
		}

	}
	if (res["status"].toInt() == 2)
	{
		this->result = move(pgcRes);
		emit success();
		return;
	}
	//season is not free .add user payment info
	auto userStatApi = "https://api.bilibili.com/pgc/view/web/season/user/status";
	auto query = "?season_id=" + QString::number(ssid);
	httpReply = Network::Bili::get(userStatApi + query);
	connect(httpReply, &QNetworkReply::finished, this, [this, pgcRes = move(pgcRes)]()mutable
	{
		auto result = getReplyJsonObj("result");
		if (result.isEmpty())
		{
			return;
		}
		auto userIsVip = result["vip_info"].toObject()["status"].toInt() == 1;
		auto userHasPaid = result["pay"].toInt() == 1;//toInt(1)
		if (!userHasPaid)
		{
			for (auto& video : pgcRes->setions.first().episodes)
			{
				auto notFree = (video.flags & ContentItemFlag::VipOnly) or (video.flags & ContentItemFlag::PayOnly);
				if (notFree && !userHasPaid)
				{
					video.flags |= ContentItemFlag::Disabled;
				}
			}
		}
		this->result = move(pgcRes);
		emit success();
	});
}

void Extractor::pugvFinished()
{
}

void Extractor::liveFinished()
{
	auto json=getReplyJsonObj();
	if (json.isEmpty())
	{
		return;
	}
	if (!json.contains("data")||json["data"].type()==QJsonValue::Null)
	{
		emit errorOccured("B站请求错误：非法房间号");
		return;
	}
	auto data=json["data"].toObject();
	auto roomInfo=data["room_info"].toObject();
	if (!roomInfo.contains("live_status"))
	{
		emit errorOccured("发生了错误：getInfoByRoom：未找到live_status");
		return;
	}
	auto roomStatus=roomInfo["live_status"].toInt();
	if (roomStatus==0)
	{
		emit errorOccured("该房间当前未开播");
		return;
	}
	if (roomStatus==2)
	{
		emit errorOccured("该房间正在轮播");
		return;
	}
	auto roomId=roomInfo["room_id"].toInteger();
	bool hasPayment=roomInfo["special_type"].toInt()==1;
	auto title=roomInfo["title"].toString();
	auto uname=data["anchor_info"].toObject()["base_info"].toObject()["uname"].toString();
	auto liveRes=make_unique<LiveResult>(roomId,QString("[%1]%2").arg(uname,title));
	if (!hasPayment)
	{
		result=move(liveRes);
		emit success();
		return;
	}
	auto validateApi="https://api.live.bilibili.com/av/v1/PayLive/liveValidate";
	auto query="?room_id="+QString::number(roomId);
	httpReply=Network::Bili::get(validateApi+query);
	connect(httpReply,&QNetworkReply::finished,this,[this,liveRes=move(liveRes)]()mutable
	{
		auto data=getReplyJsonObj("data");
		if (data.isEmpty())
		{
			return;
		}
		if(data["permission"].toInt())
		{
			result=move(liveRes);
			emit success();
		}else
		{
			emit errorOccured("该直播需要付费购票观看");
		}
	});

}

void Extractor::liveActivityFinished()
{
}

void Extractor::comicFinished()
{
}
