# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import csv
import io
import json
import sys
from dataclasses import dataclass, fields
# from functools import cache   # cache needs python 3.9
from functools import lru_cache
from pathlib import Path

from loguru import logger
from PySide6 import QtCore, QtGui, QtWidgets
from PySide6.QtCore import QAbstractTableModel, QEvent, QModelIndex, QSortFilterProxyModel, QIdentityProxyModel, Qt
from PySide6.QtWidgets import QApplication, QTableView


@dataclass
class SpaceTypeRatioRow:
    """Class for keeping track of an item in inventory."""

    buildingType: str
    spaceType: str
    ratio: float
    floorArea: float


@lru_cache(maxsize=None)
def _load_support_json():
    return json.load(Path("ModelDesignWizard.json").open("r"))


def prepareDefaultSpaceTypeRatioRows(
    standardType: str, standard: str, buildingType: str, totalBuildingFloorArea: float
):
    data = _load_support_json()
    rows = []
    for space_type, info in data[standardType]["space_types"][standard][buildingType].items():
        ratio = info["ratio"]
        rows.append(
            SpaceTypeRatioRow(
                buildingType=buildingType, spaceType=space_type, ratio=ratio, floorArea=totalBuildingFloorArea * ratio
            )
        )
    return rows


class SpaceTypeModel(QAbstractTableModel):
    """A model to interface a Qt view with pandas dataframe"""

    def __init__(self, parent=None):
        QAbstractTableModel.__init__(self, parent)
        self._support_data = _load_support_json()

        # file = QtCore.QFile("ModelDesignWizard.json")
        # if file.open(QtCore.QIODevice.ReadOnly | QtCore.QIODevice.Text):
        #     supportJson = QtCore.QJsonDocument.fromJson(file.readAll())
        #     m_supportJsonObject = supportJson.object()
        #     file.close()
        # else:
        #     logger.error("Failed to open embedded ModelDesignWizard.json")

        self.totalBuildingFloorArea = 10000.0
        self.selectedStandardType = None
        self.selectedStandard = None
        self.selectedPrimaryBuildingType = None
        self.rows = []


    def populateWithDefaults(self, selectedStandardType: str, selectedStandard: str, selectedPrimaryBuildingType: str):
        self.selectedStandardType = selectedStandardType
        self.selectedStandard = selectedStandard
        self.selectedPrimaryBuildingType = selectedPrimaryBuildingType
        self.totalBuildingFloorArea = 10000.0
        self.rows = prepareDefaultSpaceTypeRatioRows(
            standardType=self.selectedStandardType,
            standard=self.selectedStandard,
            buildingType=self.selectedPrimaryBuildingType,
            totalBuildingFloorArea=self.totalBuildingFloorArea,
        )
        self.layoutChanged.emit()

    def setTotalBuildingFloorArea(self, floorArea):
        self.totalBuildingFloorArea = floorArea
        for row in self.rows:
            row.floorArea = row.ratio * floorArea

    def rowCount(self, parent=QModelIndex()) -> int:
        """Override method from QAbstractTableModel

        Return row count of the pandas DataFrame
        """
        if parent == QModelIndex():
            return len(self.rows)

        return 0

    def columnCount(self, parent=QModelIndex()) -> int:
        """Override method from QAbstractTableModel

        Return column count of the pandas DataFrame
        """
        if parent == QModelIndex():
            return len(fields(SpaceTypeRatioRow))
        return 0

    def data(self, index: QModelIndex, role=Qt.ItemDataRole):
        """Override method from QAbstractTableModel

        Return data cell from the pandas DataFrame
        """
        if not index.isValid():
            return None

        if role == Qt.DisplayRole:
            row = index.row()
            col = index.column()
            return getattr(self.rows[row], fields(SpaceTypeRatioRow)[col].name)

        if role == Qt.TextAlignmentRole:
            return Qt.AlignmentFlag.AlignCenter

        return None

    def flags(self, index):
        if index.column() == 3:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable
        return Qt.ItemIsEditable | Qt.ItemIsEnabled | Qt.ItemIsSelectable

    def setData(self, index: QModelIndex, value, role=Qt.EditRole) -> bool:
        row = index.row()
        col = index.column()

        if col >= len(fields(SpaceTypeRatioRow)):
            print("WRONG")
            return False

        field = fields(SpaceTypeRatioRow)[col]
        try:
            cast_value = field.type(value)
        except ValueError as e:
            print(f"Failed to set field '{field.name}' at ({row}, {col}): {e}")
            return False

        if role == Qt.EditRole:
            setattr(self.rows[row], fields(SpaceTypeRatioRow)[col].name, cast_value)
            if col == 2:
                setattr(self.rows[row], fields(SpaceTypeRatioRow)[3].name, cast_value * self.totalBuildingFloorArea)
            self.dataChanged.emit(index, index)
            return True
        return False

    def headerData(self, section: int, orientation: Qt.Orientation, role: Qt.ItemDataRole):
        """Override method from QAbstractTableModel

        Return dataframe index as vertical header data and columns as horizontal header data.
        """
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal:
                return fields(SpaceTypeRatioRow)[section].name

            if orientation == Qt.Vertical:
                return str(section)

        return None


