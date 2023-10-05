#include "OSQuantityEdit.hpp"
#include "Assert.hpp"

#include <QDebug>

#include <QFocusEvent>
#include <QHBoxLayout>
#include <QStyle>

#include <bitset>
#include <iomanip>
#include <sstream>

namespace openstudio {

QuantityLineEdit::QuantityLineEdit(QWidget* parent) : QLineEdit(parent) {
  this->setStyleSheet("QLineEdit[style=\"0000\"] { color:black; background:white;   } "  // Locked=0, Focused=0, Auto=0, Defaulted=0
                      "QLineEdit[style=\"0001\"] { color:green; background:white;   } "  // Locked=0, Focused=0, Auto=0, Defaulted=1
                      "QLineEdit[style=\"0010\"] { color:grey;  background:white;   } "  // Locked=0, Focused=0, Auto=1, Defaulted=0
                      "QLineEdit[style=\"0011\"] { color:grey;  background:white;   } "  // Locked=0, Focused=0, Auto=1, Defaulted=1
                      "QLineEdit[style=\"0100\"] { color:black; background:#ffc627; } "  // Locked=0, Focused=1, Auto=0, Defaulted=0
                      "QLineEdit[style=\"0101\"] { color:green; background:#ffc627; } "  // Locked=0, Focused=1, Auto=0, Defaulted=1
                      "QLineEdit[style=\"0110\"] { color:grey;  background:#ffc627; } "  // Locked=0, Focused=1, Auto=1, Defaulted=0
                      "QLineEdit[style=\"0111\"] { color:grey;  background:#ffc627; } "  // Locked=0, Focused=1, Auto=1, Defaulted=1
                      "QLineEdit[style=\"1000\"] { color:black; background:#e6e6e6; } "  // Locked=1, Focused=0, Auto=0, Defaulted=0
                      "QLineEdit[style=\"1001\"] { color:green; background:#e6e6e6; } "  // Locked=1, Focused=0, Auto=0, Defaulted=1
                      "QLineEdit[style=\"1010\"] { color:grey;  background:#e6e6e6; } "  // Locked=1, Focused=0, Auto=1, Defaulted=0
                      "QLineEdit[style=\"1011\"] { color:grey;  background:#e6e6e6; } "  // Locked=1, Focused=0, Auto=1, Defaulted=1
                      "QLineEdit[style=\"1100\"] { color:black; background:#cc9a00; } "  // Locked=1, Focused=1, Auto=0, Defaulted=0
                      "QLineEdit[style=\"1101\"] { color:green; background:#cc9a00; } "  // Locked=1, Focused=1, Auto=0, Defaulted=1
                      "QLineEdit[style=\"1110\"] { color:grey;  background:#cc9a00; } "  // Locked=1, Focused=1, Auto=1, Defaulted=0
                      "QLineEdit[style=\"1111\"] { color:grey;  background:#cc9a00; } "  // Locked=1, Focused=1, Auto=1, Defaulted=1
  );
}

void QuantityLineEdit::enableClickFocus() {
  m_hasClickFocus = true;
}

void QuantityLineEdit::disableClickFocus() {
  m_hasClickFocus = false;
  m_focused = false;
  updateStyle();
  clearFocus();
  emit inFocus(false, false);
}

bool QuantityLineEdit::hasData() const {
  return !(this->text().isEmpty());
}

bool QuantityLineEdit::focused() const {
  return m_focused;
}

void QuantityLineEdit::setDefaultedAndAuto(bool defaulted, bool isAuto) {
  m_defaulted = defaulted;
  m_auto = isAuto;
  updateStyle();
}

bool QuantityLineEdit::locked() const {
  return m_locked;
}

void QuantityLineEdit::setLocked(bool locked) {
  if (m_locked != locked) {
    m_locked = locked;
    setReadOnly(locked);
    updateStyle();
  }
}

void QuantityLineEdit::focusInEvent(QFocusEvent* e) {
  if (e->reason() == Qt::MouseFocusReason && m_hasClickFocus) {
    m_focused = true;
    updateStyle();
    emit inFocus(m_focused, hasData());
  }

  QLineEdit::focusInEvent(e);
}

void QuantityLineEdit::focusOutEvent(QFocusEvent* e) {
  if (e->reason() == Qt::MouseFocusReason && m_hasClickFocus) {
    m_focused = false;
    updateStyle();
    emit inFocus(m_focused, false);
  }

  QLineEdit::focusOutEvent(e);
}

void QuantityLineEdit::updateStyle() {
  // Locked, Focused, Auto, Defaulted
  std::bitset<4> style;
  style[0] = m_defaulted;
  style[1] = m_auto;
  style[2] = m_focused;
  style[3] = m_locked;
  const QString thisStyle = QString::fromStdString(style.to_string());

  const QVariant currentStyle = this->property("style");
  if (currentStyle.isNull() || currentStyle.toString() != thisStyle) {
    this->setProperty("style", thisStyle);
    this->style()->unpolish(this);
    this->style()->polish(this);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: WORKAROUND
boost::optional<double> convert(double value, const std::string& originalUnits, const std::string& finalUnits) {
  if ((originalUnits == "m^2") && (finalUnits == "ft^2")) {
    return value * 10.763910416709722;
  } else if ((originalUnits == "ft^2") && (finalUnits == "m^2")) {
    return value / 10.763910416709722;
  }
  return value;
}

OSNonModelObjectQuantityEdit::OSNonModelObjectQuantityEdit(const std::string& modelUnits, const std::string& siUnits, const std::string& ipUnits,
                                                           bool isIP, QWidget* parent)
  : QWidget(parent),
    m_lineEdit(new QuantityLineEdit()),
    m_units(new QLabel()),
    m_isIP(isIP),
    m_modelUnits(modelUnits),
    m_siUnits(siUnits),
    m_ipUnits(ipUnits),
    m_isScientific(false) {
  connect(m_lineEdit, &QuantityLineEdit::inFocus, this, &OSNonModelObjectQuantityEdit::inFocus);

  connect(m_lineEdit, &QLineEdit::editingFinished, this,
          &OSNonModelObjectQuantityEdit::onEditingFinished);  // Evan note: would behaviors improve with "textChanged"?

  // do a test conversion to make sure units are ok
  boost::optional<double> test = convert(1.0, modelUnits, ipUnits);
  OS_ASSERT(test);
  test = convert(1.0, modelUnits, siUnits);
  OS_ASSERT(test);

  this->setAcceptDrops(false);
  m_lineEdit->setAcceptDrops(false);
  setEnabled(true);

  auto* hLayout = new QHBoxLayout();
  setLayout(hLayout);
  hLayout->setContentsMargins(0, 0, 0, 0);
  hLayout->addWidget(m_lineEdit);
  hLayout->addWidget(m_units);

  m_doubleValidator = new QDoubleValidator();
  // Set the Locale to C, so that "1234.56" is accepted, but not "1234,56", no matter the user's system locale
  QLocale lo(QLocale::C);
  m_doubleValidator->setLocale(lo);
  //m_lineEdit->setValidator(m_doubleValidator);

  m_lineEdit->setMinimumWidth(60);

  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_units->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_lineEdit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

void OSNonModelObjectQuantityEdit::setMinimumValue(double min) {
  m_doubleValidator->setBottom(min);
}

void OSNonModelObjectQuantityEdit::setMaximumValue(double max) {
  m_doubleValidator->setTop(max);
}

void OSNonModelObjectQuantityEdit::enableClickFocus() {
  m_lineEdit->enableClickFocus();
}

bool OSNonModelObjectQuantityEdit::locked() const {
  return m_lineEdit->locked();
}

void OSNonModelObjectQuantityEdit::setLocked(bool locked) {
  m_lineEdit->setLocked(locked);
}

QDoubleValidator* OSNonModelObjectQuantityEdit::doubleValidator() {
  return m_doubleValidator;
}

void OSNonModelObjectQuantityEdit::onEditingFinished() {

  qDebug() << "OSNonModelObjectQuantityEdit::onEditingFinished";

  emit inFocus(m_lineEdit->focused(), m_lineEdit->hasData());

  QString text = m_lineEdit->text();
  if (m_text == text) {
    return;
  }

  int pos = 0;
  QValidator::State state = m_doubleValidator->validate(text, pos);
  if (state != QValidator::Acceptable) {
    if (text.isEmpty()) {
      m_valueModelUnits.reset();
    }
    refreshTextAndLabel();
    return;
  }

  const std::string str = text.toStdString();
  bool ok = false;
  double value = text.toDouble(&ok);
  if (!ok) {
    refreshTextAndLabel();
  }

  setPrecision(str);

  std::string units;
  if (m_isIP) {
    units = m_ipUnits;
  } else {
    units = m_siUnits;
  }
  m_unitsStr = units;

  boost::optional<double> modelValue = convert(value, units, m_modelUnits);
  OS_ASSERT(modelValue);
  m_valueModelUnits = *modelValue;
  refreshTextAndLabel();
}

void OSNonModelObjectQuantityEdit::onUnitSystemChange(bool isIP) {
  m_isIP = isIP;
  refreshTextAndLabel();
}

void OSNonModelObjectQuantityEdit::updateStyle() {
  // will also call m_lineEdit->updateStyle()
  m_lineEdit->setDefaultedAndAuto(defaulted(), false);
}

bool OSNonModelObjectQuantityEdit::setDefault(double defaultValue) {
  if (defaultValue >= m_doubleValidator->bottom() && defaultValue <= m_doubleValidator->top()) {
    m_defaultValue = defaultValue;
    refreshTextAndLabel();
    return true;
  }
  return false;
}

bool OSNonModelObjectQuantityEdit::defaulted() const {
  return !m_valueModelUnits.has_value();
}

double OSNonModelObjectQuantityEdit::currentValue() const {
  return m_valueModelUnits ? *m_valueModelUnits : m_defaultValue;
}

bool OSNonModelObjectQuantityEdit::setCurrentValue(double valueModelUnits) {
  if (valueModelUnits >= m_doubleValidator->bottom() && valueModelUnits <= m_doubleValidator->top()) {
    m_valueModelUnits = valueModelUnits;
    refreshTextAndLabel();
    return true;
  }
  return false;
}

void OSNonModelObjectQuantityEdit::refreshTextAndLabel() {

  QString text = m_lineEdit->text();

  std::string units;
  if (m_isIP) {
    units = m_ipUnits;
  } else {
    units = m_siUnits;
  }

  //if (m_text == text && m_unitsStr == units) return;

  QString textValue;
  std::stringstream ss;
  const double value = m_valueModelUnits ? *m_valueModelUnits : m_defaultValue;

  boost::optional<double> displayValue = convert(value, m_modelUnits, units);
  OS_ASSERT(displayValue);

  if (m_isScientific) {
    ss << std::scientific;
  } else {
    ss << std::fixed;
  }
  if (m_precision) {

    // check if precision is too small to display value
    const int precision = *m_precision;
    const double minValue = std::pow(10.0, -precision);
    if (*displayValue < minValue) {
      m_precision.reset();
    }

    if (m_precision) {
      ss << std::setprecision(*m_precision);
    }
  }
  ss << *displayValue;
  textValue = QString::fromStdString(ss.str());
  ss.str("");

  if (m_text != textValue || text != textValue || m_unitsStr != units) {
    m_text = textValue;
    m_unitsStr = units;
    m_lineEdit->blockSignals(true);
    m_lineEdit->setText(textValue);
    updateStyle();
    m_lineEdit->blockSignals(false);
  }

  ss << units;
  m_units->blockSignals(true);
  m_units->setTextFormat(Qt::RichText);
  // m_units->setText(toQString(formatUnitString(ss.str(), DocumentFormat::XHTML)));
  m_units->setText(QString::fromStdString(units));
  m_units->blockSignals(false);

  emit(valueChanged(currentValue()));
}

void OSNonModelObjectQuantityEdit::setPrecision(const std::string& str) {
  boost::regex rgx("-?([[:digit:]]*)(\\.)?([[:digit:]]+)([EDed][-\\+]?[[:digit:]]+)?");
  boost::smatch m;
  if (boost::regex_match(str, m, rgx)) {
    std::string sci, prefix, postfix;
    if (m[1].matched) {
      prefix = std::string(m[1].first, m[1].second);
    }
    if (m[3].matched) {
      postfix = std::string(m[3].first, m[3].second);
    }
    if (m[4].matched) {
      sci = std::string(m[4].first, m[4].second);
    }
    m_isScientific = !sci.empty();

    if (m_isScientific) {
      m_precision = prefix.size() + postfix.size() - 1;
    } else {
      if (m[2].matched) {
        m_precision = postfix.size();
      } else {
        m_precision = 0;
      }
    }
  } else {
    m_isScientific = false;
    m_precision.reset();
  }
}

}  // namespace openstudio
