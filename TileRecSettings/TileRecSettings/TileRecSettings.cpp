#include "TileRecSettings.h"

#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QFileInfoList>
#include <QFileInfo>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QComboBox>
#include <QFile>
#include <QDateTime>

TileRecSettings::TileRecSettings(QWidget *parent)
	: QDialog(parent)
{
	QSettings settings;

	this->m_TileRecSettingsFolderName = QString("RECORD");
	this->m_TileRecSettingsDefaultFileName = QString("Set.txt");

	qulonglong runCount = settings.value("app/run_count", 0).toULongLong() + 1;
	settings.setValue("app/run_count", runCount);

	ui.setupUi(this);

	QObject::connect(this->ui.m_BrowseForRecordPathVisible, SIGNAL(clicked()), this, SLOT(on_m_BrowseForRecordPath_clicked()));

	QStringList deviceTreeHeaderLabels;
	deviceTreeHeaderLabels.insert(TileRecSettings::COL_PATH, tr("TileRec Location"));
	deviceTreeHeaderLabels.insert(TileRecSettings::COL_SETTING, tr("Setting"));
	deviceTreeHeaderLabels.insert(TileRecSettings::COL_VALUE, tr("Value"));
	// all labels up to TileRecSettings::COL_COUNT have been assigned here
	this->ui.m_DeviceTree->clear();
	this->ui.m_DeviceTree->setColumnCount(TileRecSettings::COL_COUNT);
	this->ui.m_DeviceTree->setHeaderLabels(deviceTreeHeaderLabels);

	this->TileRecUserPathsUpdate();

	this->m_TileRecSearchTimer = new QTimer(this);
	this->m_TileRecSearchTimer->setInterval(1000);
	this->m_TileRecSearchTimer->setSingleShot(false);
	QObject::connect(this->m_TileRecSearchTimer, &QTimer::timeout, this, &TileRecSettings::TileRecSearch);
	this->m_TileRecSearchTimer->start();
}

bool TileRecSettings::TileRecDeviceTreeDeleteItem(
	QTreeWidgetItem* item)
{
	bool result = false;
	if (item)
	{
		delete item;
		result = true;
	}

	return result;
}

QTreeWidgetItem* TileRecSettings::TileRecDeviceTreeGetItem(
	const QString& path)
{
	int itemIndex = 0;
	while (itemIndex < this->ui.m_DeviceTree->topLevelItemCount())
	{
		QTreeWidgetItem* item = this->ui.m_DeviceTree->topLevelItem(itemIndex);
		QString itemPath = item->text(TileRecSettings::COL_PATH);
		if (itemPath.compare(path, Qt::CaseSensitive) == 0)
		{
			return item;
		}
		itemIndex++;
	}
	return Q_NULLPTR;
}

