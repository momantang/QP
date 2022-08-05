#include "QMediaPlaylist.h"
#include "PlayerListModel.h"

#include <QFileInfo>
#include <QUrl>
PlayerListModel::PlayerListModel(QObject* parent) :QAbstractItemModel(parent)
{
	playlist_.reset(new QMediaPlaylist);
	connect(playlist_.data(), &QMediaPlaylist::mediaAboutToBeInserted, this, &PlayerListModel::beginInsertItems);
	connect(playlist_.data(), &QMediaPlaylist::mediaInserted, this, &PlayerListModel::endInsertItems);
	connect(playlist_.data(), &QMediaPlaylist::mediaAboutToBeRemoved, this, &PlayerListModel::beginRemoveItems);
	connect(playlist_.data(), &QMediaPlaylist::mediaRemove, this, &PlayerListModel::endRemoveItems);
	connect(playlist_.data(), &QMediaPlaylist::mediaChanged, this, &PlayerListModel::changeItems);
}
PlayerListModel::~PlayerListModel()
{
}

void PlayerListModel::beginInsertItems(int start, int end)
{
	data_.clear();
	beginInsertRows(QModelIndex(),start,end);
}

void PlayerListModel::beginRemoveItems(int start, int end)
{
	data_.clear();
	beginRemoveRows(QModelIndex(),start,end);
}

void PlayerListModel::changeItems(int start, int end)
{
	data_.count();
	emit dataChanged(index(start,0),index(end,ColumnCount));
}

int PlayerListModel::columnCount(const QModelIndex& parent) const
{
	return !parent.isValid() ? ColumnCount : 0;
}

QVariant PlayerListModel::data(const QModelIndex& index, int role) const
{
	if (index.isValid()&&role==Qt::DisplayRole)
	{
		QVariant value=data_[index];
		if (!value.isValid()&&index.column()==Title)
		{
			QUrl location=playlist_->media(index.row());
			return QFileInfo(location.path()).fileName();
		}
		return value;
	}
	return QVariant();
}

void PlayerListModel::endInsertItems()
{
	endInsertRows();
}

void PlayerListModel::endRemoveItems()
{
	endInsertRows();
}

QModelIndex PlayerListModel::index(int row, int column, const QModelIndex& parent) const
{
	return (playlist_ && !parent.isValid() && row >= 0 && row < playlist_->mediaCount() && column >= 0 && column < ColumnCount) ? createIndex(row, column) : QModelIndex();
}


QModelIndex PlayerListModel::parent(const QModelIndex& child) const
{
	return QModelIndex();
}



QMediaPlaylist* PlayerListModel::playlist() const
{
	return playlist_.data();
}



int PlayerListModel::rowCount(const QModelIndex& parent) const
{
	return playlist_ && !parent.isValid() ? playlist_->mediaCount() : 0;
}

bool PlayerListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	data_[index]=value;
	emit dataChanged(index,index);
	return true;
}
