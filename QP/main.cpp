#include "QP.h"
#include <QtWidgets/QApplication>
#include <QSharedPointer>
#include <QSharedMemory>
#include "MainWindow.h"
#ifdef Q_OS_WIN1
#include <Windows.h>
void raiseWindow(const HWND hwnd)
{
	WINDOWPLACEMENT placement;
    GetWindowPlacement(hwnd,&placement);
	if (placement.showCmd==SW_SHOWMINIMIZED)
	{
		ShowWindow(hwnd,SW_RESTORE);
	}else
	{
		SetForegroundWindow(hwnd);
	}
}

int main(int argc,char *argv[])
{
	QApplication app(argc,argv);
	QSharedMemory shareMem("B23Dlg_HWD");

	auto setHwnd=[&shareMem](HWND hWnd)
	{
		shareMem.lock();
		auto ptr=static_cast<HWND*> (shareMem.data());
		*ptr=hWnd;
		shareMem.unlock();
	};
	auto getHwnd=[&shareMem]()->HWND
	{
		shareMem.attach(QSharedMemory::ReadOnly);
		shareMem.lock();
		HWND hwnd=*static_cast<const HWND*>(shareMem.constData());
		shareMem.unlock();
		return hwnd;
	};
	bool isNoAppAlreadyExist=shareMem.create(sizeof(HWND));
	if (isNoAppAlreadyExist)
	{
		MainWindow w;
		setHwnd(HWND(w.winId()));
		w.show();
		return app.exec();
	}else
	{
		raiseWindow(getHwnd());
	}
}
#else
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
#endif
