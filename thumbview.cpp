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

#include "thumbview.h"

#define BAD_IMG_SZ	64

ThumbView::ThumbView(QWidget *parent) : QListView(parent)
{
	GData::thumbsBackgroundColor = GData::appSettings->value("backgroundThumbColor").value<QColor>();
	GData::thumbsTextColor = GData::appSettings->value("textThumbColor").value<QColor>();
	setThumbColors();
	GData::thumbSpacing = GData::appSettings->value("thumbSpacing").toInt();
	GData::thumbPagesReadahead = GData::appSettings->value("thumbPagesReadahead").toInt();
	GData::thumbsLayout = GData::appSettings->value("thumbLayout").toInt();
	thumbSize = GData::appSettings->value("thumbsZoomVal").toInt();
	currentRow = 0;

	setViewMode(QListView::IconMode);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setResizeMode(QListView::Adjust);
	setWordWrap(true);
	setDragEnabled(true);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setItemDelegate(new QItemDelegate);
	setUniformItemSizes(false);

	thumbViewModel = new QStandardItemModel(this);
	thumbViewModel->setSortRole(SortRole);
	setModel(thumbViewModel);

	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(loadVisibleThumbs(int)));
	connect(this->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
				this, SLOT(handleSelectionChanged(QItemSelection)));
   	connect(this, SIGNAL(doubleClicked(const QModelIndex &)), 
				parent, SLOT(loadImagefromThumb(const QModelIndex &)));
	connect(this->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
				parent, SLOT(updateViewerImageBySelection(QItemSelection)));

	thumbsDir = new QDir();
	fileFilters = new QStringList;

	emptyImg.load(":/images/no_image.png");

	QTime time = QTime::currentTime();
	qsrand((uint)time.msec());
	mainWindow = parent;
	infoView = new InfoView(this);
	connect(infoView, SIGNAL(infoChanged(QString,QString)), this, SLOT(updateUserFields(QString,QString)));
}

void ThumbView::setThumbColors()
{
	QString bgColor = "background: rgb(%1, %2, %3); ";
	bgColor = bgColor.arg(GData::thumbsBackgroundColor.red())
						.arg(GData::thumbsBackgroundColor.green())
						.arg(GData::thumbsBackgroundColor.blue());

	QString ss = "QListView { " + bgColor + "background-image: url("
					+ GData::thumbsBackImage
					+ "); background-attachment: fixed; }";
   	setStyleSheet(ss);

	QPalette scrollBarOrigPal = verticalScrollBar()->palette();
	QPalette thumbViewOrigPal = palette();
	thumbViewOrigPal.setColor(QPalette::Text, GData::thumbsTextColor);
	setPalette(thumbViewOrigPal);
	verticalScrollBar()->setPalette(scrollBarOrigPal);
}

void ThumbView::selectCurrentIndex()
{
	if (currentIndex.isValid() && thumbViewModel->rowCount() > 0)
	{
		scrollTo(currentIndex);
		setCurrentIndex(currentIndex);
	}
}

QString ThumbView::getSingleSelectionFilename()
{
	if (selectionModel()->selectedIndexes().size() == 1)
		return thumbViewModel->item(selectionModel()->selectedIndexes().first().row())->data(FileNameRole).toString();

	return("");
}

int ThumbView::getNextRow()
{
	if (currentRow == thumbViewModel->rowCount() - 1)
		return -1;

	return currentRow + 1;
}

int ThumbView::getPrevRow()
{
	if (currentRow == 0)
		return -1;

	return currentRow - 1;
}

int ThumbView::getLastRow()
{
	return thumbViewModel->rowCount() - 1;
}

int ThumbView::getRandomRow()
{
	return qrand() % (thumbViewModel->rowCount());
}

int ThumbView::getCurrentRow()
{
	return currentRow;
}

void ThumbView::setCurrentRow(int row)
{
	if (row >= 0)
		currentRow = row;
	else
		currentRow = 0;
}

void ThumbView::setImageviewWindowTitle()
{
	QString title = thumbViewModel->item(currentRow)->data(FileNameRole).toString()
			+ " - ["
			+ QString::number(currentRow + 1)
			+ "/"
			+ QString::number(thumbViewModel->rowCount())
			+ "] - Phototonic";


	mainWindow->setWindowTitle(title);
}

