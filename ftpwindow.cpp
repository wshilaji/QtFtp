/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include <QtNetwork>
#include <QtFtp>
#include <QDebug>
#include "ftpwindow.h"

FtpWindow::FtpWindow(QWidget *parent)
    : QDialog(parent), ftp(0), networkSession(0)
{
    ftpServerLabel = new QLabel(tr("Ftp &server:"));
    ftpServerLineEdit = new QLineEdit("ftp://tools:tools@192.168.1.104");


    ftpServerLabel->setBuddy(ftpServerLineEdit);

    statusLabel = new QLabel(tr("Please enter the name of an FTP server."));

    fileList = new QTreeWidget;
    fileList->setEnabled(false);
    fileList->setRootIsDecorated(false);
    fileList->setHeaderLabels(QStringList() << tr("Name") << tr("Size") << tr("Owner") << tr("Group") << tr("Time"));
    fileList->header()->setStretchLastSection(false);
    fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);


    connectButton = new QPushButton(tr("Connect"));
    connectButton->setDefault(true);

    cdToParentButton = new QPushButton;
    cdToParentButton->setIcon(QPixmap(":/images/cdtoparent.png"));
    cdToParentButton->setEnabled(false);

    uploadButton = new QPushButton(tr("upload"));
    uploadButton->setEnabled(false);


    downloadButton = new QPushButton(tr("Download"));
    downloadButton->setEnabled(false);

    quitButton = new QPushButton(tr("Quit"));

    buttonBox = new QDialogButtonBox;

    buttonBox->addButton(uploadButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(downloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    progressDialog = new QProgressDialog(this);
//    progressDialog->hide();
//    progressDialog->setVisible(0);

    connect(fileList, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(processItem(QTreeWidgetItem*,int)));
//    connect(fileList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
//            this, SLOT(enableUpOrDownloadButton()));
    connect(fileList, SIGNAL(itemSelectionChanged()), this, SLOT(enableUpOrDownloadButton()));
    //检测点击事件
    connect(fileList,SIGNAL(itemClicked(QTreeWidgetItem*,int)),this,SLOT(itemClick(QTreeWidgetItem*,int)));
        //检测鼠标右键
    fileList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fileList,SIGNAL(customContextMenuRequested(const QPoint&)), this,SLOT(popMenu(const QPoint&)));
    //rename
    //QObject::connect(fileList,SIGNAL(itemSelectionChanged ()),this,SLOT(slotSelectionChanged()));



//    QObject::connect(fileList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
//                     [=](QTreeWidgetItem *current, QTreeWidgetItem *previous) {fileList->closePersistentEditor(temItem, temColumn ); });
    // ;



    connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelDownload()));
    connect(connectButton, SIGNAL(clicked()), this, SLOT(connectOrDisconnect()));
    connect(cdToParentButton, SIGNAL(clicked()), this, SLOT(cdToParent()));
    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadFile()));
     connect(uploadButton, SIGNAL(clicked()), this, SLOT(uploadFile()));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *topLayout = new QHBoxLayout;//horizontal
    topLayout->addWidget(ftpServerLabel);
    topLayout->addWidget(ftpServerLineEdit);
    topLayout->addWidget(cdToParentButton);
    topLayout->addWidget(connectButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;//vertical
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(fileList);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setWindowTitle(tr("FTP"));
}

QSize FtpWindow::sizeHint() const
{
    return QSize(500, 300);
}

//![0]
void FtpWindow::connectOrDisconnect()
{
    if (ftp) {
        ftp->abort();
        ftp->deleteLater();
        ftp = 0;
//![0]
        fileList->setEnabled(false);
        cdToParentButton->setEnabled(false);
        downloadButton->setEnabled(false);
        connectButton->setEnabled(true);
        connectButton->setText(tr("Connect"));
#ifndef QT_NO_CURSOR
        setCursor(Qt::ArrowCursor);
#endif
        statusLabel->setText(tr("Please enter the name of an FTP server."));
        return;
    }

#ifndef QT_NO_CURSOR
    setCursor(Qt::WaitCursor);
#endif

    if (!networkSession || !networkSession->isOpen()) {
        if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
            if (!networkSession) {
                // Get saved network configuration
                QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
                settings.beginGroup(QLatin1String("QtNetwork"));
                const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
                settings.endGroup();

                // If the saved network configuration is not currently discovered use the system default
                QNetworkConfiguration config = manager.configurationFromIdentifier(id);
                if ((config.state() & QNetworkConfiguration::Discovered) !=
                    QNetworkConfiguration::Discovered) {
                    config = manager.defaultConfiguration();
                }

                networkSession = new QNetworkSession(config, this);
                connect(networkSession, SIGNAL(opened()), this, SLOT(connectToFtp()));
                connect(networkSession, SIGNAL(error(QNetworkSession::SessionError)), this, SLOT(enableConnectButton()));
            }
            connectButton->setEnabled(false);
            statusLabel->setText(tr("Opening network session."));
            networkSession->open();
            return;
        }
    }
    connectToFtp();
}