QTreeWidgetItem* TileRecSettings::TileRecDeviceTreeAddItem(
	const QString& path,
	QString* settingsPath)
{
	QTreeWidgetItem* item = new QTreeWidgetItem();
	item->setText(TileRecSettings::COL_PATH, path);
	this->ui.m_DeviceTree->addTopLevelItem(item);

	QTreeWidgetItem* timeItem = new QTreeWidgetItem();
	timeItem->setText(TileRecSettings::COL_SETTING, tr("Time"));
	timeItem->setText(TileRecSettings::COL_VALUE, tr("Use Computer's Clock"));
	item->addChild(timeItem);

	QTreeWidgetItem* recordingModeItem = new QTreeWidgetItem();
	recordingModeItem->setText(TileRecSettings::COL_SETTING, tr("Record Mode"));
	//recordingModeItem->setText(TileRecSettings::COL_VALUE, tr(""));
	QComboBox* recordModeCombo = new QComboBox();
	recordModeCombo->addItem(tr("Continuous"));
	recordModeCombo->addItem(tr("Voice-activated"));
	item->addChild(recordingModeItem);
	this->ui.m_DeviceTree->setItemWidget(recordingModeItem, TileRecSettings::COL_VALUE, recordModeCombo);

	QTreeWidgetItem* ledModeItem = new QTreeWidgetItem();
	ledModeItem->setText(TileRecSettings::COL_SETTING, tr("LED Mode"));
	//ledModeItem->setText(TileRecSettings::COL_VALUE, tr(""));
	QComboBox* ledModeCombo = new QComboBox();
	ledModeCombo->addItem(tr("Disabled"));
	ledModeCombo->addItem(tr("Flash when recording"));
	item->addChild(ledModeItem);
	this->ui.m_DeviceTree->setItemWidget(ledModeItem, TileRecSettings::COL_VALUE, ledModeCombo);

	item->setExpanded(true);

	QString fileName;
	QString settingsFilePath;
	QDir pathDir(path);
	QStringList nameFilters;
	nameFilters.append("*.txt");
	QStringList entries = pathDir.entryList(nameFilters, QDir::Files);
	while (entries.count() > 0)
	{
		fileName = entries.takeFirst();
		settingsFilePath = pathDir.absoluteFilePath(fileName);
		QFile recordSettingsFile(settingsFilePath);
		if (recordSettingsFile.open(QIODevice::ReadOnly))
		{
			QByteArray recordSettingsData = recordSettingsFile.readAll();
			recordSettingsFile.close();

			QString recordSettingsStr = QString::fromUtf8(recordSettingsData);
			QStringList lines = recordSettingsStr.split(QRegularExpression("(\r\n|\r|\n)"));
			if (lines.count() > 0)
			{
				QString dateTime24hrStr = lines.value(0);

				QString recordModeStr = lines.value(1);
				recordModeCombo->setCurrentIndex(recordModeStr.toInt());

				QString ledModeStr = lines.value(2);
				ledModeCombo->setCurrentIndex(ledModeStr.toInt());

				// done reading settings.
				// break out of this loop
				break;
			}
			else
			{
				// no data.
				// try another file
			}
		}
		else
		{
			// cannot open the file.
			// try another file
		}
	}

	if (settingsPath != Q_NULLPTR)
	{
		// report the settings file path used to load settings
		(*settingsPath) = settingsFilePath;
	}

	if (settingsFilePath.isEmpty())
	{
		// no settings file path was found.
		// use the default settings file path when the user decides to save settings.
		settingsFilePath = pathDir.absoluteFilePath(this->m_TileRecSettingsDefaultFileName);
	}
	item->setData(TileRecSettings::COL_PATH, Qt::UserRole + TileRecSettings::DATA_SETTINGS_PATH, settingsFilePath);

	return item;
}

void TileRecSettings::TileRecSearch(void)
{
	bool resizeColumns = false;
	QSettings settings;

	// get a list of all top level items
	// to match with tilerec RECORD folders
	QList<QTreeWidgetItem*> items;
	int itemIndex = 0;
	while (itemIndex < this->ui.m_DeviceTree->topLevelItemCount())
	{
		items.append(this->ui.m_DeviceTree->topLevelItem(itemIndex));
		itemIndex++;
	}

	// get a list of all drives/locations to search
	// for RECORD folders. this search is not recursive,
	// the folder must be in the root of each location
	// or it will not be found
	QFileInfoList locations = QDir::drives(); // this only works on Windows
	QStringList userPaths = settings.value("app/user_paths", QStringList()).toStringList();
	while (userPaths.count() > 0)
	{
		QFileInfo userPathFileInfo(userPaths.takeFirst());
		locations.append(userPathFileInfo);
	}

	// look for the RECORD folder at each location.
	// do this first using a case-sensitive match
	// but allow a close match as well just in case
	// the drive's filesystem is case-insensitve
	while (locations.count() > 0)
	{
		QFileInfo location = locations.takeFirst();
		if (location.exists() && location.isDir())
		{
			QString locationPath = location.absoluteFilePath();
			QFileInfo closeMatch;
			QFileInfo matchingEntry;
			QDir locationDir(locationPath);
			QFileInfoList entries = locationDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
			while (entries.count() > 0)
			{
				QFileInfo entry = entries.takeFirst();
				QString entryFileName = entry.fileName();
				if (entryFileName.compare(this->m_TileRecSettingsFolderName, Qt::CaseSensitive) == 0)
				{
					matchingEntry = entry;
					break;
				}
				else if (entryFileName.compare(this->m_TileRecSettingsFolderName, Qt::CaseInsensitive) == 0)
				{
					closeMatch = entry;
				}
			}
			if (matchingEntry.exists() == false)
			{
				if (closeMatch.exists())
				{
					// try to rename the close match to be exactly the path that is expected
					QString oldPath = closeMatch.absoluteFilePath();
					QDir closeMatchDir = closeMatch.absoluteDir();
					QString newPath = closeMatchDir.absoluteFilePath(this->m_TileRecSettingsFolderName);
					if (closeMatchDir.rename(closeMatch.absoluteFilePath(), newPath))
					{
						// this is now an exact match
						matchingEntry = QFileInfo(newPath);
					}
					else
					{
						// rename failed.
						// just use the close match as-is and hope it works
						matchingEntry = closeMatch;
					}
				}
				else
				{
					// found no matches
					// continue to the next location.
					continue;
				}
			}
			else
			{
				// found an exact match
			}
			if (matchingEntry.exists())
			{
				QString itemPath = matchingEntry.canonicalFilePath();
				QTreeWidgetItem* item = this->TileRecDeviceTreeGetItem(itemPath);
				if (item)
				{
					// item already exists for this path so remove it from the items list
					// that will be used later to remove the missing paths
					items.removeOne(item);
				}
				else
				{
					// the item for this path is not in the device tree.
					// add it now.
					if (this->TileRecDeviceTreeAddItem(itemPath))
					{
						resizeColumns = true;
					}
				}
			}
		}
		else
		{
			// invalid location
		}
	}
	// done searching through all locations

	// any items remaining in the items list
	// must be removed from the tree as they
	// no longer exist
	while (items.count() > 0)
	{
		QTreeWidgetItem* item = items.takeFirst();
		if (this->TileRecDeviceTreeDeleteItem(item))
		{
			resizeColumns = true;
		}
	}

	if (resizeColumns)
	{
		this->ui.m_DeviceTree->resizeColumnToContents(TileRecSettings::COL_PATH);
	}
}