def sort_selectedIndexes(selectedIndexes):
    selectedIndexes.sort(key=lambda index: (index.row(), index.column()))


def print_selectedIndexes(selectedIndexes):
    print([(x.row(), x.column()) for x in selectedIndexes])


@dataclass
class UndoCellInfo:
    """Class for keeping track of an undo operation"""

    row: int
    column: int
    value: str


class ComboBoxDelegate(QtWidgets.QStyledItemDelegate):
    def __init__(self, items):
        super(ComboBoxDelegate, self).__init__()
        self.items = items

    def createEditor(self, parent, option, index):
        editor = QtWidgets.QComboBox(parent)
        editor.setAutoFillBackground(True)
        editor.addItems(self.items)
        return editor

    def setEditorData(self, editor, index):
        current_index = editor.findText(index.model().data(index), Qt.MatchExactly)
        editor.setCurrentIndex(current_index)

    def setModelData(self, editor, model, index):
        # item_index = model.mapToSource(index)
        # item = model.sourceModel().item(item_index.row(), 0)

        # if index.parent().row() == -1 and item.hasChildren():
        #     for row in range(item.rowCount()):
        #         child = item.child(row, 3)
        #         child.setText(editor.currentText())

        model.setData(index, editor.currentText())

    def updateEditorGeometry(self, editor, option, index):
        editor.setGeometry(option.rect)