void FtpWindow::connectToFtp()
{
//![1]
    ftp = new QFtp(this);
    connect(ftp, SIGNAL(commandFinished(int,bool)),
            this, SLOT(ftpCommandFinished(int,bool)));
    connect(ftp, SIGNAL(listInfo(QUrlInfo)),
            this, SLOT(addToList(QUrlInfo)));
    connect(ftp, SIGNAL(dataTransferProgress(qint64,qint64)),
            this, SLOT(updateDataTransferProgress(qint64,qint64)));

    fileList->clear();
    currentPath.clear();
    isDirectory.clear();
//![1]

//![2]
    QUrl url(ftpServerLineEdit->text());

    if (!url.isValid() || url.scheme().toLower() != QLatin1String("ftp")) {
        ftp->connectToHost(ftpServerLineEdit->text(), 21);
        ftp->login();
    } else {
        ftp->connectToHost(url.host(), url.port(21));

        if (!url.userName().isEmpty())
            ftp->login(QUrl::fromPercentEncoding(url.userName().toLatin1()), url.password());
        else
            ftp->login();
        if (!url.path().isEmpty())
            ftp->cd(url.path());
    }
//![2]

    fileList->setEnabled(true);
    connectButton->setEnabled(false);
    connectButton->setText(tr("Disconnect"));
    statusLabel->setText(tr("Connecting to FTP server %1...")
                         .arg(ftpServerLineEdit->text()));
}

//![3]
void FtpWindow::downloadFile()
{
    QString fileName = fileList->currentItem()->text(0);
    qDebug() << "fileList->currentItem()->text(0):" << fileName;


//![3]

    if (QFile::exists(fileName)) {
        QMessageBox::information(this, tr("FTP"),
                                 tr("There already exists a file called %1 in "
                                    "the current directory.")
                                 .arg(fileName));
        return;
    }

//![4]
    file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("FTP"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(fileName).arg(file->errorString()));
        delete file;
        return;
    }

    ftp->get(_ToSpecialEncoding(fileList->currentItem()->text(0)), file);
    //class Q_CORE_EXPORT QFileDevice : public QIODevice
    //int QFtp::get(const QString &file, QIODevice *dev, TransferType type)

    progressDialog->setLabelText(tr("Downloading %1...").arg(fileName));
    downloadButton->setEnabled(false);
    progressDialog->exec();
}
//![4]

//![5]
void FtpWindow::cancelDownload()
{
    ftp->abort();

    if (file->exists()) {
        file->close();
        file->remove();
    }
    delete file;
}
//![5]

