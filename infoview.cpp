/*
 *  Copyright (C) 2013-2014 Ofer Kashayov <oferkv@live.com>
 *  This file is part of Phototonic Image Viewer.
 *
 *  Phototonic is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Phototonic is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Phototonic.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "infoview.h"

InfoView::InfoView(QWidget *parent) : QTableView(parent)
{
	setSelectionBehavior(QAbstractItemView::SelectItems);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	verticalHeader()->setVisible(false);
	verticalHeader()->setDefaultSectionSize(verticalHeader()->minimumSectionSize());
	horizontalHeader()->setVisible(false);
	horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	setShowGrid(false);

	setEditTriggers(QAbstractItemView::DoubleClicked);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setTabKeyNavigation(false);

	infoModel = new QStandardItemModel(this);
	setModel(infoModel);
	connect(infoModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(dataChanged(QModelIndex, QModelIndex)));

	// InfoView menu
	infoMenu = new QMenu("");
	copyAction = new QAction(tr("Copy"), this);
	connect(copyAction, SIGNAL(triggered()), this, SLOT(copyEntry()));
	infoMenu->addAction(copyAction);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showInfoViewMenu(QPoint)));
}

void InfoView::showInfoViewMenu(QPoint pt)
{
    selectedEntry = indexAt(pt);
	if (selectedEntry.isValid())
    	infoMenu->popup(viewport()->mapToGlobal(pt));
}
 
void InfoView::clear()
{
	infoModel->clear();
}

void InfoView::addEntry(QString &title, QString &value, const QString &key)
{
	lockInfoChanged = true;
	int atRow = infoModel->rowCount();
	QStandardItem *itemTitle = new QStandardItem(title);
	itemTitle->setEditable(false);
	infoModel->insertRow(atRow, itemTitle);
	QStandardItem *itemVal = new QStandardItem(value);
	if (!key.isEmpty()) {
		itemVal->setData(key);
		itemVal->setEditable(true);
	} else {
		itemVal->setEditable(false);
	}
	itemVal->setToolTip(value);
	infoModel->setItem(atRow, 1, itemVal);
	lockInfoChanged = false;
}

void InfoView::addTitleEntry(QString title)
{
	lockInfoChanged = true;
	int atRow = infoModel->rowCount();
	QStandardItem *itemTitle = new QStandardItem(title);
	itemTitle->setEditable(false);
	infoModel->insertRow(atRow, itemTitle);
	QStandardItem *itemVal = new QStandardItem();
	itemVal->setEditable(false);
	infoModel->setItem(atRow, 1, itemVal);

	QFont boldFont;
	boldFont.setBold(true);
	itemTitle->setData(boldFont, Qt::FontRole);
	lockInfoChanged = false;
}

void InfoView::copyEntry()
{
	if (selectedEntry.isValid())
		QApplication::clipboard()->setText(infoModel->itemFromIndex(selectedEntry)->toolTip());
}

void InfoView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
	if (!lockInfoChanged) {
		QStandardItem *item = infoModel->item(topLeft.row(), topLeft.column());
		if ((QMetaType::Type)item->data().type() == QMetaType::QString) {
			// qDebug() << item->data().toString() << item->text();
			item->setToolTip(item->text());
			emit infoChanged(item->data().toString(), item->text());
		}
	}
}