bool ThumbView::setCurrentIndexByName(QString &FileName)
{
	QModelIndexList indexList = thumbViewModel->match(thumbViewModel->index(0, 0), FileNameRole, FileName);
	if (indexList.size())
	{
	 	currentIndex = indexList[0];
	 	setCurrentRow(currentIndex.row());
		setRowHidden(currentIndex.row(), false);
	 	return true;
 	}

 	return false;
}

bool ThumbView::setCurrentIndexByRow(int row)
{
	QModelIndex idx = thumbViewModel->indexFromItem(thumbViewModel->item(row));
	if (idx.isValid())
	{
		currentIndex = idx;
	 	setCurrentRow(idx.row());
	 	return true;
 	}

 	return false;
}

void ThumbView::updateExifInfo(QString imageFullPath)
{
	Exiv2::Image::AutoPtr exifImage;
	QString key;
	QString val;

	try	{
		exifImage = Exiv2::ImageFactory::open(imageFullPath.toStdString());
		exifImage->readMetadata();
	}
	catch (Exiv2::Error &error) {
		return;
	}

	Exiv2::ExifData &exifData = exifImage->exifData();
	if (!exifData.empty()) {
		Exiv2::ExifData::const_iterator end = exifData.end();
		infoView->addTitleEntry("Exif");
		for (Exiv2::ExifData::const_iterator md = exifData.begin(); md != end; ++md) {
			// qDebug() << Exiv2::toString(md->key()).c_str() << " " << Exiv2::toString(md->value()).c_str();
			key = QString::fromUtf8(md->tagName().c_str());
			val = QString::fromUtf8(md->print().c_str());
			infoView->addEntry(key, val);
		}
	}

	Exiv2::IptcData &iptcData = exifImage->iptcData();
	if (!iptcData.empty()) {
		Exiv2::IptcData::iterator end = iptcData.end();
		infoView->addTitleEntry("IPTC");
		for (Exiv2::IptcData::iterator md = iptcData.begin(); md != end; ++md) {
			key = QString::fromUtf8(md->tagName().c_str());
			val = QString::fromUtf8(md->print().c_str());
	   		infoView->addEntry(key, val);
		}
	}

	Exiv2::XmpData &xmpData = exifImage->xmpData();
	if (!xmpData.empty()) {
		Exiv2::XmpData::iterator end = xmpData.end();
		infoView->addTitleEntry("XMP");
		for (Exiv2::XmpData::iterator md = xmpData.begin(); md != end; ++md) {
			key = QString::fromUtf8(md->tagName().c_str());
			val = QString::fromUtf8(md->print().c_str());
			infoView->addEntry(key, val);
		}
	}
}

