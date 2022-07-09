#pragma once
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <tuple>

/**
 * \brief 
 */
namespace Network
{
	QNetworkAccessManager *accessManager();
	int statusCode(QNetworkReply *reply);

	namespace Bili
	{
		extern const QByteArray Referer;
		extern const QByteArray UserAgent;

		class Request :public QNetworkRequest
		{
		public:
			Request(const QUrl &url);
			
		};
		QNetworkReply *get(const QString &url);
		QNetworkReply *get(const QUrl &url);
		QNetworkReply *postUrlEncoded(const QString &url,const QByteArray &data);
		QNetworkReply *postJson(const QString &url,const QByteArray&data);
		QNetworkReply *postJson(const QString &url,const QJsonObject &obj);
		std::pair<QJsonObject,QString> parseReply(QNetworkReply *reply,const QString&requiredKey=QString());
	}
}
