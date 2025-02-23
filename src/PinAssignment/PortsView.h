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
#pragma once

#include "PinAssignmentBaseView.h"

class QComboBox;
namespace FOEDAG {

struct IOPort;
class PortsView : public PinAssignmentBaseView {
  Q_OBJECT
 public:
  PortsView(PinsBaseModel *model, QWidget *parent = nullptr);
  void SetPin(const QString &port, const QString &pin);

 signals:
  void selectionHasChanged();

 private:
  void packagePinSelectionHasChanged(const QModelIndex &index);
  void insertTableItem(QTreeWidgetItem *parent, const IOPort &port);
  static QString normalizeName(const QString &p);
  void modeSelectionHasChanged(const QModelIndex &index);
  void updateModeCombo(const QString &port, const QModelIndex &index);

 private slots:
  void itemHasChanged(const QModelIndex &index, const QString &pin);
  void modeChanged(const QString &pin, const QString &mode);
};

}  // namespace FOEDAG
