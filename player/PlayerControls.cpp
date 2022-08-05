#include "PlayerControls.h"

PlayerControls::PlayerControls(QWidget* parent)
{
}

QMediaPlayer::PlaybackState PlayerControls::state() const
{
	return QMediaPlayer::PausedState;
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
