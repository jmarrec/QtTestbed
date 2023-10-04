#ifndef SHAREDGUICOMPONENTS_OSQUANTITYEDIT_HPP
#define SHAREDGUICOMPONENTS_OSQUANTITYEDIT_HPP

#include <QLineEdit>
#include <QLabel>
#include <QString>
#include <QValidator>

class QFocusEvent;

namespace openstudio {

class Unit;

class QuantityLineEdit : public QLineEdit
{
  Q_OBJECT
 public:
  explicit QuantityLineEdit(QWidget* parent = nullptr);

  virtual ~QuantityLineEdit() = default;

  void enableClickFocus();

  void disableClickFocus();

  bool hasData() const;

  bool focused() const;

  void setDefaultedAndAuto(bool defaulted, bool isAuto);

  bool locked() const;

  void setLocked(bool locked);

  void updateStyle();

 protected:
  virtual void focusInEvent(QFocusEvent* e) override;

  virtual void focusOutEvent(QFocusEvent* e) override;

 private:
  bool m_hasClickFocus = false;
  bool m_defaulted = false;
  bool m_auto = false;
  bool m_focused = false;
  bool m_locked = false;

 signals:

  void inFocus(bool inFocus, bool hasData);
};

}  // namespace openstudio

#endif  // SHAREDGUICOMPONENTS_OSQUANTITYEDIT_HPP