//![6]
void FtpWindow::ftpCommandFinished(int, bool error)
{
#ifndef QT_NO_CURSOR
    setCursor(Qt::ArrowCursor);
#endif

    if (ftp->currentCommand() == QFtp::ConnectToHost) {
        if (error) {
            QMessageBox::information(this, tr("FTP"),
                                     tr("Unable to connect to the FTP server "
                                        "at %1. Please check that the host "
                                        "name is correct.")
                                     .arg(ftpServerLineEdit->text()));
            connectOrDisconnect();
            return;
        }
        statusLabel->setText(tr("Logged onto %1.")
                             .arg(ftpServerLineEdit->text()));
        fileList->setFocus();
        downloadButton->setDefault(true);
        connectButton->setEnabled(true);
        return;
    }
//![6]

//![7]
    if (ftp->currentCommand() == QFtp::Login)
        ftp->list();
    if (ftp->currentCommand() == QFtp::Rename)
        if (createFolder) statusLabel->setText(QStringLiteral("rename suceess"));
                createFolder = false;
//![7]
    if (ftp->currentCommand() == QFtp::Put) {
        if (error) {
            statusLabel->setText(tr("Canceled upload of %1.")
                                 .arg(file->fileName()));
            file->close();
            //file->remove();//不是会从ftp remove而是从本地删除
        } else {
            statusLabel->setText(tr("upload %1 to current directory.")
                                 .arg(file->fileName()));
            file->close();
        }
        delete file;
        enableUpOrDownloadButton();
        progressDialog->hide();
    }
//![8]
    if (ftp->currentCommand() == QFtp::Get) {
        if (error) {
            statusLabel->setText(tr("Canceled download of %1.")
                                 .arg(file->fileName()));
            file->close();
            file->remove();
        } else {
            statusLabel->setText(tr("Downloaded %1 to current directory.")
                                 .arg(file->fileName()));
            file->close();
        }
        delete file;
        enableUpOrDownloadButton();
        progressDialog->hide();
//![8]
//![9]
    } else if (ftp->currentCommand() == QFtp::List) {
        if (isDirectory.isEmpty()) {
            fileList->addTopLevelItem(new QTreeWidgetItem(QStringList() << tr("<empty>")));
            fileList->setEnabled(false);
        }
    }
//![9]
}

//![10]
void FtpWindow::addToList(const QUrlInfo &urlInfo)
{
    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText(0, _FromSpecialEncoding(urlInfo.name()));
    //item->setText(0, urlInfo.name());
    item->setText(1, QString::number(urlInfo.size()));
    item->setText(2, urlInfo.owner());
    item->setText(3, urlInfo.group());
    item->setText(4, urlInfo.lastModified().toString("MMM dd yyyy"));

    QPixmap pixmap(urlInfo.isDir() ? ":/images/dir.png" : ":/images/file.png");
    item->setIcon(0, pixmap);
    //isDirectory[urlInfo.name()] = urlInfo.isDir();
    isDirectory[_FromSpecialEncoding(urlInfo.name())] = urlInfo.isDir();


    fileList->addTopLevelItem(item);
    if (!fileList->currentItem()) {
        fileList->setCurrentItem(fileList->topLevelItem(0));
        fileList->setEnabled(true);
    }
}
//![10]

//![11]
void FtpWindow::processItem(QTreeWidgetItem *item, int /*column*/)
{
    //必须是原乱码路径 才能 导入ftp->cd(name);疑问
    //QString name = _ToSpecialEncoding(item->text(0));
    QString name = item->text(0);
    if (isDirectory.value(name)) {
        fileList->clear();
        isDirectory.clear();
        currentPath += '/';
        //currentPath += name;
        currentPath += _ToSpecialEncoding(name);
        ftp->cd(_ToSpecialEncoding(name));
        ftp->list();
        cdToParentButton->setEnabled(true);
#ifndef QT_NO_CURSOR
        setCursor(Qt::WaitCursor);
#endif
        return;
    }
}
//![11]

//![12]
void FtpWindow::cdToParent()
{
#ifndef QT_NO_CURSOR
    setCursor(Qt::WaitCursor);
#endif
    fileList->clear();
    isDirectory.clear();
    currentPath = currentPath.left(currentPath.lastIndexOf('/'));
    if (currentPath.isEmpty()) {
        cdToParentButton->setEnabled(false);
        ftp->cd("/");
    } else {
        ftp->cd(currentPath);
    }
    ftp->list();
}
//![12]

//![13]
void FtpWindow::updateDataTransferProgress(qint64 readBytes,
                                           qint64 totalBytes)
{
    //qDebug() << "loaded" << readBytes << "of" << totalBytes;

    progressDialog->setMaximum(totalBytes);
    progressDialog->setValue(readBytes);
}
//![13]

//![14]
void FtpWindow::enableUpOrDownloadButton()
{
    QTreeWidgetItem *current = fileList->currentItem();
    if (current) {
        QString currentFile = current->text(0);
        downloadButton->setEnabled(!isDirectory.value(currentFile));
        uploadButton->setEnabled(isDirectory.value(currentFile));
    } else {
        downloadButton->setEnabled(false);
         uploadButton->setEnabled(false);
    }
}

//![14]