void ThumbView::handleSelectionChanged(const QItemSelection&)
{
	QString info;
	QModelIndexList indexesList = selectionModel()->selectedIndexes();
	int nSelected = indexesList.size();
	QString imageFullPath;

	infoView->clear();
	currentId = 0;

	if (nSelected == 1) {
		QString imageFullPath = thumbViewModel->item(indexesList.first().row())->data(FileNameRole).toString();
		imageInfoReader.setFileName(imageFullPath);
		QString key;
		QString val;

		QFileInfo imageInfo = QFileInfo(imageFullPath);
		infoView->addTitleEntry(tr("General"));

		key = tr("File name");
		val = imageInfo.fileName();
		infoView->addEntry(key, val);

		key = tr("Location");
		val = imageInfo.path();
		infoView->addEntry(key, val);

		key = tr("Size");
		val = QString::number(imageInfo.size() / 1024.0, 'f', 2) + "K";
		infoView->addEntry(key, val);

		key = tr("Modified");
		val = imageInfo.lastModified().toString(Qt::SystemLocaleShortDate);
		infoView->addEntry(key, val);

		if (imageInfoReader.size().isValid()) {
			key = tr("Format");
			val = imageInfoReader.format().toUpper();
			infoView->addEntry(key, val);

			key = tr("Resolution");
			val = QString::number(imageInfoReader.size().width())
					+ "x"
					+ QString::number(imageInfoReader.size().height());
			infoView->addEntry(key, val);

			key = tr("Megapixel");
			val = QString::number((imageInfoReader.size().width() * imageInfoReader.size().height()) / 1000000.0, 'f', 2);
			infoView->addEntry(key, val);

			// calculate md5
			QCryptographicHash crypto(QCryptographicHash::Md5);
			QFile file(imageFullPath);
			file.open(QFile::ReadOnly);
			while(!file.atEnd()){
				crypto.addData(file.read(8192));
			}
			QByteArray hash = crypto.result();
			key = tr("MD5 sum");
			val = hash.toHex();
			infoView->addEntry(key, val);

			// add EXIF info
			updateExifInfo(imageFullPath);
			recentThumb = imageFullPath;

			// update database
			infoView->addTitleEntry(tr("User"));
			QSqlDatabase db = QSqlDatabase::database("phototonic");
			QSqlQuery query(db);
			query.exec((QString)"SELECT id, label, description, rating, tags FROM image WHERE file = '" + imageFullPath + "'");
			if (query.next()) {
				currentId = query.value(0).toInt();
				key = tr("Label");
				val = query.value(1).toString();
				infoView->addEntry(key, val, "label");
				key = tr("Description");
				val = query.value(2).toString();
				infoView->addEntry(key, val, "description");
				key = tr("Rating");
				val = query.value(3).toString();
				infoView->addEntry(key, val, "rating");
				key = tr("Tags");
				val = query.value(4).toString();
				infoView->addEntry(key, val, "tags");
			} else {
				key = tr("Label");
				val = imageInfo.fileName();
				infoView->addEntry(key, val, "label");
				val = "";
				key = tr("Description");
				infoView->addEntry(key, val, "description");
				val = "";
				key = tr("Rating");
				infoView->addEntry(key, val, "rating");
				val = "";
				key = tr("Tags");
				infoView->addEntry(key, val, "tags");
			}
			if (!currentId) {
				val = (QString)"INSERT INTO image (file, label, date, md5sum, width, height) VALUES (" +
					"'" + imageFullPath + "', " +
					"'" + imageInfo.fileName() + "', " +
					"'"+ imageInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss") +"', " +
					"'" + hash.toHex() + "', " +
					QString::number(imageInfoReader.size().width()) + ", " +
					QString::number(imageInfoReader.size().height()) + ")";
				if (query.exec(val))
				{
					currentId = query.lastInsertId().toInt();
				}
			} else {
				val = (QString)"UPDATE image SET " +
					"date = '" + imageInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss") + "', " +
					"md5sum = '" + hash.toHex() + "', " +
					"width = " + QString::number(imageInfoReader.size().width()) + ", " +
					"height = " + QString::number(imageInfoReader.size().height()) +
					" WHERE id = " + QString::number(currentId);
				query.exec(val);
			}
			qDebug() << val;
		} else {
			imageInfoReader.read();
			key = tr("Error");
			val = imageInfoReader.errorString();
			infoView->addEntry(key, val);
		}
	}
	updateThumbsSelection();
}

void ThumbView::updateUserFields(QString key,QString value)
{
	QSqlDatabase db = QSqlDatabase::database("phototonic");
	QSqlQuery query(db);
	QString val = (QString)"UPDATE image SET " +
		key + " = '" + value + "'" +
		" WHERE id = " + QString::number(currentId);
	qDebug() << val;
	query.exec(val);
}

void ThumbView::startDrag(Qt::DropActions)
{
	QModelIndexList indexesList = selectionModel()->selectedIndexes();
	if (indexesList.isEmpty()) {
		return;
	}

	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData;
	drag->setMimeData(mimeData);
	drag->start(Qt::CopyAction | Qt::MoveAction);
}

void ThumbView::abort()
{
	abortOp = true;
}

