/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "PortsView.h"

#include <QCompleter>
#include <QHeaderView>
#include <QStringListModel>

#include "BufferedComboBox.h"
#include "PinsBaseModel.h"

namespace FOEDAG {

constexpr uint PortName{0};
constexpr uint DirCol{1};
constexpr uint PackagePinCol{2};
constexpr uint ModeCol{3};
constexpr uint TypeCol{4};

PortsView::PortsView(PinsBaseModel *model, QWidget *parent)
    : PinAssignmentBaseView(model, parent) {
  setHeaderLabels(model->portsModel()->headerList());
  header()->resizeSections(QHeaderView::ResizeToContents);

  QTreeWidgetItem *topLevel = new QTreeWidgetItem(this);
  topLevel->setText(0, "Design ports");
  addTopLevelItem(topLevel);
  auto portsModel = model->portsModel();
  for (const auto &group : portsModel->ports()) {
    for (const auto &p : group.ports) {
      if (p.isBus) {
        auto item = new QTreeWidgetItem;
        item->setText(PortName, p.name);
        topLevel->addChild(item);
        for (const auto &subPort : p.ports) insertTableItem(item, subPort);
      } else {
        insertTableItem(topLevel, p);
      }
    }
  }
  connect(model->portsModel(), &PortsModel::itemHasChanged, this,
          &PortsView::itemHasChanged);
  connect(model->packagePinModel(), &PackagePinsModel::modeHasChanged, this,
          &PortsView::modeChanged);
  expandItem(topLevel);
  setAlternatingRowColors(true);
  setColumnWidth(PortName, 120);
  setColumnWidth(ModeCol, 180);
  resizeColumnToContents(PackagePinCol);
}

void PortsView::SetPin(const QString &port, const QString &pin) {
  QString portNormal{normalizeName(port)};
  QModelIndex index{match(portNormal)};
  if (index.isValid()) {
    auto combo = qobject_cast<BufferedComboBox *>(
        itemWidget(itemFromIndex(index), PackagePinCol));
    if (combo)
      combo->setCurrentIndex(
          combo->findData(normalizeName(pin), Qt::DisplayRole));
  }
}

void PortsView::packagePinSelectionHasChanged(const QModelIndex &index) {
  // update here Mode selection
  auto item = itemFromIndex(index);
  auto combo =
      item ? qobject_cast<BufferedComboBox *>(itemWidget(item, PackagePinCol))
           : nullptr;
  if (combo) {
    const QString port =
        combo->currentText().isEmpty() ? QString{} : item->text(PortName);
    updateModeCombo(port, index);
  }

  if (m_blockUpdate) return;
  if (item) {
    auto combo =
        qobject_cast<BufferedComboBox *>(itemWidget(item, PackagePinCol));
    if (combo) {
      auto pin = combo->currentText();
      removeDuplications(pin, combo);

      auto port = item->text(PortName);
      m_model->update(port, pin);
      m_model->packagePinModel()->itemChange(pin, port);

      // unset previous selection
      auto prevPin = combo->previousText();
      m_model->packagePinModel()->itemChange(prevPin, QString());
      emit selectionHasChanged();
    }
  }
}

void PortsView::insertTableItem(QTreeWidgetItem *parent, const IOPort &port) {
  auto it = new QTreeWidgetItem{parent};
  it->setText(PortName, port.name);
  it->setText(DirCol, port.dir);
  it->setText(TypeCol, port.type);

  auto combo = new BufferedComboBox{this};
  combo->setModel(m_model->packagePinModel()->listModel());
  combo->setAutoFillBackground(true);
  m_allCombo.append(combo);
  combo->setEditable(true);
  auto completer{new QCompleter{m_model->packagePinModel()->listModel()}};
  completer->setFilterMode(Qt::MatchContains);
  combo->setCompleter(completer);
  combo->setInsertPolicy(QComboBox::NoInsert);
  connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [=]() {
            packagePinSelectionHasChanged(indexFromItem(it, PackagePinCol));
          });
  setItemWidget(it, PackagePinCol, combo);
  m_model->portsModel()->insert(it->text(PortName),
                                indexFromItem(it, PortName));

  auto modeCombo = new QComboBox{this};
  modeCombo->setEnabled(modeCombo->count() > 0);
  connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [=]() { modeSelectionHasChanged(indexFromItem(it, ModeCol)); });
  setItemWidget(it, ModeCol, modeCombo);
}

QString PortsView::normalizeName(const QString &p) {
  if (p.contains('@') && p.contains('%')) {
    QString portNormal{p};
    return portNormal.replace('@', '[').replace('%', ']');
  }
  return p;
}

void PortsView::modeSelectionHasChanged(const QModelIndex &index) {
  auto item = itemFromIndex(index);
  if (item) {
    auto comboMode = qobject_cast<QComboBox *>(itemWidget(item, ModeCol));
    if (comboMode) {
      auto pinIndex =
          model()->index(index.row(), PackagePinCol, index.parent());
      QComboBox *pinCombo{qobject_cast<QComboBox *>(
          itemWidget(itemFromIndex(pinIndex), PackagePinCol))};
      if (pinCombo)
        m_model->packagePinModel()->updateMode(pinCombo->currentText(),
                                               comboMode->currentText());
      emit selectionHasChanged();
    }
  }
}

void PortsView::updateModeCombo(const QString &port, const QModelIndex &index) {
  auto modeIndex = model()->index(index.row(), ModeCol, index.parent());
  QComboBox *modeCombo{
      qobject_cast<QComboBox *>(itemWidget(itemFromIndex(modeIndex), ModeCol))};
  if (modeCombo) {
    if (port.isEmpty()) {
      modeCombo->setCurrentIndex(0);
      modeCombo->setEnabled(false);
    } else {
      modeCombo->setEnabled(true);
      auto ioPort = m_model->portsModel()->GetPort(port);
      const bool output = ioPort.dir == "Output";
      QAbstractItemModel *modeModel =
          output ? m_model->packagePinModel()->modeModelTx()
                 : m_model->packagePinModel()->modeModelRx();
      if (modeCombo->model() != modeModel) {
        modeCombo->setModel(modeModel);
      }
    }
  }
}

void PortsView::itemHasChanged(const QModelIndex &index, const QString &pin) {
  auto item = itemFromIndex(index);
  if (item) {
    auto combo = qobject_cast<QComboBox *>(itemWidget(item, PackagePinCol));
    if (combo) {
      m_blockUpdate = true;
      const int index = combo->findData(pin, Qt::DisplayRole);
      combo->setCurrentIndex(index != -1 ? index : 0);
      if (pin.isEmpty()) m_model->update(item->text(PortName), QString{});
      m_blockUpdate = false;
    }
  }
}

void PortsView::modeChanged(const QString &pin, const QString &mode) {
  if (pin.isEmpty()) return;

  auto port = m_model->getPort(pin);
  QModelIndex index{match(port)};
  QModelIndex modeIndex = model()->index(index.row(), ModeCol, index.parent());
  auto modeCombo =
      qobject_cast<QComboBox *>(itemWidget(itemFromIndex(modeIndex), ModeCol));
  if (modeCombo) {
    const int index = modeCombo->findData(mode, Qt::DisplayRole);
    modeCombo->setCurrentIndex(index);
  }
}

}  // namespace FOEDAG
