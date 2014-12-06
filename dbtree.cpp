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

#include "dbtree.h"

DBTree::DBTree(QWidget *parent) : QTreeView(parent)
{
	dbModel = new QStandardItemModel;
	setHeaderHidden(true);

	QSqlDatabase db = QSqlDatabase::database("phototonic");
	QSqlQuery query(db);

	query.exec("SELECT id, name FROM option_group ORDER BY name ASC");
	while (query.next()) {
		// qDebug() << query.value(0).toString();
		QStandardItem *item = new QStandardItem(query.value(1).toString());
		QSqlQuery subquery(db);
		subquery.exec("SELECT id, name FROM option WHERE group_id = " + query.value(0).toString() + " ORDER BY name ASC");
		while (subquery.next()) {
		QStandardItem *subitem = new QStandardItem(subquery.value(1).toString());
		subitem->setData(subquery.value(0).toInt());
		item->appendRow(subitem);
		}
		dbModel->appendRow(item);
	}
	setModel(dbModel);
}
