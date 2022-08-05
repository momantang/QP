#include "player.h"

Player::Player(QWidget* parent):QWidget(parent)
{
}

bool Player::isPlayerAvailable() const
{
	return true;
}

void Player::addToPlaylist(QList<QUrl> urls)
{
}

void Player::setTrackInfo(const QString& info)
{
}

void Player::setStatusInfo(const QString& info)
{
}

void Player::handleCursor(QMediaPlayer::MediaStatus status)
{
}

QString Player::trackName(const QMediaMetaData& metaData, int index)
{
	return QString();
}
