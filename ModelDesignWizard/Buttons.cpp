#include "Buttons.hpp"

namespace openstudio {

AddButton::AddButton(QWidget* parent) : QPushButton(parent) {
  setFlat(true);
  setFixedSize(24, 24);
  setObjectName("AddButton");
}

RemoveButton::RemoveButton(QWidget* parent) : QPushButton(parent) {
  setFlat(true);
  setFixedSize(24, 24);
  setObjectName("DeleteButton");
}

}  // namespace openstudio
