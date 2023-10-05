/***********************************************************************************************************************
*  OpenStudio(R), Copyright (c) 2020-2023, OpenStudio Coalition and other contributors. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
*  following conditions are met:
*
*  (1) Redistributions of source code must retain the above copyright notice, this list of conditions and the following
*  disclaimer.
*
*  (2) Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
*  disclaimer in the documentation and/or other materials provided with the distribution.
*
*  (3) Neither the name of the copyright holder nor the names of any contributors may be used to endorse or promote products
*  derived from this software without specific prior written permission from the respective party.
*
*  (4) Other than as required in clauses (1) and (2), distributions in any form of modifications or other derivative works
*  may not use the "OpenStudio" trademark, "OS", "os", or any other confusingly similar designation without specific prior
*  written permission from Alliance for Sustainable Energy, LLC.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) AND ANY CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
*  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER(S), ANY CONTRIBUTORS, THE UNITED STATES GOVERNMENT, OR THE UNITED
*  STATES DEPARTMENT OF ENERGY, NOR ANY OF THEIR EMPLOYEES, BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
*  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
*  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
*  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************************************************************/

#include "ModelDesignWizardDialog.hpp"
#include "Buttons.hpp"
#include "OSQuantityEdit.hpp"
#include "Assert.hpp"

#include <QApplication>

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QCloseEvent>
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSharedPointer>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTimer>
#include <QStandardPaths>
#include <QDialog>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QLocale>

#include <array>
#include <fstream>
#include <vector>
#include <string_view>

#define FAILED_ARG_TEXT "<FONT COLOR = RED>Failed to Show Arguments<FONT COLOR = BLACK> <br> <br>Reason(s): <br> <br>"

#define ACCEPT_CHANGES openstudio::ModelDesignWizardDialog::tr("Accept Changes")
#define GENERATE_MODEL openstudio::ModelDesignWizardDialog::tr("Generate Model")
#define NEXT_PAGE openstudio::ModelDesignWizardDialog::tr("Next Page")

namespace openstudio {

enum LogLevel
{
  Trace = -3,
  Debug = -2,
  Info = -1,
  Warn = 0,
  Error = 1,
  Fatal = 2
};

QtMsgType convertOpenStudioLogLevelToQtMsgType(LogLevel level) {
  if (level == LogLevel::Trace || level == LogLevel::Debug) {
    return QtMsgType::QtDebugMsg;
  }
  if (level == LogLevel::Info) {
    return QtMsgType::QtInfoMsg;
  }
  if (level == LogLevel::Warn) {
    return QtMsgType::QtWarningMsg;
  }
  if (level == LogLevel::Error) {
    return QtMsgType::QtCriticalMsg;
  }
  if (level == LogLevel::Fatal) {
    return QtMsgType::QtFatalMsg;
  }

  return QtMsgType::QtDebugMsg;
}

void LOG(LogLevel level, const std::string& message) {
  const QString msg = QString::fromStdString(message);

  if (level == LogLevel::Trace || level == LogLevel::Debug) {
    qDebug() << msg;
  }
  if (level == LogLevel::Info) {
    qInfo() << msg;
  }
  if (level == LogLevel::Warn) {
    qWarning() << msg;
  }
  if (level == LogLevel::Error) {
    qCritical() << msg;
  }
  if (level == LogLevel::Fatal) {
    qFatal("%s", message.c_str());
  }
}

ModelDesignWizardDialog::ModelDesignWizardDialog(QWidget* parent)
  : OSDialog(false, parent),
    m_mainPaneStackedWidget(nullptr),
    m_rightPaneStackedWidget(nullptr),
    m_argumentsFailedTextEdit(nullptr),
    m_timer(nullptr),
    m_showAdvancedOutput(nullptr),
    m_advancedOutputDialog(nullptr) {
  setWindowTitle("Apply Measure Now");
  setWindowModality(Qt::ApplicationModal);
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  setSizeGripEnabled(true);

  // Load support JSON (has to be before createWidgets
  QFile file(":/library/ModelDesignWizard.json");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    auto supportJson = QJsonDocument::fromJson(QString(file.readAll()).toUtf8());
    m_supportJsonObject = supportJson.object();
    file.close();
  } else {
    LOG(LogLevel::Error, "Failed to open embedded ModelDesignWizard.json");
  }

  // Set the Locale to C, so that "1234.56" is accepted, but not "1234,56", no matter the user's system locale
  const QLocale lo(QLocale::C);

  m_ratioValidator = new QDoubleValidator(0.0, 1.0, 4);
  m_ratioValidator->setLocale(lo);

  m_positiveDoubleValidator = new QDoubleValidator();
  m_positiveDoubleValidator->setBottom(0.0);
  m_positiveDoubleValidator->setLocale(lo);