class CopyPastableTableView(QTableView):
    def __init__(self, *args, **kwargs):
        super(CopyPastableTableView, self).__init__(*args, **kwargs)
        self.installEventFilter(self)
        self.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.forundo = []

        data = _load_support_json()
        self.building_type_delegate = ComboBoxDelegate(data["DOE"]["building_types"])
        self.setItemDelegateForColumn(0, self.building_type_delegate)

    def eventFilter(self, source, event):
        if event.type() == QtCore.QEvent.KeyPress:
            if event == QtGui.QKeySequence.Copy:  # event.matches(QKeySequence.Copy)
                self.copySelection()
                return True
            elif event == QtGui.QKeySequence.Paste:
                self.pasteSelection()
                return True
            elif event == QtGui.QKeySequence.Delete:
                self.deleteSelection()
                return True
            elif event == QtGui.QKeySequence.Cut:
                self.copySelection()
                self.deleteSelection()
                return True
            elif event == QtGui.QKeySequence.Undo:
                self.undoSelection()
                return True
        return super(CopyPastableTableView, self).eventFilter(source, event)

    def copySelection(self):
        selection = self.selectedIndexes()
        print(selection)
        if selection:
            all_rows = []
            all_columns = []
            for index in selection:
                if not index.row() in all_rows:
                    all_rows.append(index.row())
                if not index.column() in all_columns:
                    all_columns.append(index.column())
            visible_rows = [row for row in all_rows if not self.isRowHidden(row)]
            visible_columns = [col for col in all_columns if not self.isColumnHidden(col)]
            table = [[""] * len(visible_columns) for _ in range(len(visible_rows))]
            for index in selection:
                if index.row() in visible_rows and index.column() in visible_columns:
                    selection_row = visible_rows.index(index.row())
                    selection_column = visible_columns.index(index.column())
                    table[selection_row][selection_column] = index.data()
            stream = io.StringIO()
            csv.writer(stream, delimiter="\t").writerows(table)
            QApplication.clipboard().setText(stream.getvalue())

    def pasteSelection(self):
        selection = self.selectedIndexes()
        if not selection:
            return

        print_selectedIndexes(selection)

        buffer = QApplication.clipboard().text()
        reader = csv.reader(io.StringIO(buffer), delimiter="\t")
        arr = [[cell for cell in row] for row in reader]
        if not arr:
            return

        model = self.model()

        visible_rows = list(set([row for index in selection if not self.isRowHidden(row := index.row())]))
        visible_columns = list(
            set([column for index in selection if not self.isColumnHidden(column := index.column()) > 0])
        )

        this_undo = []

        nrows = len(arr)
        ncols = len(arr[0])
        is_only_top_cell = len(visible_rows) == 1 and len(visible_columns) == 1
        is_single_value = nrows == 1 and ncols == 1

        if is_only_top_cell:
            # Only the top-left cell is highlighted.
            insert_rows = [visible_rows[0]]
            row = insert_rows[0] + 1
            while len(insert_rows) < nrows:
                if not self.isRowHidden(row):
                    insert_rows.append(row)
                row += 1
            insert_columns = [visible_columns[0]]
            col = insert_columns[0] + 1
            while len(insert_columns) < ncols:
                if not self.isColumnHidden(col):
                    insert_columns.append(col)
                col += 1
            for i, insert_row in enumerate(insert_rows):
                for j, insert_column in enumerate(insert_columns):
                    cell = arr[i][j]
                    old_value = model.index(insert_row, insert_column).data()
                    this_undo.append(UndoCellInfo(insert_row, insert_column, old_value))
                    model.setData(model.index(insert_row, insert_column), cell)
        elif is_single_value:
            single_value = arr[0][0]
            for index in selection:
                selection_row = visible_rows.index(index.row())
                selection_column = visible_columns.index(index.column())
                old_value = model.index(index.row(), index.column()).data()
                this_undo.append(UndoCellInfo(index.row(), index.column(), old_value))
                model.setData(
                    model.index(index.row(), index.column()),
                    single_value,
                )
        else:
            if nrows != len(visible_rows) == 1 or ncols != len(visible_columns):
                print("Selection size mistmatch")
                return

            # Assume the selection size matches the clipboard data size.
            for index in selection:
                selection_row = visible_rows.index(index.row())
                selection_column = visible_columns.index(index.column())
                old_value = model.index(index.row(), index.column()).data()
                this_undo.append(UndoCellInfo(index.row(), index.column(), old_value))
                model.setData(
                    model.index(index.row(), index.column()),
                    arr[selection_row][selection_column],
                )

        print(this_undo)
        self.forundo.append(this_undo)

    def deleteSelection(self):
        self.forundo.append([self.selectedRanges()[0].topRow(), self.selectedRanges()[0].leftColumn(), []])
        for i_row in range(self.selectedRanges()[0].topRow(), self.selectedRanges()[0].bottomRow() + 1):
            undorow = []
            for i_col in range(self.selectedRanges()[0].leftColumn(), self.selectedRanges()[0].rightColumn() + 1):
                undorow.append(self.item(i_row, i_col).text())
                self.setItem(i_row, i_col, QtWidgets.QTableWidgetItem(""))
            self.forundo[-1][2].append(undorow)

    def undoSelection(self):
        if len(self.forundo) > 0:
            prevundo = self.forundo.pop()
            print(prevundo)
            model = self.model()
            self.setCurrentIndex(model.index(prevundo[0].row, prevundo[0].column))
            for undoCellInfo in prevundo:
                model.setData(
                    model.index(undoCellInfo.row, undoCellInfo.column),
                    undoCellInfo.value,
                )


class MyProxyModel(QSortFilterProxyModel):
    def __init__(self, parent=None):
        QSortFilterProxyModel.__init__(self, parent)


