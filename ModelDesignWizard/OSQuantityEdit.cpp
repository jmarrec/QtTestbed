#include "OSQuantityEdit.hpp"

#include <QFocusEvent>
#include <QStyle>

#include <bitset>

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

}  // namespace openstudio
