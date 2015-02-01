#include "imageinfo.h"
#include <QCryptographicHash>

/*ImageInfo::ImageInfo()
{
}

ImageInfo::ImageInfo( const QString &file )
{

}*/

void ImageInfo::refresh()
{
	QFileInfo::refresh();
	md5hash.clear();
}

QByteArray ImageInfo::md5()
{
	if (caching() && !md5hash.isEmpty())
		return md5hash;
	QFile file(filePath());
	if (!file.open(QFile::ReadOnly)) {
		md5hash.clear();
		return md5hash;
	}
	QCryptographicHash crypto(QCryptographicHash::Md5);
	while(!file.atEnd()) {
		crypto.addData(file.read(8192));
	}
	return md5hash = crypto.result();
}

QSize ImageInfo::dimensions()
{
	if (imageReader->fileName().isEmpty())
		imageReader->setFileName(filePath());
	return imageReader->size();
}

QByteArray ImageInfo::format()
{
	if (imageReader->fileName().isEmpty())
		imageReader->setFileName(filePath());
	return imageReader->format();
}