void TileRecSettings::TileRecUserPathsUpdate(void)
{
	QSettings settings;
	QStringList userPaths = settings.value("app/user_paths", QStringList()).toStringList();
	this->ui.m_UserPaths->clear();
	if (userPaths.count() > 0)
	{
		this->ui.m_UserPaths->addItems(userPaths);
		this->ui.m_UserPaths->sortItems();
		this->ui.m_SearchPaths->setVisible(true);
		this->ui.m_BrowseForRecordPath->setVisible(false);
	}
	else
	{
		this->ui.m_SearchPaths->setVisible(false);
		this->ui.m_BrowseForRecordPath->setVisible(true);
	}
}

void TileRecSettings::on_m_BrowseForRecordPath_clicked(void)
{
	QString resultStr;
	QSettings settings;
	
	// use the last path the user selected or the desktop if it does not exist
	QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
	QString lastPath = settings.value("app/browse", desktopPath).toString();
	QFileInfo lastPathInfo(lastPath);
	if (lastPathInfo.exists() == false)
	{
		lastPath = desktopPath;
	}

	QString path = QFileDialog::getExistingDirectory(this, tr("Browse for TileRec"), lastPath);
	if (path.isEmpty() == false)
	{
		QFileInfo pathInfo(path);
		if (pathInfo.exists() && pathInfo.isDir())
		{
			QStringList userPaths = settings.value("app/user_paths", QStringList()).toStringList();
			if (pathInfo.fileName().compare(this->m_TileRecSettingsFolderName, Qt::CaseInsensitive) == 0)
			{
				// the user selected the RECORD folder itself
				QDir pathDir = pathInfo.absoluteDir();
				QString path = pathDir.canonicalPath();
				if (path.isEmpty() == false)
				{
					if (userPaths.contains(path) == false)
					{
						userPaths.append(path);
						settings.setValue("app/user_paths", userPaths);
						settings.setValue("app/browse", path);
					}
				}
				else
				{
					resultStr.append(tr("The path to the parent folder could not be found.\n"));
				}
			}
			else
			{
				// the user selected a folder that either has the RECORD folder
				// or where the drive will eventually be mounted
				QString path = pathInfo.canonicalFilePath();
				if (path.isEmpty() == false)
				{
					if (userPaths.contains(path) == false)
					{
						userPaths.append(path);
						settings.setValue("app/user_paths", userPaths);
						settings.setValue("app/browse", path);
					}
				}
				else
				{
					resultStr.append(tr("The path to the folder could not be found.\n"));
				}
			}
		}
		else
		{
			resultStr.append(tr("The selected folder is missing or is not a directory.\n"));
		}

		if (resultStr.isEmpty())
		{
			this->TileRecUserPathsUpdate();
		}
		else
		{
			QMessageBox::information(this, tr("Browse"), resultStr);
		}
	}
	else
	{
		// user canceled
	}

}