//![15]
void FtpWindow::enableConnectButton()
{
    // Save the used configuration
    QNetworkConfiguration config = networkSession->configuration();
    QString id;
    if (config.type() == QNetworkConfiguration::UserChoice)
        id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
    else
        id = config.identifier();

    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    settings.beginGroup(QLatin1String("QtNetwork"));
    settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
    settings.endGroup();

    connectButton->setEnabled(true);
    statusLabel->setText(tr("Please enter the name of an FTP server."));
}
//![15]

//![16]
QString FtpWindow::getFileName(QString m_filePath)
{
    QString temp;
    QString fileName;
    int count = -1;
    fileName = m_filePath;
    for (int i = 0; temp != "/"; i++)
    {
        temp = fileName.right(1);
        fileName.chop(1);
        count++;
    }
    fileName = m_filePath.right(count);
    //std::cout<< m_filePath.toStdString();	//C:/Users/dys/Desktop/sobelself.png
    // std::cout << fileName.toStdString();	//sobelself.png

    return fileName;
}
//![16]
//![17]
void FtpWindow::uploadFile()
{
    /*         QString filename;
    filename = QFileDialog::getOpenFileName()
    */
    //得到选择的文件的路径，保存在字符串链表中
    QStringList string_list;
    string_list = QFileDialog::getOpenFileNames(this, tr("选择文件"), "", tr("Files (*)"));
    if (!string_list.isEmpty()) {
        for (int i = 0; i < string_list.count(); i++) {
            QString filePath;
            filePath = string_list.at(i);

            QString name = filePath.mid(filePath.lastIndexOf("/") + 1);

            qDebug() << "filePath.mid(filePath.lastIndexOf(/) + 1):" << name;

           file = new QFile(filePath);

           if (!file->open(QIODevice::ReadOnly)) {
               QMessageBox::critical( this, tr("Upload error"),
                 tr("Can't open file '%1' for reading.").arg(name) );
                 delete file;
                 return;
               }

//           QString path = QString("%1/%2").arg(currentPath).arg(name);
           QByteArray data = file->readAll();
            //ftp->put(file, path);
            ftp->put(data, _ToSpecialEncoding(name));
            progressDialog->setLabelText(tr("uploading %1...").arg(name));
            uploadButton->setEnabled(false);
            progressDialog->exec();
        }
    }

}
//![17]
QString FtpWindow::_FromSpecialEncoding(const QString &InputStr) {
    #ifdef Q_OS_WIN
        return QString::fromLocal8Bit(InputStr.toLatin1());
    #else
        QTextCodec *codec = QTextCodec::codecForName("gbk");
        if (codec) {
            return codec->toUnicode(InputStr.toLatin1());
        } else {
            return QString("");
        }
    #endif
}

QString FtpWindow::_ToSpecialEncoding(const QString &InputStr) {
    #ifdef Q_OS_WIN
        return QString::fromLatin1(InputStr.toLocal8Bit());
    #else
        QTextCodec *codec= QTextCodec::codecForName("gbk");
        if (codec) {
            return QString::fromLatin1(codec->fromUnicode(InputStr));
        } else {
            return QString("");
        }
    #endif
}


void FtpWindow::itemClick(QTreeWidgetItem *item, int column)
{
    //qDebug()<<"curItem->text(0):"<<item->text(0);
     //qDebug()<< _ToSpecialEncoding(item->text(0));
     //item->setFlags(item->flags() | Qt::ItemIsEditable);
     //qDebug()<<"currentPath"<<currentPath;
    //qDebug()<<"itemClick() happen";

}