void ThumbView::loadVisibleThumbs(int scrollBarValue)
{
	static int lastScrollBarValue = 0;

	if (GData::thumbsLayout == Compact) {
		scrolledForward = true;
	} else {
		scrolledForward = (scrollBarValue >= lastScrollBarValue);
	}

	lastScrollBarValue = scrollBarValue;

Start:
	int first = getFirstVisibleThumb();
	int last = getLastVisibleThumb();
	if (abortOp || first < 0 || last < 0) 
		return;

	if (scrolledForward)
	{
		last += ((last - first) * (GData::thumbPagesReadahead + 1));
		if (last >= thumbViewModel->rowCount())
			last = thumbViewModel->rowCount() - 1;
	}
	else
	{
		first -= (last - first) * (GData::thumbPagesReadahead + 1);
		if (first < 0)
			first = 0;

		last += 10;
		if (last >= thumbViewModel->rowCount())
			last = thumbViewModel->rowCount() - 1;
	}

	if (thumbsRangeFirst == first && thumbsRangeLast == last) {
		return;
	}

	thumbsRangeFirst = first;
	thumbsRangeLast = last;

	loadThumbsRange();
	if (!abortOp)
		goto Start;
}

int ThumbView::getFirstVisibleThumb()
{
	QModelIndex idx;

	for (int currThumb = 0; currThumb < thumbViewModel->rowCount(); ++currThumb)
	{
		idx = thumbViewModel->indexFromItem(thumbViewModel->item(currThumb));
		if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1)))
		{
			return idx.row();
		}
	}

	return -1;
}

int ThumbView::getLastVisibleThumb()
{
	QModelIndex idx;

	for (int currThumb = thumbViewModel->rowCount() -1; currThumb >= 0 ; --currThumb)
	{
		idx = thumbViewModel->indexFromItem(thumbViewModel->item(currThumb));
		if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1)))
		{
			return idx.row();
		}
	}

	return -1;
}

bool ThumbView::isThumbVisible(QModelIndex idx)
{
	if (viewport()->rect().contains(QPoint(0, visualRect(idx).y() + visualRect(idx).height() + 1)))
	{
		return true;
	}

	return false;
}

void ThumbView::updateThumbsCount()
{
	QString state ;
	
	if (thumbViewModel->rowCount() > 0) {
		state = (QString::number(thumbViewModel->rowCount()) + tr(" images"));
	} else {
		state = QString::number(thumbViewModel->rowCount()) + tr(" images");
	}

	emit setStatus(state);
}

void ThumbView::updateThumbsSelection()
{
	QString state;
	int nSelected = selectionModel()->selectedIndexes().size();

	if (!nSelected)
		state = QString::number(thumbViewModel->rowCount()) + tr(" images");
	else if (nSelected >= 1)
		state = QString(tr("Selected ") + QString::number(nSelected) + tr(" of ")
							+ QString::number(thumbViewModel->rowCount()) + tr(" images"));
	emit setStatus(state);
}

void ThumbView::loadPrepare()
{
	float thumbAspect = 1.33;
	if (GData::thumbsLayout == Compact)
		thumbAspect = 1.77;
	else if (GData::thumbsLayout == Squares)
		thumbAspect = 2;

	thumbHeight = (GData::thumbsLayout == Squares)? thumbSize * thumbAspect : thumbSize;
	thumbWidth = (GData::thumbsLayout == Squares)? thumbSize * thumbAspect : thumbHeight * thumbAspect;
	setIconSize(QSize(thumbWidth, thumbHeight));

	fileFilters->clear();
	QString textFilter("*");
	textFilter+= filterStr;
	*fileFilters	<< textFilter + "*.BMP"
					<< textFilter + "*.GIF"
					<< textFilter + "*.ICO"
					<< textFilter + "*.JPEG"
					<< textFilter + "*.JPG"
					<< textFilter + "*.MNG"
					<< textFilter + "*.PBM"
					<< textFilter + "*.PGM"
					<< textFilter + "*.PNG"
					<< textFilter + "*.PPM"
					<< textFilter + "*.SVG"
					<< textFilter + "*.SVGZ"
					<< textFilter + "*.TGA"					
					<< textFilter + "*.TIF"
					<< textFilter + "*.TIFF"
					<< textFilter + "*.WBMP"
					<< textFilter + "*.XBM"
					<< textFilter + "*.XPM"
					<< textFilter + "*.JPE";
	thumbsDir->setNameFilters(*fileFilters);
	thumbsDir->setFilter(QDir::Files);
	if (GData::showHiddenFiles) {
		thumbsDir->setFilter(thumbsDir->filter() | QDir::Hidden);
	}
	
	thumbsDir->setPath(currentViewDir);

	QDir::SortFlags tempThumbsSortFlags = thumbsSortFlags;
	if (tempThumbsSortFlags & QDir::Size || tempThumbsSortFlags & QDir::Time) {
		tempThumbsSortFlags ^= QDir::Reversed;
	}
	thumbsDir->setSorting(tempThumbsSortFlags);

	thumbViewModel->clear();
	setSpacing(GData::thumbSpacing);

	if (isNeedScroll) {
		scrollToTop();
	}

	abortOp = false;
	newIndex = 0;

	thumbsRangeFirst = -1;
	thumbsRangeLast = -1;
}

