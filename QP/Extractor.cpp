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
}

std::unique_ptr<Extractor::Result> Extractor::getResult()
{
	std::unique_ptr<Extractor::Result> rs;
	return rs;
}

QJsonObject Extractor::getReplyJsonObj(const QString& requiredKey)
{
	return QJsonObject();
}

QString Extractor::getReplyText()
{
	return QString();
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
	auto rqst=QNetworkRequest(url);
	rqst.setMaximumRedirectsAllowed(0);
	httpReply=Network::accessManager()->get(rqst);
	connect(httpReply,&QNetworkReply::finished,this,[this]()
	{
		auto reply=httpReply;
		httpReply=nullptr;
		reply->deleteLater();
		if (reply->hasRawHeader("Location"))
		{
			auto redirect=QString::fromUtf8(reply->rawHeader("Location"));
			if (redirect.contains("bilibili.com"))
			{
				parseUrl(redirect);
			}e
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
}

void Extractor::startPugv(PugvIdType idType, qint64 id)
{
}

void Extractor::startLive(qint64 roomId)
{
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
}

void Extractor::pugvFinished()
{
}

void Extractor::liveFinished()
{
}

void Extractor::liveActivityFinished()
{
}

void Extractor::comicFinished()
{
}