void FtpWindow::onRemove()
{
    int row = fileList->currentIndex().row();
        // 如果是多级目录，则选中的第一级就不给重命名
        if (currentPath.indexOf("/") >= 0 && row <= 0) return;
        // 解决中文乱码问题
          QString name =fileList->currentItem()->text(0);

            //qDebug()<<"fileList->currentIndex().row()"<<row; row() 2

          //T &operator[](const Key &key);
        if (isDirectory[name]) ftp->rmdir(QString::fromLatin1(name.toLocal8Bit()));
        else                ftp->remove(QString::fromLatin1(name.toLocal8Bit()));
        qDebug()<<"isDirectory[name]"<<isDirectory[name]<<",key="<<name;
        isDirectory.remove(name);

}
void FtpWindow::onRefresh()
{
    fileList->clear();
    isDirectory.clear();
    ftp->list();
}
void FtpWindow::onRename()
{
    int row = fileList->currentIndex().row();
        // 如果是多级目录，则选中的第一级就不给重命名
        if (currentPath.indexOf("/") >= 0 && row <= 0) return;//file..
    QTreeWidgetItem *item = fileList->currentItem();
    mutex.lock();
    oldname = item->text(0);//0 col  user group   MMM dd yyyy
    mutex.unlock();
       fileList->openPersistentEditor(item);    // 打开编辑
        fileList->editItem(item);//default:int column = 0
        QObject::connect(fileList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),this,SLOT(slotCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

}

void FtpWindow::slotSelectionChanged()
{
    qDebug()<<"slotSelectionChanged() happen";
}
//void FtpWindow::slotCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
//{
//     qDebug()<<"slotCurrentItemChanged() happen";

//}

void FtpWindow::slotCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    // qDebug()<<"slotCurrentItemChanged() happen";
    // QTreeWidgetItem *item = fileList->currentItem();
     fileList->closePersistentEditor(previous);
      QString newname =previous->text(0);
      qDebug()<<"previous->text(0):"<<newname;
      QTreeWidgetItemIterator it(fileList);
      while (*it) {
           //do something like
          //QString name=(*it)->text(0);

          if (((*it)->text(0) == newname) && (isDirectory[oldname] == isDirectory[newname]))//file and dir可以同名
          {

              if (createFolder)  statusLabel->setText(QStringLiteral("文件名已存在"));
              QMessageBox::warning(NULL,"warning","Name cannot be duplicate");
              previous->setText(0,oldname);
              createFolder = false;
              QObject::disconnect(fileList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),this,SLOT(slotCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
              return;
          }
          ++it;
      }
      ftp->rename(QString::fromLatin1(oldname.toLocal8Bit()),QString::fromLatin1(newname.toLocal8Bit()));

      isDirectory[newname] == isDirectory[oldname];
      isDirectory.remove(oldname);
      QObject::disconnect(fileList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),this,SLOT(slotCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

}

void FtpWindow::onCreateFolder()
{
    //QString name = createFolderName();//判断是否有重复的
    QString name = "NewFolder";

    QTreeWidgetItem *item = new QTreeWidgetItem;
    item->setText(0, _FromSpecialEncoding(name));
    //item->setText(0, urlInfo.name());
    item->setText(1, "0");
    item->setText(2, "user");
    item->setText(3, "group");
    item->setText(4, QDateTime::currentDateTime().toString("MMM dd yyyy"));

    QPixmap pixmap( ":/images/dir.png");
    item->setIcon(0, pixmap);

    isDirectory[name] =true;
    fileList->addTopLevelItem(item);
    if (!fileList->currentItem()) {
        fileList->setCurrentItem(fileList->topLevelItem(0));
        fileList->setEnabled(true);
    }

    createFolder = true;
    ftp->mkdir(_ToSpecialEncoding(name));
    mutex.lock();
    oldname = name;//0 col  user group   MMM dd yyyy
    mutex.unlock();
    // 打开编辑

    fileList->openPersistentEditor(item);
    fileList->editItem(item);
    QObject::connect(fileList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),this,SLOT(slotCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));


}
//弹出菜单
void FtpWindow::popMenu(const QPoint &)
{
   // closePersistentEditor();
    QTreeWidgetItem* curItem=fileList->currentItem();
   /*
    if(curItem == NULL || curItem->parent() == NULL)
        return; */          //右键的位置在空白位置右击或者点击的是顶层item

    QMenu menu;

    menu.addAction("upload", this,  SLOT(uploadFile()));
    menu.addAction("download", this, SLOT(downloadFile()));
    menu.addSeparator();
    menu.addAction(QIcon(":/images/dir.png"), "CreateNewFolder", this, SLOT(onCreateFolder()));
    menu.addSeparator();
    menu.addAction(QStringLiteral ("rename"), this, SLOT(onRename()));
    menu.addAction(QStringLiteral ("Remove"), this, SLOT(onRemove()));
    menu.addAction("Refresh", this, SLOT(onRefresh()));

    menu.exec(QCursor::pos());  //在当前鼠标位置显示
}




