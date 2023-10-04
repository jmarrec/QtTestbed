#include "Buttons.hpp"

namespace openstudio {

RemoveButton::RemoveButton(QWidget* parent) : QPushButton(parent) {
  setFlat(true);
  setFixedSize(24, 24);
  setObjectName("DeleteButton");
}

}  // namespace openstudio
