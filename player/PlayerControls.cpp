#include "PlayerControls.h"

#include <QBoxLayout>
#include <QSlider>
#include <QStyle>
#include <QToolButton>
PlayerControls::PlayerControls(QWidget* parent) :QWidget(parent)
{
	playButton_ = new QToolButton(this);
	playButton_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

	connect(playButton_, &QAbstractButton::clicked, this, &PlayerControls::playClicked);
	stopButton_ = new QToolButton(this);
	stopButton_->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
	stopButton_->setEnabled(false);

	connect(stopButton_, &QAbstractButton::clicked, this, &PlayerControls::stop);

	nextButton_ = new QToolButton(this);
	nextButton_->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));

	connect(nextButton_, &QAbstractButton::clicked, this, &PlayerControls::next);

	previousButton_=new QToolButton(this);
	previousButton_->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
	connect(previousButton_,&QAbstractButton::clicked,this,&PlayerControls::previouse);

	muteButton_=new QToolButton(this);
	muteButton_->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
	connect(muteButton_,&QAbstractButton::clicked,this,&PlayerControls::muteClicked);

	volumeSlider_=new QSlider(Qt::Horizontal,this);
	volumeSlider_->setRange(0,100);
	connect(volumeSlider_,&QSlider::valueChanged,this,&PlayerControls::onVolumeSliderValueChanged );

	rateBox_=new QComboBox(this);
	rateBox_->addItem("0.5x",QVariant(0.5));
	rateBox_->addItem("1.0x",QVariant(1.0));
	rateBox_->addItem("2.0x",QVariant(2.0));
	rateBox_->setCurrentIndex(1);

	connect(rateBox_,QOverload<int>::of(&QComboBox::activated),this,&PlayerControls::updateRate);

	QBoxLayout *layout=new QHBoxLayout;
	layout->setContentsMargins(0,0,0,0);
	layout->addWidget(stopButton_);
	layout->addWidget(previousButton_);
	layout->addWidget(playButton_);
	layout->addWidget(nextButton_);
	layout->addWidget(muteButton_);
	layout->addWidget(volumeSlider_);
	layout->addWidget(rateBox_);
	setLayout(layout);
}

QMediaPlayer::PlaybackState PlayerControls::state() const
{
	return playState_;
}

float PlayerControls::volume() const
{
	return 0.0f;
}

bool PlayerControls::isMuted() const
{
	return false;
}

qreal PlayerControls::playbackRate() const
{
	return 0;
}

void PlayerControls::setState(QMediaPlayer::PlaybackState state)
{
}

void PlayerControls::setVolumne(float volumne)
{
}

void PlayerControls::setMuted(bool muted)
{
}

void PlayerControls::setPlaybackRate(float rate)
{
}

void PlayerControls::playClicked()
{
}

void PlayerControls::muteClicked()
{
}

void PlayerControls::updateRate()
{
}

void PlayerControls::onVolumeSliderValueChanged()
{
}