  createWidgets();
}

ModelDesignWizardDialog::~ModelDesignWizardDialog() = default;

QSize ModelDesignWizardDialog::sizeHint() const {
  return {770, 560};
}

QWidget* ModelDesignWizardDialog::createTemplateSelectionPage() {
  auto* widget = new QWidget();
  auto* mainGridLayout = new QGridLayout();
  mainGridLayout->setContentsMargins(7, 7, 7, 7);
  mainGridLayout->setSpacing(14);
  widget->setLayout(mainGridLayout);

  int row = mainGridLayout->rowCount();
  {
    int col = 0;

    {
      auto* standardTypeLabel = new QLabel("Standard Template Type:");
      standardTypeLabel->setObjectName("H2");
      mainGridLayout->addWidget(standardTypeLabel, row, col++, 1, 1);
    }
    {
      auto* targetStandardLabel = new QLabel("Target Standard:");
      targetStandardLabel->setObjectName("H2");
      mainGridLayout->addWidget(targetStandardLabel, row, col++, 1, 1);
    }
    {
      auto* primaryBuildingTypeLabel = new QLabel("Primary Building Template Type:");
      primaryBuildingTypeLabel->setObjectName("H2");
      mainGridLayout->addWidget(primaryBuildingTypeLabel, row, col++, 1, 1);
    }
  }

  ++row;
  {
    int col = 0;
    {
      m_standardTypeComboBox = new QComboBox();
      qDebug() << "m_supportJsonObject.keys()=" << m_supportJsonObject.keys();
      for (const QString& standardType : m_supportJsonObject.keys()) {
        qDebug() << "Adding standardType=" << standardType;

        m_standardTypeComboBox->addItem(standardType);
      }
      m_standardTypeComboBox->setCurrentIndex(0);
      mainGridLayout->addWidget(m_standardTypeComboBox, row, col++, 1, 1);
      const bool isConnected = connect(m_standardTypeComboBox, &QComboBox::currentTextChanged, this, &ModelDesignWizardDialog::onStandardTypeChanged);
    }

    {
      m_targetStandardComboBox = new QComboBox();
      mainGridLayout->addWidget(m_targetStandardComboBox, row, col++, 1, 1);
      const bool isConnected =
        connect(m_targetStandardComboBox, &QComboBox::currentTextChanged, this, &ModelDesignWizardDialog::onTargetStandardChanged);
    }

    {
      m_primaryBuildingTypeComboBox = new QComboBox();
      mainGridLayout->addWidget(m_primaryBuildingTypeComboBox, row, col++, 1, 1);
      const bool isConnected =
        connect(m_primaryBuildingTypeComboBox, &QComboBox::currentTextChanged, this, &ModelDesignWizardDialog::onPrimaryBuildingTypeChanged);
    }
  }

  ++row;
  m_useIPCheckBox = new QCheckBox("Use IP Units");
  mainGridLayout->addWidget(m_useIPCheckBox, row, 0, 1, 1);
  m_useIPCheckBox->setChecked(m_isIP);
  connect(m_useIPCheckBox, &QCheckBox::stateChanged, this, [this](int state) { m_isIP = state == Qt::Checked; });

  m_standardTypeComboBox->setCurrentText("DOE");
  mainGridLayout->setRowStretch(mainGridLayout->rowCount(), 100);

  return widget;
}

void ModelDesignWizardDialog::onStandardTypeChanged(const QString& /*text*/) {
  populateTargetStandards();
  populatePrimaryBuildingTypes();
}

void ModelDesignWizardDialog::populateStandardTypes() {}

void ModelDesignWizardDialog::onTargetStandardChanged(const QString& /*text*/) {
  disableOkButton(m_targetStandardComboBox->currentText().isEmpty() || m_primaryBuildingTypeComboBox->currentText().isEmpty());
}

void ModelDesignWizardDialog::populateTargetStandards() {
  m_targetStandardComboBox->blockSignals(true);

  m_targetStandardComboBox->clear();

  m_targetStandardComboBox->addItem("");

  const QString selectedStandardType = m_standardTypeComboBox->currentText();

  for (const auto& temp : m_supportJsonObject[selectedStandardType].toObject()["templates"].toArray()) {
    m_targetStandardComboBox->addItem(temp.toString());
  }

  m_targetStandardComboBox->setCurrentIndex(0);

  m_targetStandardComboBox->blockSignals(false);
}

void ModelDesignWizardDialog::onPrimaryBuildingTypeChanged(const QString& /*text*/) {
  const bool disabled = m_targetStandardComboBox->currentText().isEmpty() || m_primaryBuildingTypeComboBox->currentText().isEmpty();
  disableOkButton(disabled);
  if (!disabled) {
    qDebug() << "Populate Space Type Ratios";
    populateSpaceTypeRatiosPage();
  }
}