void ThumbView::load()
{
	loadPrepare();
	initThumbs();
	updateThumbsCount();
	loadVisibleThumbs();

	if (GData::includeSubFolders) {
		emit showBusy(true);
		QDirIterator iterator(currentViewDir, QDirIterator::Subdirectories);
		while (iterator.hasNext()) {
			iterator.next();
			if (iterator.fileInfo().isDir() && iterator.fileName() != "." && iterator.fileName() != "..") {
				thumbsDir->setPath(iterator.filePath());

				initThumbs();
				updateThumbsCount();
				loadVisibleThumbs();

				if (abortOp) {
					goto finish;
				}
			}
			QApplication::processEvents();
		}
		updateThumbsSelection();
	}

finish:
	busy = false;
	emit showBusy(false);
	return;
}

void ThumbView::loadDuplicates()
{
	loadPrepare();

	emit showBusy(true);
	emit setStatus(tr("Searching duplicate images..."));

	dupImageHashes.clear();
	findDupes(true);

	if (GData::includeSubFolders) {
		QDirIterator iterator(currentViewDir, QDirIterator::Subdirectories);
		while (iterator.hasNext()) {
			iterator.next();
			if (iterator.fileInfo().isDir() && iterator.fileName() != "." && iterator.fileName() != "..") {
				thumbsDir->setPath(iterator.filePath());

				findDupes(false);
				if (abortOp) {
					goto finish;
				}
			}
			QApplication::processEvents();
		}
	}

finish:
	busy = false;
	emit showBusy(false);
	return;
}

void ThumbView::initThumbs()
{
	thumbFileInfoList = thumbsDir->entryInfoList();
	static QStandardItem *thumbIitem;
	static int currThumb;
	static QPixmap emptyPixMap;
	static QSize hintSize;

	emptyPixMap = QPixmap::fromImage(emptyImg).scaled(thumbWidth, thumbHeight);

	if (GData::thumbsLayout == Squares)
		hintSize = QSize(thumbWidth / 2, thumbWidth / 2);
	else if (GData::thumbsLayout == Classic)
		hintSize = QSize(thumbWidth, thumbHeight + 
							(GData::showLabels? QFontMetrics(font()).height() + 5 : 0));

	for (currThumb = 0; currThumb < thumbFileInfoList.size(); ++currThumb)
	{
		thumbFileInfo = thumbFileInfoList.at(currThumb);
		// qDebug() << thumbFileInfo.fileName();
		thumbIitem = new QStandardItem();
		thumbIitem->setData(false, LoadedRole);
		thumbIitem->setData(currThumb, SortRole);
		thumbIitem->setData(thumbFileInfo.filePath(), FileNameRole);
		if (GData::thumbsLayout != Squares && GData::showLabels)
			thumbIitem->setData(thumbFileInfo.fileName(), Qt::DisplayRole);
		if (GData::thumbsLayout == Compact)
			thumbIitem->setIcon(emptyPixMap);
		thumbIitem->setTextAlignment(Qt::AlignTop | Qt::AlignHCenter);
		if (GData::thumbsLayout != Compact)
			thumbIitem->setSizeHint(hintSize);

		thumbViewModel->appendRow(thumbIitem);
	}
}

