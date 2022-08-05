#pragma once
#include <QAbstractItemModel>
#include <QScopedPointer>

class QMediaPlaylist;
class PlayerListModel :public QAbstractItemModel
{
	Q_OBJECT
public:
	enum Column
	{
		Title = 0,
		ColumnCount
	};
	explicit PlayerListModel(QObject* parent = nullptr);
	~PlayerListModel();

	int rowCount(const QModelIndex& parent) const override;
	int columnCount(const QModelIndex& parent) const override;

	QModelIndex index(int row, int column, const QModelIndex& parent=QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;

	QVariant data(const QModelIndex& index, int role) const override;

	QMediaPlaylist* playlist() const;

	bool setData(const QModelIndex& index, const QVariant& value, int role) override;

private slots:
	void beginInsertItems(int start, int end);
	void endInsertItems();

	void beginRemoveItems(int start, int end);
	void endRemoveItems();

	void changeItems(int start, int end);

private:
	QScopedPointer<QMediaPlaylist> playlist_;
	QMap<QModelIndex, QVariant> data_;
};