class AddBuildingFloorAreaAndRemoveButtonProxy(QIdentityProxyModel):
    def __init__(self, parent=None):
        super(AddBuildingFloorAreaAndRemoveButtonProxy, self).__init__(parent)

    def rowCount(self, parent=QModelIndex()) -> int:
        """Override method from QAbstractTableModel
        """
        return self.sourceModel().rowCount(parent)

    def columnCount(self, parent=QModelIndex()) -> int:
        """Override method from QAbstractTableModel

        Return column count of the pandas DataFrame
        """
        print(parent, self.sourceModel().columnCount() + 2)
        if parent == QModelIndex():
            return self.sourceModel().columnCount() + 2
        return 0

    def data(self, index: QModelIndex, role=Qt.ItemDataRole):
        """Override method from QAbstractTableModel

        Return data cell from the pandas DataFrame
        """
        print(index.row(), index.column())

        col = index.column()
        model = self.sourceModel()

        if role != Qt.DisplayRole:
            return super().data(index, role)

        if col < model.columnCount():
            return super().data(index, role)

        print(col)

        if col == model.columnCount():
            return model.index(index.row(), col - 1).data(role) * 10


    def flags(self, index):
        if index.column() < self.sourceModel().columnCount():
            return super().flags(index)

        return Qt.ItemIsEnabled | Qt.ItemIsSelectable

    def headerData(self, section: int, orientation: Qt.Orientation, role: Qt.ItemDataRole):
        """Override method from QAbstractTableModel

        Return dataframe index as vertical header data and columns as horizontal header data.
        """
        model = self.sourceModel()

        if orientation == Qt.Horizontal and section == model.columnCount():
            return "Proxy area"
        if orientation == Qt.Horizontal and section == model.columnCount() + 1:
            return "Remove"

        return super().headerData(section, orientation, role)


class TemplateSelection(QtCore.QObject):

    primaryBuildingTypeChanged = QtCore.Signal(str, str, str)

    def __init__(self):
        super(TemplateSelection, self).__init__()

        self.data = _load_support_json()

        self.mainLayout = QtWidgets.QVBoxLayout()

        firstRowLayout = QtWidgets.QHBoxLayout()
        self.mainLayout.addLayout(firstRowLayout)

        standardTypeLayout = QtWidgets.QVBoxLayout()
        firstRowLayout.addLayout(standardTypeLayout)
        standardTypeLabel = QtWidgets.QLabel("Standard Template Type:")
        standardTypeLabel.setObjectName("H2")
        standardTypeLayout.addWidget(standardTypeLabel)

        self.standardTypeComboBox = QtWidgets.QComboBox();
        self.standardTypeComboBox.addItems(self.data.keys())
        self.standardTypeComboBox.currentTextChanged.connect(self.onStandardTypeChanged)
        standardTypeLayout.addWidget(self.standardTypeComboBox);


        targetStandardLayout = QtWidgets.QVBoxLayout()
        firstRowLayout.addLayout(targetStandardLayout)
        targetStandardLabel = QtWidgets.QLabel("Target Standard:")
        targetStandardLabel.setObjectName("H2")
        targetStandardLayout.addWidget(targetStandardLabel)

        self.targetStandardComboBox = QtWidgets.QComboBox();
        targetStandardLayout.addWidget(self.targetStandardComboBox);


        primaryBuildingTypeLayout = QtWidgets.QVBoxLayout()
        firstRowLayout.addLayout(primaryBuildingTypeLayout)
        primaryBuildingTypeLabel = QtWidgets.QLabel("Primary Building Template Type:")
        primaryBuildingTypeLabel.setObjectName("H2")
        primaryBuildingTypeLayout.addWidget(primaryBuildingTypeLabel)

        self.primaryBuildingTypeComboBox = QtWidgets.QComboBox();
        primaryBuildingTypeLayout.addWidget(self.primaryBuildingTypeComboBox);
        self.primaryBuildingTypeComboBox.currentTextChanged.connect(self.onPrimaryBuildingTypeChanged)



        secondRowLayout = QtWidgets.QHBoxLayout()
        self.mainLayout.addLayout(secondRowLayout)
        buildingFloorAreaLabel = QtWidgets.QLabel("Building Floor Area:")
        buildingFloorAreaLabel.setObjectName("H2")
        secondRowLayout.addWidget(buildingFloorAreaLabel)

        self.buildingFloorAreaEdit = QtWidgets.QLineEdit()
        positiveDoubleValidator = QtGui.QDoubleValidator()
        positiveDoubleValidator.setBottom(0)
        positiveDoubleValidator.setDecimals(2)
        self.buildingFloorAreaEdit.setValidator(positiveDoubleValidator)
        self.buildingFloorAreaEdit.setText(str(10000.0))
        secondRowLayout.addWidget(self.buildingFloorAreaEdit)


    def onPrimaryBuildingTypeChanged(self):
        selectedStandardType = self.standardTypeComboBox.currentText()
        selectedStandard = self.targetStandardComboBox.currentText()
        selectedPrimaryBuildingType = self.primaryBuildingTypeComboBox.currentText()
        self.primaryBuildingTypeChanged.emit(selectedStandardType, selectedStandard, selectedPrimaryBuildingType)

    def onStandardTypeChanged(self, text):
        self.populateTargetStandards()
        self.populatePrimaryBuildingTypes()

    def populateTargetStandards(self):
        self.targetStandardComboBox.blockSignals(True)
        self.targetStandardComboBox.clear()

        self.targetStandardComboBox.addItem("")

        selectedStandardType = self.standardTypeComboBox.currentText()
        for temp in self.data[selectedStandardType]["templates"]:
            self.targetStandardComboBox.addItem(temp);
        self.targetStandardComboBox.setCurrentIndex(0)

        self.targetStandardComboBox.blockSignals(False)

    def populatePrimaryBuildingTypes(self):
        self.populateBuildingTypeComboBox(self.primaryBuildingTypeComboBox)

    def populateBuildingTypeComboBox(self, comboBox):
        comboBox.blockSignals(True)
        comboBox.clear()

        comboBox.addItem("")

        selectedStandardType = self.standardTypeComboBox.currentText()
        for temp in self.data[selectedStandardType]["building_types"]:
            comboBox.addItem(temp);
        comboBox.setCurrentIndex(0)

        comboBox.blockSignals(False)


