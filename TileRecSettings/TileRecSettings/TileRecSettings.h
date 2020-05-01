#pragma once

#include <QtWidgets/QDialog>
#include "ui_TileRecSettings.h"

class TileRecSettings : public QDialog
{
	Q_OBJECT

public:
	TileRecSettings(QWidget *parent = Q_NULLPTR);

private:
	Ui::TileRecSettingsClass ui;
};