void ModelDesignWizardDialog::populateBuildingTypeComboBox(QComboBox* comboBox) {
  comboBox->blockSignals(true);

  comboBox->clear();

  comboBox->addItem("");

  const QString selectedStandardType = m_standardTypeComboBox->currentText();

  for (const auto& temp : m_supportJsonObject[selectedStandardType].toObject()["building_types"].toArray()) {
    comboBox->addItem(temp.toString());
  }

  comboBox->setCurrentIndex(0);

  comboBox->blockSignals(false);
}

void ModelDesignWizardDialog::populatePrimaryBuildingTypes() {
  populateBuildingTypeComboBox(m_primaryBuildingTypeComboBox);
}

void ModelDesignWizardDialog::populateSpaceTypeComboBox(QComboBox* comboBox, QString buildingType) {

  comboBox->blockSignals(true);

  comboBox->clear();

  comboBox->addItem("");

  if (buildingType.isEmpty()) {
    buildingType = m_primaryBuildingTypeComboBox->currentText();
  }
  if (!buildingType.isEmpty()) {
    const QString selectedStandardType = m_standardTypeComboBox->currentText();
    const QString selectedStandard = m_targetStandardComboBox->currentText();

    for (const QString& temp : m_supportJsonObject[selectedStandardType]
                                 .toObject()["space_types"]
                                 .toObject()[selectedStandard]
                                 .toObject()[buildingType]
                                 .toObject()
                                 .keys()) {
      comboBox->addItem(temp);
    }
  }

  comboBox->setCurrentIndex(0);

  comboBox->blockSignals(false);
}

QGridLayout* ModelDesignWizardDialog::spaceTypeRatiosMainLayout() const {
  return m_spaceTypeRatiosMainLayout;
}

QString ModelDesignWizardDialog::selectedPrimaryBuildingType() const {
  return m_primaryBuildingTypeComboBox->currentText();
}

bool ModelDesignWizardDialog::isIP() const {
  return m_isIP;
}

void ModelDesignWizardDialog::addSpaceTypeRatioRow(const QString& buildingType, const QString& spaceType, double ratio) {

  qDebug() << "inside: " << m_spaceTypeRatiosMainLayout;
  qDebug() << "inside: " << m_spaceTypeRatiosMainLayout->rowCount();

  auto* spaceTypeRow = new SpaceTypeRatioRow(this, buildingType, spaceType, ratio);
  m_spaceTypeRatioRows.push_back(spaceTypeRow);
  spaceTypeRow->vectorPos = m_spaceTypeRatioRows.size() - 1;

  // populateBuildingTypeComboBox(spaceTypeRow->buildingTypeComboBox);
  // populateSpaceTypeComboBox(spaceTypeRow->spaceTypeComboBox, buildingType);
  connect(m_useIPCheckBox, &QCheckBox::stateChanged,
          [this, &spaceTypeRow](int state) { spaceTypeRow->onUnitSystemChange(static_cast<bool>(state)); });

  connect(m_showAdvancedOutput, &QPushButton::clicked, this, &ModelDesignWizardDialog::showAdvancedOutput);

  connect(spaceTypeRow->deleteRowButton, &QPushButton::clicked, [this, spaceTypeRow]() { removeSpaceTypeRatioRow(spaceTypeRow); });
}

void ModelDesignWizardDialog::removeSpaceTypeRatioRow(SpaceTypeRatioRow* row) {

  qDebug() << "removeSpaceTypeRatioRow, Original rowCount: " << m_spaceTypeRatiosMainLayout->rowCount();
  qDebug() << "Removing row at gridLayoutRowIndex=" << row->gridLayoutRowIndex << " and vectorPos=" << row->vectorPos;
  for (int i = 0; auto* spaceTypeRatioRow : m_spaceTypeRatioRows) {
    qDebug() << "* " << i++ << "gridLayoutRowIndex=" << spaceTypeRatioRow->gridLayoutRowIndex << " and vectorPos=" << spaceTypeRatioRow->vectorPos;
  }
  auto it = std::next(m_spaceTypeRatioRows.begin(), row->vectorPos);

#if 1

  //  m_spaceTypeRatiosMainLayout->removeWidget(row->buildingTypeComboBox);
  m_spaceTypeRatiosMainLayout->removeWidget(row->buildingTypeComboBox);
  delete row->buildingTypeComboBox;
  m_spaceTypeRatiosMainLayout->removeWidget(row->spaceTypeComboBox);
  delete row->spaceTypeComboBox;
  m_spaceTypeRatiosMainLayout->removeWidget(row->spaceTypeRatioEdit);
  delete row->spaceTypeRatioEdit;
  m_spaceTypeRatiosMainLayout->removeWidget(row->spaceTypeFloorAreaEdit);
  delete row->spaceTypeFloorAreaEdit;
  m_spaceTypeRatiosMainLayout->removeWidget(row->deleteRowButton);
  delete row->deleteRowButton;

  m_spaceTypeRatiosMainLayout->setRowMinimumHeight(row->gridLayoutRowIndex, 0);
  m_spaceTypeRatiosMainLayout->setRowStretch(row->gridLayoutRowIndex, 0);

#elif 0
  QLayoutItem* child = nullptr;

  while ((child = m_spaceTypeRatiosMainLayout->takeAt(index)) != nullptr) {
    QWidget* widget = child->widget();

    OS_ASSERT(widget);

    delete widget;
    // Using deleteLater is actually slower than calling delete directly on the widget
    // deleteLater also introduces a strange redraw issue where the select all check box
    // is not redrawn, after being checked.
    //widget->deleteLater();

    delete child;
  }
#endif
  m_spaceTypeRatioRows.erase(it);
  qDebug() << "removeSpaceTypeRatioRow, Final rowCount: " << m_spaceTypeRatiosMainLayout->rowCount();
  for (int i = 0; auto* spaceTypeRatioRow : m_spaceTypeRatioRows) {
    spaceTypeRatioRow->vectorPos = i++;
  }
  recalculateTotalBuildingRatio(true);
}