USE_SORTABLE_PROXY = False
USE_MY_PROXY = False

if __name__ == "__main__":
    app = QApplication(sys.argv)

    QtCore.QLocale.setDefault(QtCore.QLocale.c())

    mainWindow = QtWidgets.QMainWindow()
    mainWindow.resize(QtGui.QGuiApplication.primaryScreen().availableGeometry().size() * 0.7)
    centralWidget = QtWidgets.QWidget()
    mainWindow.setCentralWidget(centralWidget)
    # centralWidget.resize(800, 500)

    mainLayout = QtWidgets.QVBoxLayout()
    centralWidget.setLayout(mainLayout)

    template_selection = TemplateSelection()
    mainLayout.addLayout(template_selection.mainLayout)
    template_selection.standardTypeComboBox.setCurrentText("DOE");


    tableView = CopyPastableTableView()
    mainLayout.addWidget(tableView)
    # tableView.resize(800, 500)
    tableView.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
    tableView.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
    tableView.horizontalHeader().setStretchLastSection(False)
    # tableView.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.ResizeToContents)
    tableView.horizontalHeader().setDefaultSectionSize(200)
    tableView.setAlternatingRowColors(True)
    # tableView.setSelectionBehavior(QTableView.SelectRows)

    model = SpaceTypeModel()

    if USE_SORTABLE_PROXY:
        proxy = MyProxyModel()
        proxy.setSourceModel(model)
        tableView.setModel(proxy)
        tableView.setSortingEnabled(True)
        proxy.sort(2, Qt.DescendingOrder)
    elif USE_MY_PROXY:
        proxy = AddBuildingFloorAreaAndRemoveButtonProxy()
        proxy.setSourceModel(model)
        tableView.setModel(proxy)
    else:
        tableView.setModel(model)

    template_selection.primaryBuildingTypeChanged.connect(model.populateWithDefaults)
    template_selection.buildingFloorAreaEdit.editingFinished.connect(lambda:
                                                                     model.setTotalBuildingFloorArea(float(template_selection.buildingFloorAreaEdit.text())))


    mainWindow.show()
    app.exec()
