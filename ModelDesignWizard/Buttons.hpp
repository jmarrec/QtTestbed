#ifndef SHAREDGUICOMPONENTS_BUTTONS_HPP
#define SHAREDGUICOMPONENTS_BUTTONS_HPP

#include <QPushButton>

namespace openstudio {
class RemoveButton : public QPushButton
{
  Q_OBJECT

 public:
  explicit RemoveButton(QWidget* parent = nullptr);
  virtual ~RemoveButton() = default;
};

}  // namespace openstudio

#endif  // SHAREDGUICOMPONENTS_BUTTONS_HPP
