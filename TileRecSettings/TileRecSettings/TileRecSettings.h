#pragma once

#include <QtWidgets/QDialog>
#include "ui_TileRecSettings.h"

class QTimer;
class QTreeWidgetItem;

class TileRecSettings : public QDialog
{
	Q_OBJECT

public:
	TileRecSettings(QWidget *parent = Q_NULLPTR);

	enum DeviceTreeCols_e
	{
		COL_PATH = 0,
		COL_SETTING,
		COL_VALUE,
		COL_COUNT
	};

	enum DeviceTreeItemData_e
	{
		DATA_SETTINGS_PATH = 0,
		DATA_COUNT
	};

	/** delete the specified item from the device tree
	*/
	bool TileRecDeviceTreeDeleteItem(
		QTreeWidgetItem* item);

	/** get an item from the device tree for the specified path
	*/
	QTreeWidgetItem* TileRecDeviceTreeGetItem(
		const QString& path);

	/** add an item to the device tree for the specified
		path to a RECORD folder where settings may be stored
		\param path the path to add
		\param settingsPath optional location to store the full path to the file used to load settings.
		       if this string is empty it just means that an existing settings file could not be found.
	*/
	QTreeWidgetItem* TileRecDeviceTreeAddItem(
		const QString& path,
		QString* settingsPath = Q_NULLPTR);

public slots:

	/** check all drives for a RECORD folder at the root
	    and populate/depopulate the device tree
	*/
	void TileRecSearch(void);

	/** update the user paths display
	*/
	void TileRecUserPathsUpdate(void);

private slots:

	void on_m_BrowseForRecordPath_clicked(void);
	void on_m_SaveSelected_clicked(void);
	void on_m_RemoveRecordPath_clicked(void);

private:
	Ui::TileRecSettingsClass ui;

	QTimer* m_TileRecSearchTimer;
	QString m_TileRecSettingsFolderName;
	QString m_TileRecSettingsDefaultFileName;
};
