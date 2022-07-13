#include "Settings.h"
#include <QtNetwork>
CookieJar::CookieJar(QObject* parent) :QNetworkCookieJar(parent)
{
}

CookieJar::CookieJar(const QString& cookies, QObject* parent) :QNetworkCookieJar(parent)
{
	fromString(cookies);
}

QByteArray CookieJar::getCookie(const QString& name) const
{
	for (auto& cookie : allCookies())
	{
		if (cookie.name() == name)
		{
			return cookie.value();
		}
	}
	return QByteArray();
}

bool CookieJar::isEmpty() const
{
	return allCookies().isEmpty();
}

void CookieJar::clear()
{
	setAllCookies(QList<QNetworkCookie>());
}

QString CookieJar::toString() const
{
	QString rs;
	for (auto& cookie : allCookies())
	{
		if (!rs.isEmpty())
		{
			rs.append(CookiesSeparator);
		}
		rs.append(cookie.toRawForm());
	}
	return rs;
}

void CookieJar::fromString(const QString& cookies)
{
	QList<QNetworkCookie> cs;
	auto cookiesStrings = cookies.split(CookiesSeparator);
	for (auto& str : cookiesStrings)
	{
		cs.append(QNetworkCookie::parseCookies(str.toUtf8()));
	}
	setAllCookies(cs);
}

Q_GLOBAL_STATIC(Settings, settings)
static constexpr auto KeyCookies = "cookies";

Settings::Settings(QObject* parent) :QSettings(QSettings::IniFormat, QSettings::UserScope, "VoidZero", "B23Downloader", parent)
{
	setFallbacksEnabled(false);
	auto cookieStr = value(KeyCookies).toString();
	cookieJar = new CookieJar(cookieStr, parent);
	if (!cookieStr.isEmpty() && cookieJar->isEmpty())
	{
		remove(KeyCookies);
	}
}

Settings* Settings::inst()
{
	return settings();
}

CookieJar* Settings::getCookieJar()
{
	return cookieJar;
}

bool Settings::hasCookies()
{
	return  contains(KeyCookies);
}

void Settings::saveCookies()
{
	setValue(KeyCookies, cookieJar->toString());
}

void Settings::removeCookies()
{
	cookieJar->clear();
	remove(KeyCookies);
}