void ThumbView::updateFoundDupesState(int duplicates, int filesScanned, int originalImages)
{
	QString state;
	state = tr("Scanned ") + tr("%n images, displaying ", "", filesScanned)
				+  tr("%n images (", "", originalImages + duplicates) 
				+  tr("%n originals and ", "", originalImages)
				+  tr("%n duplicates)", "", duplicates);
	emit setStatus(state);
}

void ThumbView::findDupes(bool resetCounters)
{
	thumbFileInfoList = thumbsDir->entryInfoList();
	static int originalImages;
	static int foundDups;
	static int totalFiles;
	if (resetCounters) {
		originalImages = totalFiles = foundDups = 0;
	}

	for (int currThumb = 0; currThumb < thumbFileInfoList.size(); ++currThumb) {
		thumbFileInfo = thumbFileInfoList.at(currThumb);
	    QCryptographicHash md5gen(QCryptographicHash::Md5);
	    QString currentFilePath = thumbFileInfo.filePath();

	    QFile file(currentFilePath);
		if (!file.open(QIODevice::ReadOnly)) {
		    continue;
		}
		totalFiles++;

	    md5gen.addData(file.readAll());
	    file.close();
		QString md5 = md5gen.result().toHex();

		if (dupImageHashes.contains(md5)) {
			if (dupImageHashes[md5].duplicates < 1) {
				addThumb(dupImageHashes[md5].filePath);
				originalImages++;
			}

			foundDups++;
			dupImageHashes[md5].duplicates++;
			addThumb(currentFilePath);
		} else {
			DuplicateImage dupImage;
			dupImage.filePath = currentFilePath; 
			dupImage.duplicates = 0; 
			dupImageHashes.insert(md5, dupImage);
		}

		
		QApplication::processEvents();
		updateFoundDupesState(foundDups, totalFiles, originalImages);
		
		if (abortOp) {
			break;
		}
	}
	updateFoundDupesState(foundDups, totalFiles, originalImages);
}

void ThumbView::loadThumbsRange()
{
	static bool inProgress = false;
	static QImageReader thumbReader;
	static QSize currThumbSize;
	static int currRowCount;
	static QString imageFileName;
	static QImage thumb;
	int currThumb;

	if (inProgress) {	
		abortOp = true;
		QTimer::singleShot(0, this, SLOT(loadThumbsRange()));
		return;
	}

	inProgress = true;
	currRowCount = thumbViewModel->rowCount();

	for (	scrolledForward? currThumb = thumbsRangeFirst : currThumb = thumbsRangeLast;
			(scrolledForward? currThumb <= thumbsRangeLast : currThumb >= thumbsRangeFirst);
			scrolledForward? ++currThumb : --currThumb) {
		if (abortOp || thumbViewModel->rowCount() != currRowCount)
			break;

		if (thumbViewModel->item(currThumb)->data(LoadedRole).toBool())
			continue;

		imageFileName = thumbViewModel->item(currThumb)->data(FileNameRole).toString();
		thumbReader.setFileName(imageFileName);
		currThumbSize = thumbReader.size();

		if (currThumbSize.isValid()) {
			if (!GData::noEnlargeSmallThumb
				|| (currThumbSize.width() > thumbWidth || currThumbSize.height() > thumbHeight)) {
				currThumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
			}

			thumbReader.setScaledSize(currThumbSize);
			thumb = thumbReader.read();

			if (GData::exifThumbRotationEnabled) {
				ImageView::rotateByExifRotation(thumb, imageFileName);
				currThumbSize = thumb.size();
				currThumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
			}
			thumbViewModel->item(currThumb)->setIcon(QPixmap::fromImage(thumb));
		} else {
			thumbViewModel->item(currThumb)->setIcon(QIcon::fromTheme("image-missing",
									QIcon(":/images/error_image.png")).pixmap(BAD_IMG_SZ, BAD_IMG_SZ));
			currThumbSize.setHeight(BAD_IMG_SZ);
			currThumbSize.setWidth(BAD_IMG_SZ);
		}

		if (GData::thumbsLayout == Compact) {
			if (GData::showLabels)
				currThumbSize.setHeight(currThumbSize.height() + QFontMetrics(font()).height() + 5);
			thumbViewModel->item(currThumb)->setSizeHint(currThumbSize);
			if (isThumbVisible(thumbViewModel->item(currThumb)->index()))
				setRowHidden(currThumb, false);
		}

		thumbViewModel->item(currThumb)->setData(true, LoadedRole);

		QApplication::processEvents();
	}

	if (GData::thumbsLayout == Compact && thumbViewModel->rowCount() > 0) {
		setRowHidden(0 , false);
	}

	inProgress = false;
	abortOp = false;
}