SpaceTypeRatioRow::SpaceTypeRatioRow(ModelDesignWizardDialog* parent, const QString& buildingType, const QString& spaceType, double ratio)
  : buildingTypeComboBox(new QComboBox()),
    spaceTypeComboBox(new QComboBox()),
    spaceTypeRatioEdit(new openstudio::OSNonModelObjectQuantityEdit("", "", "", false)),
    spaceTypeFloorAreaEdit(new openstudio::OSNonModelObjectQuantityEdit("ft^2", "m^2", "ft^2", parent->isIP())),
    deleteRowButton(new openstudio::RemoveButton()),
    gridLayoutRowIndex(parent->spaceTypeRatiosMainLayout()->rowCount()) {

  int col = 0;

  parent->spaceTypeRatiosMainLayout()->addWidget(buildingTypeComboBox, gridLayoutRowIndex, col++, 1, 1);
  parent->populateBuildingTypeComboBox(buildingTypeComboBox);
  buildingTypeComboBox->setCurrentText(buildingType);

  parent->spaceTypeRatiosMainLayout()->addWidget(spaceTypeComboBox, gridLayoutRowIndex, col++, 1, 1);
  parent->populateSpaceTypeComboBox(spaceTypeComboBox, buildingType);
  spaceTypeComboBox->setCurrentText(spaceType);

  spaceTypeRatioEdit->setMinimumValue(0.0);
  spaceTypeRatioEdit->setMaximumValue(1.0);
  spaceTypeRatioEdit->enableClickFocus();
  // connect(this, &SpaceTypeRatioRow::onUnitSystemChange, spaceTypeRatioEdit, &OSNonModelObjectQuantityEdit::onUnitSystemChange);
  // ModelDesignWizardDialog::connect(parent, &ModelDesignWizardDialog::onUnitSystemChange, spaceTypeRatioEdit,
  //                                 &OSNonModelObjectQuantityEdit::onUnitSystemChange);

  parent->spaceTypeRatiosMainLayout()->addWidget(spaceTypeRatioEdit, gridLayoutRowIndex, col++, 1, 1);

  spaceTypeFloorAreaEdit->setMinimumValue(0.0);
  // spaceTypeFloorAreaEdit->enableClickFocus();
  spaceTypeFloorAreaEdit->setLocked(true);

  parent->spaceTypeRatiosMainLayout()->addWidget(spaceTypeFloorAreaEdit, gridLayoutRowIndex, col++, 1, 1);
  // connect(this, &SpaceTypeRatioRow::onUnitSystemChange, spaceTypeFloorAreaEdit, &OSNonModelObjectQuantityEdit::onUnitSystemChange);

  OSNonModelObjectQuantityEdit::connect(spaceTypeRatioEdit, &OSNonModelObjectQuantityEdit::valueChanged, [this, parent](double newValue) {
    recalculateFloorArea(parent->totalBuildingFloorArea());
    parent->recalculateTotalBuildingRatio(false);
  });
  parent->spaceTypeRatiosMainLayout()->addWidget(deleteRowButton, gridLayoutRowIndex, col++, 1, 1);

  spaceTypeRatioEdit->setDefault(ratio);

  // const bool isConnected = QObject::connect(buildingTypeComboBox, &QComboBox::currentTextChanged, [this, &spaceTypeComboBox](const QString& text) {
  //   parent->populateSpaceTypeComboBox(spaceTypeComboBox, text);
  // });
}

