#ifndef SHAREDGUICOMPONENTS_BUTTONS_HPP
#define SHAREDGUICOMPONENTS_BUTTONS_HPP

#include <QPushButton>

namespace openstudio {

class AddButton : public QPushButton
{
  Q_OBJECT

 public:
  explicit AddButton(QWidget* parent = nullptr);
  virtual ~AddButton() = default;
};

class RemoveButton : public QPushButton
{
  Q_OBJECT

 public:
  explicit RemoveButton(QWidget* parent = nullptr);
  virtual ~RemoveButton() = default;
};

}  // namespace openstudio

#endif  // SHAREDGUICOMPONENTS_BUTTONS_HPP
