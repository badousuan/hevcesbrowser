#include "MainWindow.h"

#include <QToolBar>
#include <QAction>
#include <QIcon>
#include <QFileDialog>
#include <QMessageBox>
#include <QScopedArrayPointer>
#include <QDebug>
#include <QScrollArea>
#include <QLabel>
#include <QFile>
#include <QByteArray>
#include <QSettings>
#include <QDir>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QCoreApplication>
#include <QProcess>
#include <QProgressDialog>


#include "CentralWidget.h"
#include "WarningsViewer.h"
#include "StreamInfoViewer.h"
#include "HDRInfoViewer.h"
#include "ProfileConformanceAnalyzer.h"

#include "version_info.h"

#include <HevcParser.h>

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags):
  QMainWindow(parent, flags)
  ,m_pwarnViewer(new WarningsViewer)
  ,m_pinfoViewer(new StreamInfoViewer)
  ,m_phdrInfoViewer(new HDRInfoViewer)
{
  QToolBar *ptb = addToolBar("Menu");

  QAction *pactOpen = ptb -> addAction("Open...");
  pactOpen -> setShortcut(QKeySequence("Ctrl+O"));
  connect(pactOpen, SIGNAL(triggered()), SLOT(slotOpen()));

  QAction *pactShowWarn = ptb -> addAction("Warnings...");
  connect(pactShowWarn, SIGNAL(triggered()), SLOT(slotShowWarningsViewer()));

  QAction *pactShowInfo = ptb -> addAction("Info...");
  connect(pactShowInfo, SIGNAL(triggered()), SLOT(slotShowInfoViewer()));

  QAction *pactShowHDRInfo = ptb -> addAction("HDR Info...");
  connect(pactShowHDRInfo, SIGNAL(triggered()), SLOT(slotShowHDRInfoViewer()));


  QMenu *pmenu = menuBar() -> addMenu("&File");
  pmenu -> addAction(pactOpen);
  addAction (pactOpen);

  pmenu -> addSeparator();
  pmenu -> addAction(pactShowWarn);
  pmenu -> addAction(pactShowInfo);
  pmenu -> addAction(pactShowHDRInfo);

  pmenu -> addSeparator();
  pmenu -> addAction("Exit", this, SLOT(close()));

  pmenu = menuBar() -> addMenu("&Help");
  pmenu -> addAction ("About HEVCESBrowser...", this, SLOT(slotAbout()));

  setAcceptDrops(true);


  CentralWidget *pwgt = new CentralWidget(this);
  setCentralWidget(pwgt);

  readCustomData();
}


void MainWindow::process(const QString &fileName)
{
  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly))
  {
    QMessageBox::critical(this, "File opening problem", "Problem with open file `" + fileName + "`for reading");
    return;
  }

  HEVC::Parser *pparser = HEVC::Parser::create();
  CentralWidget *pcntwgt = dynamic_cast<CentralWidget *>(centralWidget());
  pparser -> addConsumer(pcntwgt -> m_pcomInfoViewer.data());
  pparser -> addConsumer(dynamic_cast<WarningsViewer *> (m_pwarnViewer));
  pparser -> addConsumer(dynamic_cast<StreamInfoViewer *> (m_pinfoViewer));
  pparser -> addConsumer(dynamic_cast<HDRInfoViewer *> (m_phdrInfoViewer));

  ProfileConformanceAnalyzer profConfAnalyzer;
  profConfAnalyzer.m_pconsumer = dynamic_cast<WarningsViewer *> (m_pwarnViewer);
  pparser -> addConsumer(&profConfAnalyzer);

  std::size_t position = 0;

  QProgressBar *pprogressBar = new QProgressBar(NULL);
  pprogressBar -> setWindowTitle("Opening...");
  pprogressBar -> setMinimum(0);
  pprogressBar -> setMaximum(100);
  pprogressBar -> setValue(0);

  std::size_t fileSize = file.size();

  Qt::WindowFlags flags = pprogressBar -> windowFlags();

  pprogressBar -> show();

  while(!file.atEnd())
  {
    QByteArray arr = file.read(4 * (1 << 20));
    std::size_t parsed = pparser -> process((const uint8_t *)arr.data(), arr.size(), position);
    position += parsed;
    if(file.atEnd())
    {
      break;
    }

    file.seek(position);
    pprogressBar -> setValue(position * 100 / fileSize);
    QCoreApplication::processEvents();

    if(!pprogressBar -> isVisible())
    {
      break;
    }
  }

  if(pcntwgt -> m_pcomInfoViewer -> rowCount() > 0)
  {
    QString lastOffset = pcntwgt -> m_pcomInfoViewer -> item(pcntwgt -> m_pcomInfoViewer -> rowCount() - 1, 0) -> text();
    std::size_t start = lastOffset.indexOf('(');
    std::size_t end = lastOffset.lastIndexOf(')');
    lastOffset = lastOffset.mid(start + 1);
    lastOffset.remove(")");
    std::size_t lastNalUOffset = lastOffset.toULongLong(NULL);
    pcntwgt -> m_pcomInfoViewer -> item(pcntwgt -> m_pcomInfoViewer -> rowCount() - 1, 1) -> setText(QString::number(position - lastNalUOffset));
  }
  file.seek(0);
  pcntwgt -> m_phexViewer -> setData((QHexView::DataStorage *)new QHexView::DataStorageArray(file.readAll()));

  pprogressBar -> close();
  delete pprogressBar;

  HEVC::Parser::release(pparser);
  file.close();
}


