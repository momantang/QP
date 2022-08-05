#pragma once

#include <QtWidgets>
#include <QMediaPlayer>
#include <QMediaMetaData>

#include "QMediaPlaylist.h"
//class QVideoWidget;
//class PlaylistModel;

class Player : public QWidget
{
	Q_OBJECT

public:
	explicit  Player(QWidget* parent = nullptr);
	~Player() = default;
	bool isPlayerAvailable() const;
	void  addToPlaylist(QList<QUrl> urls);
private:
	void setTrackInfo(const QString& info);
	void setStatusInfo(const QString& info);
	void handleCursor(QMediaPlayer::MediaStatus status);
	QString trackName(const QMediaMetaData& metaData, int index);

	QMediaPlayer* player_=nullptr;
	QAudioOutput *audioOutput_=nullptr;
	//QMediaPlaylist *playlist_=nullptr;
	//QVideoWidget *videoWidget_=nullptr;
	QSlider* slider_=nullptr;
	QLabel* labelDuration_=nullptr;
	QPushButton * fullScreenButton_=nullptr;
	QComboBox * audioOutputCombo=nullptr;
	QLabel * statusLabel_=nullptr;
	QStatusBar *statusBar_=nullptr;

	QComboBox* audioTracks_=nullptr;
	QComboBox* videoTracks=nullptr;
	QComboBox *subtitleTracks=nullptr;

	//PlaylistModel *playlistModel=nullptr;
	QAbstractItemView * playlistView=nullptr;

	QString trackInfo_;
	QString statusInfo_;
	qint64 duration_;

	QWidget *metaDataFields[QMediaMetaData::NumMetaData]={};
	QLabel *metaDataLabels[QMediaMetaData::NumMetaData]={};
};
