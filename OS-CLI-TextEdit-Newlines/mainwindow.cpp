#include "mainwindow.hpp"

#include <QGridLayout>
#include <QToolButton>
#include <QTextEdit>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto * centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto * mainLayout = new QGridLayout();
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(5);
    centralWidget->setLayout(mainLayout);

    m_playButton = new QToolButton();
    m_playButton->setText("Run");
    m_playButton->setCheckable(true);
    m_playButton->setChecked(false);
    m_playButton->setStyleSheet("QToolButton { background:blue; font: bold; }");
    m_playButton->setIconSize(QSize(35, 35));
    m_playButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_playButton->setLayoutDirection(Qt::RightToLeft);
    m_playButton->setStyleSheet("QAbstractButton:!hover { border: none; }");

    mainLayout->addWidget(m_playButton, 0, 0);
    connect(m_playButton, &QToolButton::clicked, this, &MainWindow::playButtonClicked);

    m_textInfo = new QTextEdit();
    m_textInfo->setReadOnly(true);
    mainLayout->addWidget(m_textInfo, 1, 0, 1, 4);

    m_runProcess = new QProcess(this);
    connect(m_runProcess, &QProcess::finished, this, &MainWindow::onRunProcessFinished);
    connect(m_runProcess, &QProcess::errorOccurred, this, &MainWindow::onRunProcessErrored);
    connect(m_runProcess, &QProcess::readyReadStandardError, this, &MainWindow::readyReadStandardError);
    connect(m_runProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::readyReadStandardOutput);

    m_runTcpServer = new QTcpServer();
    m_runTcpServer->listen();
    connect(m_runTcpServer, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
}



MainWindow::~MainWindow()
{
}

void MainWindow::onNewConnection() {
  m_runSocket = m_runTcpServer->nextPendingConnection();
  connect(m_runSocket, &QTcpSocket::readyRead, this, &MainWindow::onRunDataReady);
}

void MainWindow::playButtonClicked(bool t_checked) {

  if (t_checked) {
    // run

    unsigned port = m_runTcpServer->serverPort();
    m_hasSocketConnexion = (port != 0);
    // NOTE: temp test, uncomment to see fallback to stdout
    // m_hasSocketConnexion = false;

    QStringList arguments;
    arguments << "--verbose";

    QString workflowJSONPath("/Users/julien/Software/QtTestBed/OS-CLI-TextEdit-Newlines/test/compact.osw");

    if (m_hasSocketConnexion) {
      arguments << "run"
                << "-s" << QString::number(port) << "-w" << workflowJSONPath;
    } else {
      arguments << "run"
                << "--show-stdout"
                // << "--style-stdout"
                // << "--add-timings"
                << "-w" << workflowJSONPath;
    }
    qDebug() << "run arguments = " << arguments.join(";");

    m_textInfo->clear();

    if (!m_hasSocketConnexion) {
      m_textInfo->setTextColor(Qt::red);
      m_textInfo->setFontPointSize(15);
      m_textInfo->append("Could not open socket connection to OpenStudio CLI.");
      m_textInfo->setFontPointSize(12);
      m_textInfo->append("Falling back to stdout/stderr parsing, live updates might be slower.");
    }

    m_runProcess->start("/Applications/OpenStudio-3.4.0/bin/openstudio", arguments);
  } else {
    // stop running
    qDebug() << "Kill Simulation";
    m_textInfo->setTextColor(Qt::red);
    m_textInfo->setFontPointSize(18);
    m_textInfo->append("Aborted");
    m_runProcess->blockSignals(true);
    m_runProcess->kill();
    m_runProcess->blockSignals(false);
  }
}

