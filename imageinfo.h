#ifndef IMAGEINFO_H
#define IMAGEINFO_H

#include <QFileInfo>
#include <QImageReader>

class ImageInfo : public QFileInfo
{
public:
	ImageInfo() : QFileInfo() { imageReader = new QImageReader(); };
	ImageInfo( const QString &file ) : QFileInfo( file ) { imageReader = new QImageReader(); };
	void refresh();
	QByteArray md5();
	QSize dimensions();
	QByteArray format();
private:
	QByteArray md5hash;
	QImageReader* imageReader;
};

#endif // IMAGEINFO_H