void ModelDesignWizardDialog::recalculateTotalBuildingRatio(bool forceToOne) {
  double totalRatio = 0;
  for (auto* spaceTypeRatioRow : m_spaceTypeRatioRows) {
    totalRatio += spaceTypeRatioRow->spaceTypeRatioEdit->currentValue();
  }
  if (forceToOne) {
    for (auto* spaceTypeRatioRow : m_spaceTypeRatioRows) {
      spaceTypeRatioRow->spaceTypeRatioEdit->blockSignals(true);
      spaceTypeRatioRow->spaceTypeRatioEdit->setCurrentValue(spaceTypeRatioRow->spaceTypeRatioEdit->currentValue() / totalRatio);
      spaceTypeRatioRow->spaceTypeRatioEdit->blockSignals(false);
    }
    totalRatio = 1.0;
  }

  m_totalBuildingRatioEdit->setText(QString::number(totalRatio));
}

double ModelDesignWizardDialog::totalBuildingFloorArea() const {
  return m_totalBuildingFloorAreaEdit->currentValue();
}

void SpaceTypeRatioRow::onUnitSystemChange(bool isIP) {
  // spaceTypeRatioEdit->onUnitSystemChange(isIP);
  spaceTypeFloorAreaEdit->onUnitSystemChange(isIP);
}

void SpaceTypeRatioRow::recalculateFloorArea(double totalBuildingFloorArea) {
  const double floorArea = spaceTypeRatioEdit->currentValue() * totalBuildingFloorArea;
  spaceTypeFloorAreaEdit->setCurrentValue(floorArea);
}

