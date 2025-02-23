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
#include "PinAssignmentCreator.h"

#include <QBoxLayout>
#include <QDir>
#include <filesystem>

#include "Main/ToolContext.h"
#include "PackagePinsLoader.h"
#include "PackagePinsView.h"
#include "PinsBaseModel.h"
#include "PortsLoader.h"
#include "PortsView.h"
#include "Utils/QtUtils.h"

namespace FOEDAG {

QMap<QString, PackagePinsLoader *> PinAssignmentCreator::m_loader{};
QMap<QString, PortsLoader *> PinAssignmentCreator::m_portsLoader{};

PinAssignmentCreator::PinAssignmentCreator(const PinAssignmentData &data,
                                           QObject *parent)
    : QObject(parent) {
  PortsModel *portsModel = new PortsModel{this};
  auto packagePinModel = new PackagePinsModel;
  const QString fileName = searchCsvFile(data.target, data.context);
  m_baseModel = new PinsBaseModel;
  m_baseModel->setPackagePinModel(packagePinModel);
  m_baseModel->setPortsModel(portsModel);

  PortsLoader *portsLoader{FindPortsLoader(data.target)};
  portsLoader->load(searchPortsFile(data.projectPath));

  PackagePinsLoader *loader{FindPackagePinLoader(data.target)};
  loader->loadHeader(packagePinHeaderFile(data.context));
  loader->load(fileName);

  auto portsView = new PortsView(m_baseModel);
  connect(portsView, &PortsView::selectionHasChanged, this,
          &PinAssignmentCreator::changed);
  m_portsView = CreateLayoutedWidget(portsView);

  auto packagePins = new PackagePinsView(m_baseModel);
  connect(packagePins, &PackagePinsView::selectionHasChanged, this,
          &PinAssignmentCreator::changed);
  m_packagePinsView = CreateLayoutedWidget(packagePins);
  parseConstraints(data.commands, packagePins, portsView);
}

QWidget *PinAssignmentCreator::GetPackagePinsWidget() {
  return m_packagePinsView;
}

QWidget *PinAssignmentCreator::GetPortsWidget() { return m_portsView; }

QString PinAssignmentCreator::generateSdc() const {
  QString sdc;
  const auto pinMap = m_baseModel->pinMap();
  // generate pin location
  for (auto it = pinMap.constBegin(); it != pinMap.constEnd(); ++it) {
    sdc.append(QString("set_pin_loc %1 %2\n").arg(it.key(), it.value()));
  }
  // generate mode
  auto modeMap = m_baseModel->packagePinModel()->modeMap();
  for (auto it{modeMap.begin()}; it != modeMap.end(); ++it) {
    sdc.append(QString("set_mode %1 %2\n").arg(it.value(), it.key()));
  }
  return sdc;
}

QWidget *PinAssignmentCreator::CreateLayoutedWidget(QWidget *main) {
  QWidget *w = new QWidget;
  w->setLayout(new QVBoxLayout);
  w->layout()->addWidget(main);
  return w;
}

QString PinAssignmentCreator::searchCsvFile(const QString &targetDevice,
                                            ToolContext *context) const {
  std::filesystem::path path{context->DataPath()};
  path = path / "etc" / "devices";
  if (!targetDevice.isEmpty()) path /= targetDevice.toLower().toStdString();

  QDir dir{path.string().c_str()};
  auto files = dir.entryList({"*.csv"}, QDir::Files);
  if (!files.isEmpty()) return dir.filePath(files.first());

  std::filesystem::path pathDefault{context->DataPath()};
  pathDefault = pathDefault / "etc" / "templates" / "Pin_Table.csv";
  return QString(pathDefault.string().c_str());
}

QString PinAssignmentCreator::packagePinHeaderFile(ToolContext *context) const {
  auto path = context->DataPath() / "etc" / "package_pin_info.json";
  return QString::fromStdString(path.string());
}

PackagePinsLoader *PinAssignmentCreator::FindPackagePinLoader(
    const QString &targetDevice) const {
  if (!m_loader.contains(targetDevice)) {
    RegisterPackagePinLoader(targetDevice, new PackagePinsLoader{nullptr});
  }
  auto loader = m_loader.value(targetDevice);
  loader->setModel(m_baseModel->packagePinModel());
  return loader;
}

PortsLoader *PinAssignmentCreator::FindPortsLoader(
    const QString &targetDevice) const {
  if (!m_portsLoader.contains(targetDevice)) {
    RegisterPortsLoader(targetDevice, new PortsLoader{nullptr});
  }
  auto loader = m_portsLoader.value(targetDevice);
  loader->SetModel(m_baseModel->portsModel());
  return loader;
}

void PinAssignmentCreator::parseConstraints(const QStringList &commands,
                                            PackagePinsView *packagePins,
                                            PortsView *portsView) {
  // First need to setup ports and then modes sinse mode will apply only when
  // port is selected.
  for (const auto &cmd : commands) {
    if (cmd.startsWith("set_pin_loc")) {
      auto list = QtUtils::StringSplit(cmd, ' ');
      if (list.size() >= 3) {
        portsView->SetPin(list.at(1), list.at(2));
      }
    }
  }
  for (const auto &cmd : commands) {
    if (cmd.startsWith("set_mode")) {
      auto list = QtUtils::StringSplit(cmd, ' ');
      if (list.size() >= 3) {
        packagePins->SetMode(list.at(2), list.at(1));
      }
    }
  }
}

QString PinAssignmentCreator::searchPortsFile(const QString &projectPath) {
  const QDir dir{projectPath};
  auto file = dir.filePath("port_info.json");
  const QFileInfo fileInfo{file};
  if (fileInfo.exists()) return file;
  return QString();
}

void PinAssignmentCreator::RegisterPackagePinLoader(const QString &device,
                                                    PackagePinsLoader *l) {
  m_loader.insert(device, l);
}

void PinAssignmentCreator::RegisterPortsLoader(const QString &device,
                                               PortsLoader *l) {
  m_portsLoader.insert(device, l);
}

PinsBaseModel *PinAssignmentCreator::baseModel() const { return m_baseModel; }

}  // namespace FOEDAG
