#pragma once
#include <QTableWidget>

#include <QtWidgets>

class AbstractDownloadTask;
class ElidedTextLabel;
class TaskCellWidget;
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
	TaskCellWidget* cellWidget(int row) const;
	int rowOfCell(TaskCellWidget* cell)const;

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
	Q_OBJECT
public:
	enum State { Stopped, Waiting, Downloading, Finished };
public:
	TaskCellWidget(AbstractDownloadTask* task, QWidget* parent = nullptr);
	~TaskCellWidget();
	static int cellHeight();
	const AbstractDownloadTask* getTask() const { return task; }
	State getState() const { return state; }
	void setState(State state);

	void setWaitState();
	void startDownload();
	void stopDownload();
	void remove();
signals:
	void downloadStopped();
	void downloadFinished();
	void startBtnClicked();
	void removeBtnClicked();
private slots:
	void onErrorOccurred(const QString &errStr);
	void onFinished();
	void open();
private:
	void initProgressWidgets();
	void updateDownloadStats();

	void updateProgressWidgets();
	void updaateStartStopBtn();
	void startStopBtnClicked();

	void startCalcDownRate();
	void stopCalcDownRate();
private:
	State state = Stopped;
	AbstractDownloadTask* task = nullptr;

	QPushButton *iconButton;
	ElidedTextLabel* titleLabel;
	QLabel *qnDescLabel;
	QLabel *progressLabel;

	QProgressBar *progressBar;

	QLabel *downRateLabel;
	QLabel *timeLeftLabel;
	ElidedTextLabel* statusTextLabel;
	QWidget *downloadStatsWidget;
	QStackedWidget *statusStackedWidget;

	QPushButton* startStopButton;
	QPushButton* removebutton;

	QTimer *downRateTimer;
	QList<qint64> downRateWindow;


};