void ModelDesignWizardDialog::populateSpaceTypeRatiosPage() {

  if (auto* existingLayout = m_spaceTypeRatiosPageWidget->layout()) {
    // Reparent the layout to a temporary widget, so we can install the new one, and this one will get deleted because we don't have a reference to it anymore
    QWidget().setLayout(existingLayout);
  }

  m_spaceTypeRatiosMainLayout = new QGridLayout();
  m_spaceTypeRatiosMainLayout->setContentsMargins(7, 7, 7, 7);
  m_spaceTypeRatiosMainLayout->setSpacing(14);
  m_spaceTypeRatiosPageWidget->setLayout(m_spaceTypeRatiosMainLayout);

  int row = m_spaceTypeRatiosMainLayout->rowCount();
  {
    int col = 0;

    {
      auto* totalBuildingFloorAreaLabel = new QLabel("Total Building Floor Area:");
      totalBuildingFloorAreaLabel->setObjectName("H2");
      m_spaceTypeRatiosMainLayout->addWidget(totalBuildingFloorAreaLabel, row, col++, 1, 1);
    }
    {
      m_totalBuildingFloorAreaEdit = new openstudio::OSNonModelObjectQuantityEdit("ft^2", "m^2", "ft^2", m_isIP);
      m_totalBuildingFloorAreaEdit->setMinimumValue(0.0);
      m_totalBuildingFloorAreaEdit->enableClickFocus();
      m_spaceTypeRatiosMainLayout->addWidget(m_totalBuildingFloorAreaEdit, row, col++, 1, 1);
      connect(m_useIPCheckBox, &QCheckBox::stateChanged, m_totalBuildingFloorAreaEdit, &OSNonModelObjectQuantityEdit::onUnitSystemChange);
      m_totalBuildingFloorAreaEdit->setDefault(10000.0);
    }
    {
      auto* totalBuildingRatioLabel = new QLabel("Total Ratio:");
      totalBuildingRatioLabel->setObjectName("H2");
      m_spaceTypeRatiosMainLayout->addWidget(totalBuildingRatioLabel, row, col++, 1, 1);
    }
    {
      m_totalBuildingRatioEdit = new QLineEdit();
      m_totalBuildingRatioEdit->setEnabled(false);
      m_spaceTypeRatiosMainLayout->addWidget(m_totalBuildingRatioEdit, row, col++, 1, 1);
    }
  }

  ++row;

  auto* addRowButton = new openstudio::AddButton();
  m_spaceTypeRatiosMainLayout->addWidget(addRowButton, row, 0, 1, 1);
  connect(addRowButton, &QPushButton::clicked, [this]() { addSpaceTypeRatioRow(); });

  // TODO: add a way to add / delete rows, so one could pick from another building type for eg

  ++row;
  {
    int col = 0;
    {
      auto* buildingTypelabel = new QLabel("Building Type:");
      buildingTypelabel->setObjectName("H2");
      m_spaceTypeRatiosMainLayout->addWidget(buildingTypelabel, row, col++, 1, 1);
    }
    {
      auto* spaceTypelabel = new QLabel("Space Type:");
      spaceTypelabel->setObjectName("H2");
      m_spaceTypeRatiosMainLayout->addWidget(spaceTypelabel, row, col++, 1, 1);
    }
    {
      auto* ratioLabel = new QLabel("Ratio:");
      ratioLabel->setObjectName("H2");
      m_spaceTypeRatiosMainLayout->addWidget(ratioLabel, row, col++, 1, 1);
    }
    {
      auto* floorAreaLabel = new QLabel("Area:");
      floorAreaLabel->setObjectName("H2");
      m_spaceTypeRatiosMainLayout->addWidget(floorAreaLabel, row, col++, 1, 1);
    }
  }

  const QString selectedStandardType = m_standardTypeComboBox->currentText();
  const QString selectedStandard = m_targetStandardComboBox->currentText();
  const QString selectedPrimaryBuildingType = m_primaryBuildingTypeComboBox->currentText();

  const QJsonObject defaultSpaceTypeRatios = m_supportJsonObject[selectedStandardType]
                                               .toObject()["space_types"]
                                               .toObject()[selectedStandard]
                                               .toObject()[selectedPrimaryBuildingType]
                                               .toObject();
  for (QJsonObject::const_iterator it = defaultSpaceTypeRatios.constBegin(); it != defaultSpaceTypeRatios.constEnd(); ++it) {
    ++row;
    {
#if 1
      const QString spaceType = it.key();
      const double ratio = it.value().toObject()["ratio"].toDouble();
      qDebug() << "before: " << m_spaceTypeRatiosMainLayout;
      qDebug() << "before: " << m_spaceTypeRatiosMainLayout->rowCount();
      addSpaceTypeRatioRow(selectedPrimaryBuildingType, spaceType, ratio);

#elif 0
      auto* buildingTypeComboBox = new QComboBox();
      m_spaceTypeRatiosMainLayout->addWidget(buildingTypeComboBox, row, col++, 1, 1);
      populateBuildingTypeComboBox(buildingTypeComboBox);
      buildingTypeComboBox->setCurrentText(selectedPrimaryBuildingType);

      auto* spaceTypeComboBox = new QComboBox();
      m_spaceTypeRatiosMainLayout->addWidget(spaceTypeComboBox, row, col++, 1, 1);
      populateSpaceTypeComboBox(spaceTypeComboBox, selectedPrimaryBuildingType);
      spaceTypeComboBox->setCurrentText(it.key());

      auto* spaceTypeRatioEdit = new openstudio::OSNonModelObjectQuantityEdit("", "", "", false);
      spaceTypeRatioEdit->setMinimumValue(0.0);
      spaceTypeRatioEdit->setMaximumValue(1.0);
      spaceTypeRatioEdit->enableClickFocus();
      connect(m_useIPCheckBox, &QCheckBox::stateChanged, spaceTypeRatioEdit, &OSNonModelObjectQuantityEdit::onUnitSystemChange);

      spaceTypeRatioEdit->setDefault(it.value().toObject()["ratio"].toDouble());
      m_spaceTypeRatiosMainLayout->addWidget(spaceTypeRatioEdit, row, col++, 1, 1);

      auto* spaceTypeFloorAreaEdit = new openstudio::OSNonModelObjectQuantityEdit("ft^2", "m^2", "ft^2", m_isIP);
      spaceTypeFloorAreaEdit->setMinimumValue(0.0);
      spaceTypeFloorAreaEdit->enableClickFocus();
      m_spaceTypeRatiosMainLayout->addWidget(spaceTypeFloorAreaEdit, row, col++, 1, 1);
      connect(m_useIPCheckBox, &QCheckBox::stateChanged, spaceTypeFloorAreaEdit, &OSNonModelObjectQuantityEdit::onUnitSystemChange);

      auto* deleteRowButton = new openstudio::RemoveButton();
      m_spaceTypeRatiosMainLayout->addWidget(deleteRowButton, row, col++, 1, 1);

      const bool isConnected = connect(buildingTypeComboBox, &QComboBox::currentTextChanged,
                                       [this, &spaceTypeComboBox](const QString& text) { populateSpaceTypeComboBox(spaceTypeComboBox, text); });
#else
      // Put it in a widget so we can delete and hide
      auto* rowWidget = new QWidget();
      auto* hBoxLayout = new QHBoxLayout(rowWidget);

      auto* buildingTypeComboBox = new QComboBox();
      hBoxLayout->addWidget(buildingTypeComboBox);
      populateBuildingTypeComboBox(buildingTypeComboBox);
      buildingTypeComboBox->setCurrentText(selectedPrimaryBuildingType);

      auto* spaceTypeComboBox = new QComboBox();
      hBoxLayout->addWidget(spaceTypeComboBox);
      populateSpaceTypeComboBox(spaceTypeComboBox, selectedPrimaryBuildingType);
      spaceTypeComboBox->setCurrentText(it.key());

      auto* spaceTypeRatioEdit = new QLineEdit();
      spaceTypeRatioEdit->setValidator(m_ratioValidator);
      hBoxLayout->addWidget(spaceTypeRatioEdit);

      auto* deleteRowButton = new openstudio::RemoveButton();
      hBoxLayout->addWidget(deleteRowButton);

      spaceTypeRatioEdit->setText(QString::number(it.value().toObject()["ratio"].toDouble()));

      // Spans 1 row and 4 columns
      m_spaceTypeRatiosMainLayout->addWidget(rowWidget, row, 0, 1, 4);

      const bool isConnected = connect(buildingTypeComboBox, &QComboBox::currentTextChanged,
                                       [this, &spaceTypeComboBox](const QString& text) { populateSpaceTypeComboBox(spaceTypeComboBox, text); });
#endif
    }
  }

  m_spaceTypeRatiosMainLayout->setRowStretch(m_spaceTypeRatiosMainLayout->rowCount(), 100);
}