void TileRecSettings::on_m_SaveSelected_clicked(void)
{
	QString resultStr;

	QList<QTreeWidgetItem*> items = this->ui.m_DeviceTree->selectedItems();
	if (items.count() == 0)
	{
		QMessageBox::information(this, tr("Save Selected"), tr("Please select 1 or more TileRec locations to save."));
		return;
	}

	QList<QTreeWidgetItem*> topLevelItems;
	while (items.count() > 0)
	{
		QTreeWidgetItem* item = items.takeFirst();
		while (item->parent() != Q_NULLPTR)
		{
			item = item->parent();
		}
		if (topLevelItems.contains(item) == false)
		{
			topLevelItems.append(item);
		}
	}

	while (topLevelItems.count() > 0)
	{
		QTreeWidgetItem* item = topLevelItems.takeFirst();
		if (item->childCount() >= 3)
		{
			QTreeWidgetItem* timeItem = item->child(0);
			QTreeWidgetItem* recordingModeItem = item->child(1);
			QTreeWidgetItem* ledModeItem = item->child(2);
			QComboBox* recordModeCombo = dynamic_cast<QComboBox*>(this->ui.m_DeviceTree->itemWidget(recordingModeItem, TileRecSettings::COL_VALUE));
			QComboBox* ledModeCombo = dynamic_cast<QComboBox*>(this->ui.m_DeviceTree->itemWidget(ledModeItem, TileRecSettings::COL_VALUE));
			if (recordModeCombo && ledModeCombo)
			{
				int recordModeValue = recordModeCombo->currentIndex();
				int ledModeValue = ledModeCombo->currentIndex();

				QString settingsFilePath = item->data(TileRecSettings::COL_PATH, Qt::UserRole + TileRecSettings::DATA_SETTINGS_PATH).toString();
				QFile settingsFile(settingsFilePath);
				if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
				{
					QStringList lines;
					lines.append(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
					lines.append(QString::number(recordModeValue));
					lines.append(QString::number(ledModeValue));
					QString settingsFileStr = lines.join("\r\n");
					QByteArray settingsFileData = settingsFileStr.toUtf8();
					if (settingsFile.write(settingsFileData) != settingsFileData.length())
					{
						resultStr.append(tr("Cannot write to \"%1\"\n").arg(settingsFilePath));
					}
					else
					{
						resultStr.append(tr("\"%1\" Saved\n").arg(settingsFilePath));
					}
					settingsFile.close();
				}
				else
				{
					resultStr.append(tr("Cannot open or create \"%1\"\n").arg(settingsFilePath));
				}
			}
			else
			{
				resultStr.append(tr("Missing settings data for \"%1\"\n").arg(item->text(TileRecSettings::COL_PATH)));
			}
		}
		else
		{
			resultStr.append(tr("Missing settings for \"%1\"\n").arg(item->text(TileRecSettings::COL_PATH)));
		}
	}

	if (resultStr.isEmpty() == false)
	{
		QMessageBox::information(this, tr("Save Selected"), resultStr);
	}
}

void TileRecSettings::on_m_RemoveRecordPath_clicked(void)
{
	QList<QListWidgetItem*> items = this->ui.m_UserPaths->selectedItems();
	while (items.count() > 0)
	{
		delete items.takeFirst();
	}

	QStringList userPaths;
	int itemIndex = 0;
	while (itemIndex < this->ui.m_UserPaths->count())
	{
		QListWidgetItem* item = this->ui.m_UserPaths->item(itemIndex);
		userPaths.append(item->text());
		itemIndex++;
	}

	QSettings settings;
	settings.setValue("app/user_paths", userPaths);
	this->TileRecUserPathsUpdate();
}
