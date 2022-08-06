#pragma once
#include <QWidget>
#include <QMediaPlayer>

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QComboBox>
class PlayerControls :public  QWidget
{
	Q_OBJECT
public:
	explicit PlayerControls(QWidget* parent = nullptr);

	QMediaPlayer::PlaybackState state() const;
	float volume() const;
	bool isMuted() const;
	qreal playbackRate() const;
public slots:
	void setState(QMediaPlayer::PlaybackState state);
	void setVolumne(float volumne);
	void setMuted(bool muted);
	void setPlaybackRate(float rate);
signals:
	void play();
	void pause();
	void stop();
	void next();
	void previouse();
	void changeVolume(float volumne);
	void changeMuting(bool muting);
	void changeRate(qreal rate);


private slots:
	void playClicked();
	void muteClicked();
	void updateRate();
	void onVolumeSliderValueChanged();
private:
	QMediaPlayer::PlaybackState playState_ = QMediaPlayer::StoppedState;
	bool playerMuted_ = false;
	QAbstractButton* playButton_ = nullptr;
	QAbstractButton* stopButton_ = nullptr;
	QAbstractButton* nextButton_ = nullptr;
	QAbstractButton* previousButton_ = nullptr;
	QAbstractButton* muteButton_ = nullptr;
	QAbstractSlider* volumeSlider_ = nullptr;
	QComboBox* rateBox_ = nullptr;
};