QWidget* ModelDesignWizardDialog::createSpaceTypeRatiosPage() {
  m_spaceTypeRatiosPageWidget = new QWidget();

  return m_spaceTypeRatiosPageWidget;
}

QWidget* ModelDesignWizardDialog::createRunningPage() {
  auto* widget = new QWidget();

  auto* label = new QLabel("Running Measure");
  label->setObjectName("H2");

  auto* layout = new QVBoxLayout();
  layout->addStretch();
  layout->addWidget(label, 0, Qt::AlignCenter);
  layout->addStretch();

  widget->setLayout(layout);
  return widget;
}

QWidget* ModelDesignWizardDialog::createOutputPage() {
  auto* widget = new QWidget();

  auto* label = new QLabel("Measure Output");
  label->setObjectName("H1");

  m_jobPath = new QLabel();
  m_jobPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
#if !(_DEBUG || (__GNUC__ && !NDEBUG))
  m_jobPath->hide();
#endif

  auto* layout = new QVBoxLayout();
  layout->addWidget(label);
  layout->addWidget(m_jobPath);

  layout->addStretch();

  m_showAdvancedOutput = new QPushButton("Advanced Output");
  connect(m_showAdvancedOutput, &QPushButton::clicked, this, &ModelDesignWizardDialog::showAdvancedOutput);

  //layout->addStretch();

  auto* hLayout = new QHBoxLayout();
  hLayout->addWidget(m_showAdvancedOutput);
  hLayout->addStretch();
  layout->addLayout(hLayout);

  widget->setLayout(layout);

  auto* scrollArea = new QScrollArea();
  scrollArea->setWidgetResizable(true);
  scrollArea->setWidget(widget);

  return scrollArea;
}

void ModelDesignWizardDialog::createWidgets() {

  // PAGE STACKED WIDGET

  m_mainPaneStackedWidget = new QStackedWidget();
  upperLayout()->addWidget(m_mainPaneStackedWidget);

  // Selection of the template
  m_templateSelectionPageIdx = m_mainPaneStackedWidget->addWidget(createTemplateSelectionPage());

  m_spaceTypeRatiosPageIdx = m_mainPaneStackedWidget->addWidget(createSpaceTypeRatiosPage());

  // RUNNING
  m_runningPageIdx = m_mainPaneStackedWidget->addWidget(createRunningPage());

  // OUTPUT

  m_outputPageIdx = m_mainPaneStackedWidget->addWidget(createOutputPage());

  // SET CURRENT INDEXES
  m_mainPaneStackedWidget->setCurrentIndex(m_templateSelectionPageIdx);

  // BUTTONS

  this->okButton()->setText(NEXT_PAGE);
  this->okButton()->setEnabled(false);

  this->backButton()->show();
  this->backButton()->setEnabled(false);

  // OS SETTINGS

#ifdef Q_OS_DARWIN
  setWindowFlags(Qt::FramelessWindowHint);
#elif defined(Q_OS_WIN)
  setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
#endif

  // For quicker testing, TODO: REMOVE
  m_targetStandardComboBox->setCurrentText("90.1-2019");
  m_primaryBuildingTypeComboBox->setCurrentText("SecondarySchool");
  // END TODO: REMOVE
}

void ModelDesignWizardDialog::resizeEvent(QResizeEvent* event) {
  // Use the QDialog one so it can be resized (OSDialog prevents resizing)
  QDialog::resizeEvent(event);
}

void ModelDesignWizardDialog::runMeasure() {
  m_mainPaneStackedWidget->setCurrentIndex(m_runningPageIdx);
  m_timer->start(50);
  this->okButton()->hide();
  this->backButton()->hide();
}

