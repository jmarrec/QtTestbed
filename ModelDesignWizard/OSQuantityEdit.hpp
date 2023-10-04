#ifndef SHAREDGUICOMPONENTS_OSQUANTITYEDIT_HPP
#define SHAREDGUICOMPONENTS_OSQUANTITYEDIT_HPP

#include <QLineEdit>
#include <QLabel>
#include <QString>
#include <QValidator>

#include <boost/optional.hpp>

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

class OSNonModelObjectQuantityEdit : public QWidget
{
  Q_OBJECT
 public:
  OSNonModelObjectQuantityEdit(const std::string& modelUnits, const std::string& siUnits, const std::string& ipUnits, bool isIP,
                               QWidget* parent = nullptr);

  virtual ~OSNonModelObjectQuantityEdit() = default;

  void enableClickFocus();

  void disableClickFocus();

  bool locked() const;

  void setLocked(bool locked);

  QDoubleValidator* doubleValidator();
  void setMinimumValue(double min);
  void setMaximumValue(double max);

  bool setDefault(double defaultValue);

 signals:

  void inFocus(bool inFocus, bool hasData);

 public slots:

  void onUnitSystemChange(bool isIP);

 private slots:

  void onEditingFinished();

 private:
  bool defaulted() const;
  void updateStyle();

  QuantityLineEdit* m_lineEdit;
  QLabel* m_units;
  QString m_text = "UNINITIALIZED";
  std::string m_unitsStr;
  QDoubleValidator* m_doubleValidator;
  double m_defaultValue = 0.0;
  boost::optional<double> m_valueModelUnits;

  bool m_isIP;
  std::string m_modelUnits;
  std::string m_siUnits;
  std::string m_ipUnits;

  bool m_isScientific;
  boost::optional<int> m_precision;

  void refreshTextAndLabel();

  void setPrecision(const std::string& str);
};

}  // namespace openstudio

#endif  // SHAREDGUICOMPONENTS_OSQUANTITYEDIT_HPP