void MainWindow::readyReadStandardOutput() {

  auto appendErrorText = [&](const QString& text) {
    m_textInfo->setTextColor(Qt::red);
    m_textInfo->setFontPointSize(18);
    m_textInfo->append(text);
  };

  auto appendNormalText = [&](const QString& text) {
    m_textInfo->setTextColor(Qt::black);
    m_textInfo->setFontPointSize(12);
    m_textInfo->append(text);
  };

  auto appendH1Text = [&](const QString& text) {
    m_textInfo->setTextColor(Qt::black);
    m_textInfo->setFontPointSize(18);
    m_textInfo->append(text);
  };

  auto appendH2Text = [&](const QString& text) {
    m_textInfo->setTextColor(Qt::black);
    m_textInfo->setFontPointSize(15);
    m_textInfo->append(text);
  };

  QString data = m_runProcess->readAllStandardOutput();
  QStringList lines = data.split("\n");


  for (const auto& line : lines) {
    //std::cout << data.toStdString() << std::endl;

    QString trimmedLine = line.trimmed();

    // DLM: coordinate with openstudio-workflow-gem\lib\openstudio\workflow\adapters\output\socket.rb
    if (trimmedLine.isEmpty()) {
      continue;
    } else if ((trimmedLine.contains("DEBUG")) || (trimmedLine.contains("] <-2>"))) {
      m_textInfo->setFontPointSize(10);
      m_textInfo->setTextColor(Qt::lightGray);
      m_textInfo->append(trimmedLine);
    } else if ((trimmedLine.contains("INFO")) || (trimmedLine.contains("] <-1>"))) {
      m_textInfo->setFontPointSize(10);
      m_textInfo->setTextColor(Qt::gray);
      m_textInfo->append(trimmedLine);
    } else if ((trimmedLine.contains("WARN")) || (trimmedLine.contains("] <0>"))) {
      m_textInfo->setFontPointSize(12);
      m_textInfo->setTextColor(Qt::darkYellow);
      m_textInfo->append(trimmedLine);
    } else if ((trimmedLine.contains("ERROR")) || (trimmedLine.contains("] <1>"))) {
      m_textInfo->setFontPointSize(12);
      m_textInfo->setTextColor(Qt::darkRed);
      m_textInfo->append(trimmedLine);
    } else if ((trimmedLine.contains("FATAL")) || (trimmedLine.contains("] <1>"))) {
      m_textInfo->setFontPointSize(14);
      m_textInfo->setTextColor(Qt::red);
      m_textInfo->append(trimmedLine);

    } else if (!m_hasSocketConnexion) {
      // For socket fall back. Avoid doing all these compare if we know we don't need to
      if (QString::compare(trimmedLine, "Starting state initialization", Qt::CaseInsensitive) == 0) {
        appendH1Text("Initializing workflow.");
      } else if (QString::compare(trimmedLine, "Started", Qt::CaseInsensitive) == 0) {
        // no-op
      } else if (QString::compare(trimmedLine, "Returned from state initialization", Qt::CaseInsensitive) == 0) {
        // no-op
      } else if (QString::compare(trimmedLine, "Starting state os_measures", Qt::CaseInsensitive) == 0) {
        appendH1Text("Processing OpenStudio Measures.");
      } else if (QString::compare(trimmedLine, "Returned from state os_measures", Qt::CaseInsensitive) == 0) {
        // no-op
      } else if (QString::compare(trimmedLine, "Starting state translator", Qt::CaseInsensitive) == 0) {
        appendH1Text("Translating the OpenStudio Model to EnergyPlus.");
      } else if (QString::compare(trimmedLine, "Returned from state translator", Qt::CaseInsensitive) == 0) {
        // no-op
      } else if (QString::compare(trimmedLine, "Starting state ep_measures", Qt::CaseInsensitive) == 0) {
        appendH1Text("Processing EnergyPlus Measures.");
      } else if (QString::compare(trimmedLine, "Returned from state ep_measures", Qt::CaseInsensitive) == 0) {
        // no-op
      } else if (QString::compare(trimmedLine, "Starting state preprocess", Qt::CaseInsensitive) == 0) {
        // ignore this state
      } else if (QString::compare(trimmedLine, "Returned from state preprocess", Qt::CaseInsensitive) == 0) {
        // ignore this state
      } else if (QString::compare(trimmedLine, "Starting state simulation", Qt::CaseInsensitive) == 0) {
        appendH1Text("Starting Simulation.");
      } else if (QString::compare(trimmedLine, "Returned from state simulation", Qt::CaseInsensitive) == 0) {
        // no-op
      } else if (QString::compare(trimmedLine, "Starting state reporting_measures", Qt::CaseInsensitive) == 0) {
        appendH1Text("Processing Reporting Measures.");
      } else if (QString::compare(trimmedLine, "Returned from state reporting_measures", Qt::CaseInsensitive) == 0) {
        // no-op
      } else if (QString::compare(trimmedLine, "Starting state postprocess", Qt::CaseInsensitive) == 0) {
        appendH1Text("Gathering Reports.");
      } else if (QString::compare(trimmedLine, "Returned from state postprocess", Qt::CaseInsensitive) == 0) {
        // no-op
      } else if (QString::compare(trimmedLine, "Failure", Qt::CaseInsensitive) == 0) {
        appendErrorText("Failed.");
      } else if (QString::compare(trimmedLine, "Complete", Qt::CaseInsensitive) == 0) {
        appendH1Text("Completed.");
      } else if (trimmedLine.startsWith("Applying", Qt::CaseInsensitive)) {
        appendH2Text(line);
      } else if (trimmedLine.startsWith("Applied", Qt::CaseInsensitive)) {
        // no-op
      } else {
        appendNormalText(trimmedLine);
      }
    } else {  // m_hasSocketConnexion: we know it's stdout and not important socket info, so we put that in gray
      m_textInfo->setFontPointSize(10);
      m_textInfo->setTextColor(Qt::gray);
      m_textInfo->append(trimmedLine);
    }
  }
}

void MainWindow::readyReadStandardError() {
  auto appendErrorText = [&](const QString& text) {
    m_textInfo->setTextColor(Qt::darkRed);
    m_textInfo->setFontPointSize(18);
    m_textInfo->append(text);
  };

  QString data = m_runProcess->readAllStandardError();
  QStringList lines = data.split("\n");

  for (const auto& line : lines) {

    QString trimmedLine = line.trimmed();

    if (trimmedLine.isEmpty()) {
      continue;
    } else {
      appendErrorText("stderr: " + line);
    }
  }
}

void MainWindow::onRunProcessErrored(QProcess::ProcessError error) {
  m_textInfo->setTextColor(Qt::red);
  m_textInfo->setFontPointSize(18);
  QString text = tr("onRunProcessErrored: Simulation failed to run, QProcess::ProcessError: ") + QString::number(error);
  m_textInfo->append(text);
}

void MainWindow::onRunProcessFinished(int exitCode, QProcess::ExitStatus status) {
  if (status == QProcess::NormalExit) {
    qDebug() << "run finished, exit code = " << exitCode;
  }

  if (exitCode != 0 || status == QProcess::CrashExit) {
    m_textInfo->setTextColor(Qt::red);
    m_textInfo->setFontPointSize(18);
    m_textInfo->append(tr("Simulation failed to run, with exit code ") + QString::number(exitCode));
  }

  m_playButton->setChecked(false);

  if (m_runSocket) {
    delete m_runSocket;
  }
  m_runSocket = nullptr;
}

void MainWindow::onRunDataReady() {
}