void MainWindow::slotOpen()
{
  QString dir = QDir::currentPath();
  QSettings settings("HEVCESBrowser", "HEVCESBrowser");

  if(settings.value("MainWindow/PrevDir").toString().length())
  {
    dir = settings.value("MainWindow/PrevDir").toString();
  }

  QString fileName = QFileDialog::getOpenFileName(this, "HEVC ES File", dir);
  openFile(fileName);
}

void MainWindow::openFile(const QString &fileName)
{
    QFileInfo info(fileName);

    if(!fileName.isEmpty() && info.exists() && info.isFile())
    {
      QSettings settings("HEVCESBrowser", "HEVCESBrowser");
      CentralWidget *pcntwgt = dynamic_cast<CentralWidget *>(centralWidget());
      pcntwgt -> m_pcomInfoViewer -> clear();
      pcntwgt -> m_psyntaxViewer -> clear();
      dynamic_cast<WarningsViewer *> (m_pwarnViewer) -> clear();
      dynamic_cast<StreamInfoViewer *> (m_pinfoViewer) -> clear();
      dynamic_cast<HDRInfoViewer *> (m_phdrInfoViewer) -> clear();

      QString input_fileName = fileName;
      QString tmp265File = "";
      if(!fileName.endsWith(".265") && !fileName.endsWith(".h265")) {

          QProcess process;
          QString program = "ffmpeg";
          QStringList arguments;
          tmp265File = fileName+"_output.265";
          arguments << "-i" << fileName << "-an" << "-vcodec" << "copy" << tmp265File << "-y";  // FFmpeg 参数
          process.start(program, arguments);

          // 创建进度对话框
          QProgressDialog progressDialog("正在加载，请稍候...", "取消", 0, 0);
          progressDialog.setWindowModality(Qt::WindowModal);
          progressDialog.setMinimumDuration(0); // 立即显示
          progressDialog.show();

          // 等待进程完成
          if (process.waitForFinished()) {
              process.close();
              progressDialog.close(); // 关闭加载对话框
              // 检查进程退出状态
              if (process.exitStatus() != QProcess::NormalExit) {
                  QMessageBox::critical(this, "File convert to hevc nal stream faild", "Problem with open file `" + fileName + "`for reading");
                  if(!tmp265File.isEmpty()) {
                      QFile::remove(tmp265File);
                  }

                  qCritical() <<"ffmpeg convert failed\n";
                  return;
              } else {

                   QFileInfo tmpFinfo(tmp265File);
                   if (tmpFinfo.size() == 0) {
                       if(!tmp265File.isEmpty()) {
                           QFile::remove(tmp265File);
                       }
                       QMessageBox::critical(this, "File convert to hevc nal stream faild", "Problem with open file `" + fileName + "`for reading");
                       return;
                   }
                   input_fileName = tmp265File;
                   qInfo() <<"ffmpeg convert success\n";
              }
          } else {
              QMessageBox::critical(this, "FFmpeg not found, install it first", "Problem with open file `" + fileName + "`for reading");
              if(!tmp265File.isEmpty()) {
                  QFile::remove(tmp265File);
              }
              return;
          }

      }

      process(input_fileName);
      QFileInfo info(fileName);
      settings.setValue("MainWindow/PrevDir", info.absoluteDir().absolutePath());

      setWindowTitle(info.fileName());
      if(!tmp265File.isEmpty()) {
         QFile::remove(tmp265File);
      }
    }
}

void MainWindow::slotShowWarningsViewer()
{
  m_pwarnViewer -> show();
  m_pwarnViewer -> raise();
}


void MainWindow::slotShowInfoViewer()
{
  m_pinfoViewer -> show();
  m_pinfoViewer -> raise();
}


void MainWindow::slotShowHDRInfoViewer()
{
  m_phdrInfoViewer -> show();
  m_phdrInfoViewer -> raise();
}


void MainWindow::closeEvent(QCloseEvent *pevent)
{
  saveCustomData();
  CentralWidget *pcntwgt = dynamic_cast<CentralWidget *>(centralWidget());
  pcntwgt -> m_pcomInfoViewer -> saveCustomData();

  m_pwarnViewer -> close();
  m_pinfoViewer -> close();
  m_phdrInfoViewer -> close();

  QWidget::closeEvent(pevent);
}


void MainWindow::saveCustomData()
{
  QSettings settings("HEVCESBrowser", "HEVCESBrowser");
  settings.setValue("MainWindow/geometry", saveGeometry());
}


void MainWindow::readCustomData()
{
  QSettings settings("HEVCESBrowser", "HEVCESBrowser");
  restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
}

void MainWindow::slotAbout()
{
  QString message;
  message = "<center><b>HEVCESBrowser</b></center>";
  message += "<center>virinext@gmail.com</center>";
  message += QString("<center>Version: ") + VERSION_STR + "</center>";
  message += "<center>GUI Based on Qt</center>";
  QMessageBox::about(this, "About", message);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    if(e->mimeData()->urls().size() > 0)
    {
        QSettings settings("HEVCESBrowser", "HEVCESBrowser");
        QUrl url = e->mimeData()->urls().first();
        openFile(url.toLocalFile());
    }
}
