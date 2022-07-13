#include "TaskTable.h"
#include "DownloadTask.h"
#include "Settings.h"
#include "utils.h"

#include <QtWidgets>
static constexpr int MaxConcurrentTaskCount = 3;
static constexpr int SaveTasksInterval = 5000; // ms

static constexpr int DownRateTimerInterval = 500; // ms
static constexpr int DownRateWindowLength = 10;

TaskTableWidget::TaskTableWidget(QWidget* parent) :QTableWidget(parent)
{
	horizontalHeader()->hide();
	verticalHeader()->hide();
	setColumnCount(1);
	horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	setFocusPolicy(Qt::NoFocus);
	setStyleSheet("QTableWidget{selection-backgroud-color:rgb(227,227,229);");

	startAllAct = new QAction("全部开始");
	stopAllAct = new QAction("全部暂停");
	removeAllAct = new QAction("全部删除");

	connect(startAllAct, &QAction::triggered, this, &TaskTableWidget::startAll);
	connect(stopAllAct, &QAction::triggered, this, &TaskTableWidget::stopAll);
	connect(removeAllAct, &QAction::triggered, this, &TaskTableWidget::removeAll);

	saveTasksTimer = new QTimer(this);
	saveTasksTimer->setInterval(SaveTasksInterval);
	saveTasksTimer->setSingleShot(false);
	connect(saveTasksTimer, &QTimer::timeout, this, &TaskTableWidget::save);

}

void TaskTableWidget::save()
{
}

void TaskTableWidget::load()
{
	auto settings = Settings::inst();
	auto array = QJsonDocument::fromJson(settings->value("tasks").toByteArray()).array();
	QList<AbstractDownloadTask* > tasks;
	for (auto obj : array)
	{
		auto task = AbstractDownloadTask::fromJsonObj(obj.toObject());
		if (task != nullptr)
		{
			tasks.append(task);
		}
	}
	addTasks(tasks, false);
}

void TaskTableWidget::addTasks(const QList<AbstractDownloadTask*>& tasks, bool activate)
{
	auto shouldSetDirty = false;
	auto rowHt = TaskCellWidget::cellHeight();
	for (auto task : tasks)
	{
		auto cell = new TaskCellWidget(task);
		int idx = rowCount();
		insertRow(idx);
		setRowHeight(idx, rowHt);
		setCellWidget(idx, 0, cell);

		connect(cell, &TaskCellWidget::downloadStopped, this, &TaskTableWidget::onCellTaskStopped);
		connect(cell, &TaskCellWidget::downloadFinished, this, &TaskTableWidget::onCellTaskFinished);
		connect(cell, &TaskCellWidget::startBtnClicked, this, &TaskTableWidget::onCellStartBtnClicked);
		connect(cell, &TaskCellWidget::removeBtnClicked, this, &TaskTableWidget::onCellRemoveBtnClicked);

		if (activate)
		{
			if (acitveTaskCnt<MaxConcurrentTaskCount)
			{
				acitveTaskCnt++;
				shouldSetDirty=true;
				cell->startDownload();
			}else
			{
				cell->setWaitState();
			}
		}
	}
	if (shouldSetDirty)
	{
		setDirty();
	}
}

void TaskTableWidget::stopAll()
{
}

void TaskTableWidget::startAll()
{
}

void TaskTableWidget::removeAll()
{
}

void TaskTableWidget::onCellTaskStopped()
{
}

void TaskTableWidget::onCellTaskFinished()
{
}

void TaskTableWidget::onCellStartBtnClicked()
{
}

void TaskTableWidget::onCellRemoveBtnClicked()
{
}

void TaskTableWidget::contextMenuEvent(QContextMenuEvent* event)
{
	QTableWidget::contextMenuEvent(event);
}

TaskCellWidget* TaskTableWidget::cellWidget(int row) const
{
	TaskCellWidget *t=nullptr;
	return t;
}

int TaskTableWidget::rowOfCell(TaskCellWidget* cell) const
{
	return 0;
}

void TaskTableWidget::setDirty()
{
	if (dirty)
	{
		return;
	}
	dirty=true;
	if (!saveTasksTimer->isActive())
	{
		saveTasksTimer->start();
	}
}

void TaskTableWidget::activateWaitingTasks()
{
}

TaskCellWidget::TaskCellWidget(AbstractDownloadTask* task, QWidget* parent)
{
}

TaskCellWidget::~TaskCellWidget()
{
}

int TaskCellWidget::cellHeight()
{
	return 0;
}

void TaskCellWidget::setState(State state)
{
}

void TaskCellWidget::setWaitState()
{
}

void TaskCellWidget::startDownload()
{
}

void TaskCellWidget::stopDownload()
{
}

void TaskCellWidget::remove()
{
}

void TaskCellWidget::onErrorOccurred(const QString& errStr)
{
}

void TaskCellWidget::onFinished()
{
}

void TaskCellWidget::open()
{
}

void TaskCellWidget::initProgressWidgets()
{
}

void TaskCellWidget::updateDownloadStats()
{
}

void TaskCellWidget::updateProgressWidgets()
{
}

void TaskCellWidget::updaateStartStopBtn()
{
}

void TaskCellWidget::startStopBtnClicked()
{
}

void TaskCellWidget::startCalcDownRate()
{
}

void TaskCellWidget::stopCalcDownRate()
{
}
