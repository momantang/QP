#pragma once
#include <QTableWidget>

class AbstractDownloadTask;
class ElidedTextLabel;
class TaskCellWidet;
class TaskTableWidget :public QTableWidget
{
	Q_OBJECT
public:
	explicit TaskTableWidget(QWidget* parent = nullptr);
	void save();
	void load();
	void addTasks(const QList<AbstractDownloadTask*>&, bool activate = true);

	void stopAll();
	void startAll();
	void removeAll();

private slots:
	void onCellTaskStopped();
	void onCellTaskFinished();
	void onCellStartBtnClicked();
	void onCellRemoveBtnClicked();
protected:
	void contextMenuEvent(QContextMenuEvent* event) override;
private:
	TaskCellWidet* cellWidget(int row) const;
	int rowOfCell(TaskCellWidet* cell)const;

	QAction* startAllAct;
	QAction* stopAllAct;
	QAction* removeAllAct;

	bool dirty = false;
	QTimer* saveTasksTimer;
	void setDirty();

	int acitveTaskCnt = 0;
	void activateWaitingTasks();
};

class TaskCellWidget :public QWidget
{
};