void ThumbView::addThumb(QString &imageFullPath)
{
	QStandardItem *thumbIitem = new QStandardItem();
	QImageReader thumbReader;
	QSize hintSize;
	QSize currThumbSize;
	static QImage thumb;

	if (GData::thumbsLayout == Squares)
		hintSize = QSize(thumbWidth / 2, thumbWidth / 2);
	else if (GData::thumbsLayout == Classic)
		hintSize = QSize(thumbWidth, thumbHeight + 
							(GData::showLabels? QFontMetrics(font()).height() + 5 : 0));
	
	thumbFileInfo = QFileInfo(imageFullPath);
	thumbIitem->setData(true, LoadedRole);
	thumbIitem->setData(0, SortRole);
	thumbIitem->setData(thumbFileInfo.filePath(), FileNameRole);
	if (GData::thumbsLayout != Squares && GData::showLabels)
		thumbIitem->setData(thumbFileInfo.fileName(), Qt::DisplayRole);

	thumbReader.setFileName(imageFullPath);
	currThumbSize = thumbReader.size();
	if (currThumbSize.isValid())
	{
		if (!GData::noEnlargeSmallThumb
			|| (currThumbSize.width() > thumbWidth || currThumbSize.height() > thumbHeight))
		{
			currThumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
		}
			
		thumbReader.setScaledSize(currThumbSize);
		thumb = thumbReader.read();

		if (GData::exifThumbRotationEnabled)
		{
			ImageView::rotateByExifRotation(thumb, imageFullPath);
			currThumbSize = thumb.size();
			currThumbSize.scale(QSize(thumbWidth, thumbHeight), Qt::KeepAspectRatio);
		}
			
		thumbIitem->setIcon(QPixmap::fromImage(thumb));
	}
	else
	{
		thumbIitem->setIcon(QIcon::fromTheme("image-missing",
								QIcon(":/images/error_image.png")).pixmap(BAD_IMG_SZ, BAD_IMG_SZ));
		currThumbSize.setHeight(BAD_IMG_SZ);
		currThumbSize.setWidth(BAD_IMG_SZ);
	}

	if (GData::thumbsLayout == Compact)
	{
		if (GData::showLabels)
			currThumbSize.setHeight(currThumbSize.height() + QFontMetrics(font()).height() + 5);
		thumbIitem->setSizeHint(currThumbSize);
	}
	else
		thumbIitem->setSizeHint(hintSize);

	thumbViewModel->appendRow(thumbIitem);
}

void ThumbView::wheelEvent(QWheelEvent *event)
{
	if (event->delta() < 0)
		verticalScrollBar()->setValue(verticalScrollBar()->value() + thumbHeight);
	else
		verticalScrollBar()->setValue(verticalScrollBar()->value() - thumbHeight);
}

void ThumbView::mousePressEvent(QMouseEvent *event)
{
	QListView::mousePressEvent(event);

	if (GData::reverseMouseBehavior && event->button() == Qt::MiddleButton)
	{
		if (selectionModel()->selectedIndexes().size() == 1)
			emit(doubleClicked(selectionModel()->selectedIndexes().first()));
	}
}

void ThumbView::invertSelection()
{
	QItemSelection toggleSelection;
	QModelIndex firstIndex = thumbViewModel->index(0, 0);
	QModelIndex lastIndex = thumbViewModel->index(thumbViewModel->rowCount() - 1, 0);
	toggleSelection.select(firstIndex, lastIndex);
	selectionModel()->select(toggleSelection, QItemSelectionModel::Toggle);
}