void ModelDesignWizardDialog::displayResults() {
  QString qstdout;
  QString qstderr;

  m_mainPaneStackedWidget->setCurrentIndex(m_outputPageIdx);
  m_timer->stop();

  this->okButton()->setText(ACCEPT_CHANGES);
  this->okButton()->show();
  this->okButton()->setEnabled(false);
  this->backButton()->show();
  this->backButton()->setEnabled(true);
  this->cancelButton()->setEnabled(true);

  m_advancedOutput.clear();

  try {

    m_advancedOutput = "";

    m_advancedOutput += "<b>Standard Output:</b>\n";
    m_advancedOutput += qstdout;
    m_advancedOutput += QString("\n");

    m_advancedOutput += "<b>Standard Error:</b>\n";
    m_advancedOutput += qstderr;
    m_advancedOutput += QString("\n");

    m_advancedOutput += QString("\n");

    m_advancedOutput.replace("\n", "<br>");

  } catch (std::exception&) {
  }
}

/***** SLOTS *****/

void ModelDesignWizardDialog::on_cancelButton(bool checked) {
  qApp->exit();
  if (m_mainPaneStackedWidget->currentIndex() == m_templateSelectionPageIdx) {
    // Nothing specific here
  } else if (m_mainPaneStackedWidget->currentIndex() == m_spaceTypeRatiosPageIdx) {
    // Nothing specific here
  } else if (m_mainPaneStackedWidget->currentIndex() == m_runningPageIdx) {
    // TODO: fix
    //    if(m_job){
    //      m_job->requestStop();
    //      this->cancelButton()->setDisabled(true);
    //      this->okButton()->setDisabled(true);
    //      return;
    //    }
    m_mainPaneStackedWidget->setCurrentIndex(m_templateSelectionPageIdx);
    m_timer->stop();
    this->okButton()->show();
    this->backButton()->show();
    return;
  } else if (m_mainPaneStackedWidget->currentIndex() == m_outputPageIdx) {
    m_mainPaneStackedWidget->setCurrentIndex(m_templateSelectionPageIdx);
  }

  // DLM: m_job->requestStop() might still be working, don't try to delete this here
  //removeWorkingDir();
}

void ModelDesignWizardDialog::on_backButton(bool checked) {
  if (m_mainPaneStackedWidget->currentIndex() == m_templateSelectionPageIdx) {
    // Nothing specific here
  } else if (m_mainPaneStackedWidget->currentIndex() == m_spaceTypeRatiosPageIdx) {
    this->backButton()->setEnabled(false);
    this->okButton()->setText(NEXT_PAGE);
    m_mainPaneStackedWidget->setCurrentIndex(m_templateSelectionPageIdx);
  } else if (m_mainPaneStackedWidget->currentIndex() == m_runningPageIdx) {
    // Nothing specific here
  } else if (m_mainPaneStackedWidget->currentIndex() == m_outputPageIdx) {
    this->okButton()->setEnabled(true);
    this->okButton()->setText(ACCEPT_CHANGES);
    this->backButton()->setEnabled(false);
    m_mainPaneStackedWidget->setCurrentIndex(m_templateSelectionPageIdx);
  }
}

void ModelDesignWizardDialog::on_okButton(bool checked) {
  if (m_mainPaneStackedWidget->currentIndex() == m_templateSelectionPageIdx) {
    m_mainPaneStackedWidget->setCurrentIndex(m_spaceTypeRatiosPageIdx);
    this->backButton()->setEnabled(true);
    this->okButton()->setText(GENERATE_MODEL);
  } else if (m_mainPaneStackedWidget->currentIndex() == m_spaceTypeRatiosPageIdx) {
    runMeasure();
  } else if (m_mainPaneStackedWidget->currentIndex() == m_runningPageIdx) {
    // N/A
    // OS_ASSERT(false);
  } else if (m_mainPaneStackedWidget->currentIndex() == m_outputPageIdx) {
    // reload the model
    requestReload();
  }
}

void ModelDesignWizardDialog::requestReload() {

  // close the dialog
  close();
}

void ModelDesignWizardDialog::closeEvent(QCloseEvent* event) {
  //DLM: don't do this here in case we are going to load the model
  //removeWorkingDir();

  // DLM: do not allow closing window while running
  if (m_mainPaneStackedWidget->currentIndex() == m_runningPageIdx) {
    event->ignore();
    return;
  }

  event->accept();
}

void ModelDesignWizardDialog::disableOkButton(bool disable) {
  this->okButton()->setDisabled(disable);
}

void ModelDesignWizardDialog::showAdvancedOutput() {
  if (m_advancedOutput.isEmpty()) {
    QMessageBox::information(this, QString("Advanced Output"), QString("No advanced output."));
  }
}

}  // namespace openstudio
