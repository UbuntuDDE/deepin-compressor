/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     gaoxiang <gaoxiang@uniontech.com>
*
* Maintainer: gaoxiang <gaoxiang@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "uitools.h"
#include "mimetypes.h"
#include "kpluginfactory.h"
#include "kpluginloader.h"
#include "pluginmanager.h"

#include <DStandardPaths>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QDebug>
#include <QFileInfo>
#include <QFontInfo>
#include <QMimeType>
#include <QApplication>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QImageReader>
#include <QPixmap>
#include <QTextCodec>
#include <QRegularExpression>
#include <QUuid>
#include <QStorageInfo>

#include <KEncodingProber>

DCORE_USE_NAMESPACE
Q_DECLARE_METATYPE(KPluginMetaData)

QStringList UiTools::m_associtionlist = QStringList() << "file_association.file_association_type.x-7z-compressed"
                                        << "file_association.file_association_type.x-archive"
                                        << "file_association.file_association_type.x-bcpio"
                                        << "file_association.file_association_type.x-bzip"
                                        << "file_association.file_association_type.x-cpio"
                                        << "file_association.file_association_type.x-cpio-compressed"
                                        << "file_association.file_association_type.vnd.debian.binary-package"
                                        << "file_association.file_association_type.gzip"
                                        << "file_association.file_association_type.x-java-archive"
                                        << "file_association.file_association_type.x-lzma"
                                        << "file_association.file_association_type.vnd.ms-cab-compressed"
                                        << "file_association.file_association_type.vnd.rar"
                                        << "file_association.file_association_type.x-rpm"
                                        << "file_association.file_association_type.x-sv4cpio"
                                        << "file_association.file_association_type.x-sv4crc"
                                        << "file_association.file_association_type.x-tar"
                                        << "file_association.file_association_type.x-bzip-compressed-tar"
                                        << "file_association.file_association_type.x-compressed-tar"
                                        << "file_association.file_association_type.x-lzip-compressed-tar"
                                        << "file_association.file_association_type.x-lzma-compressed-tar"
                                        << "file_association.file_association_type.x-tzo"
                                        << "file_association.file_association_type.x-xz-compressed-tar"
                                        << "file_association.file_association_type.x-tarz"
                                        << "file_association.file_association_type.x-xar"
                                        << "file_association.file_association_type.x-xz"
                                        << "file_association.file_association_type.zip"
                                        << "file_association.file_association_type.x-iso9660-image"
                                        << "file_association.file_association_type.x-iso9660-appimage"
                                        << "file_association.file_association_type.x-source-rpm"
                                        << "file_association.file_association_type.x-chrome-extension";

UiTools::UiTools(QObject *parent)
    : QObject(parent)
{
}

UiTools::~UiTools()
{
}

QString UiTools::getConfigPath()
{
    QDir dir(QDir(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first())
             .filePath(qApp->organizationName()));

    return dir.filePath(qApp->applicationName());
}

QPixmap UiTools::renderSVG(const QString &filePath, const QSize &size)
{
    QImageReader reader;
    QPixmap pixmap;

    reader.setFileName(filePath);

    if (reader.canRead()) {
        const qreal ratio = qApp->devicePixelRatio();
        reader.setScaledSize(size * ratio);
        pixmap = QPixmap::fromImage(reader.read());
        pixmap.setDevicePixelRatio(ratio);
    } else {
        pixmap.load(filePath);
    }

    return pixmap;
}

QString UiTools::humanReadableSize(const qint64 &size, int precision)
{
    if (size == 0) {
        return "-";
    }

    double sizeAsDouble = size;
    static QStringList measures;
    if (measures.isEmpty()) {
        measures <<  "B" << "KB" << "MB" << "GB" << "TB" << "PB" << "EB" << "ZB" << "YB";
    }
    QStringListIterator it(measures);
    QString measure(it.next());
    while (sizeAsDouble >= 1024.0 && it.hasNext()) {
        measure = it.next();
        sizeAsDouble /= 1024.0;
    }

    return QString::fromLatin1("%1 %2").arg(sizeAsDouble, 0, 'f', precision).arg(measure);
}

bool UiTools::isArchiveFile(const QString &strFileName)
{
    QString strTransFileName = strFileName;
    UnCompressParameter::SplitType type;
    UiTools::transSplitFileName(strTransFileName, type);

    CustomMimeType mimeType = determineMimeType(strTransFileName);
    QString mime;
    if (mimeType.name().contains("application/"))
        mime = mimeType.name().remove("application/");

    bool ret = false;

    if (mime.size() > 0) {
        bool bArchive = false;
        ret = isExistMimeType(mime, bArchive); // ?????????????????????????????????????????????????????????
    } else {
        ret = false;
    }

    if (strTransFileName.endsWith(".deb")) {    // ???deb???????????????????????????
        ret = false;
    }

    if (strTransFileName.endsWith(".crx") || strTransFileName.endsWith(".apk")) {    // ???crx???apk????????????????????????
        ret = true;
    }

    return ret;
}

QString UiTools::judgeFileMime(const QString &strFileName)
{
    QString type = "";
    if (strFileName.endsWith(".7z") || strFileName.contains(".7z.0")) {
        type = "x-7z-compressed";
    } else if (strFileName.endsWith(".cpio.gz")) {
        type = "x-cpio-compressed";
    } else if (strFileName.endsWith(".tar.bz2")) {
        type = "x-bzip-compressed-tar";
    } else if (strFileName.endsWith(".tar.gz")) {
        type = "x-compressed-tar";
    } else if (strFileName.endsWith(".tar.lz")) {
        type = "x-lzip-compressed-tar";
    } else if (strFileName.endsWith(".tar.lzma")) {
        type = "x-lzma-compressed-tar";
    } else if (strFileName.endsWith(".tar.lzo")) {
        type = "x-tzo";
    } else if (strFileName.endsWith(".tar.xz")) {
        type = "x-xz-compressed-tar";
    } else if (strFileName.endsWith(".tar.Z")) {
        type = "x-tarz";
    } else if (strFileName.endsWith(".src.rpm")) {
        type = "x-source-rpm";
    } else if (strFileName.endsWith(".ar")) {
        type = "x-archive";
    } else if (strFileName.endsWith(".bcpio")) {
        type = "x-bcpio";
    } else if (strFileName.endsWith(".bz2")) {
        type = "x-bzip";
    } else if (strFileName.endsWith(".cpio")) {
        type = "x-cpio";
    } else if (strFileName.endsWith(".deb")) {
        type = "vnd.debian.binary-package";
    } else if (strFileName.endsWith(".gz")) {
        type = "gzip";
    } else if (strFileName.endsWith(".jar")) {
        type = "x-java-archive";
    } else if (strFileName.endsWith(".lzma")) {
        type = "x-lzma";
    } else if (strFileName.endsWith(".cab")) {
        type = "vnd.ms-cab-compressed";
    } else if (strFileName.endsWith(".rar")) {
        type = "vnd.rar";
    } else if (strFileName.endsWith(".rpm")) {
        type = "x-rpm";
    } else if (strFileName.endsWith(".sv4cpio")) {
        type = "x-sv4cpio";
    } else if (strFileName.endsWith(".sc4crc")) {
        type = "x-sc4crc";
    } else if (strFileName.endsWith(".tar")) {
        type = "x-tar";
    } else if (strFileName.endsWith(".xar")) {
        type = "x-xar";
    } else if (strFileName.endsWith(".xz")) {
        type = "x-xz";
    } else if (strFileName.endsWith(".zip")) {
        type = "zip";
    } else if (strFileName.endsWith(".iso")) {
        type = "x-iso9660-image";
    } else if (strFileName.endsWith(".appimage")) {
        type = "x-iso9660-appimage";
    }

    return type;
}

bool UiTools::isExistMimeType(const QString &strMimeType, bool &bArchive)
{
    QString conf = readConf();
    QStringList confList = conf.split("\n", QString::SkipEmptyParts);

    bool exist = false;
    for (int i = 0; i < confList.count(); i++) {
        if (confList.at(i).contains("." + strMimeType + ":")) {
            bArchive = true;
            if (confList.at(i).contains("true")) {
                exist = true;
                break;
            } else {
                exist = false;
                continue;
            }
        }
    }

    return exist;
}

QString UiTools::readConf()
{
    const QString confDir = DStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir;
    if (!dir.exists(confDir + QDir::separator())) {
        dir.mkpath(confDir + QDir::separator());
    }

    const QString confPath = confDir + QDir::separator() + "deepin-compressor.confbf";
    QFile confFile(confPath);

    // default settings
    if (!confFile.exists()) {
        confFile.open(QIODevice::WriteOnly | QIODevice::Text);

        foreach (QString key, m_associtionlist) {
            QString content = key + ":" + "true" + "\n";
            confFile.write(content.toUtf8());
        }

        confFile.close();
    }

    QString confValue;
    bool readStatus = confFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (readStatus) {
        confValue = confFile.readAll();
    }

    confFile.close();
    return confValue;
}

QString UiTools::toShortString(QString strSrc, int limitCounts, int left)
{
    left = (left >= limitCounts || left <= 0) ? limitCounts / 2 : left;
    int right = limitCounts - left;
    QString displayName = "";
    displayName = strSrc.length() > limitCounts ? strSrc.left(left) + "..." + strSrc.right(right) : strSrc;
    return displayName;
}

ReadOnlyArchiveInterface *UiTools::createInterface(const QString &fileName, bool bWrite, AssignPluginType eType/*, bool bUseLibArchive*/)
{
//    QFileInfo fileinfo(fileName); // ??????????????????

    const CustomMimeType mimeType = determineMimeType(fileName);

    QVector<Plugin *> offers;
    if (bWrite) {
        offers = PluginManager::get_instance().preferredWritePluginsFor(mimeType);

//        if (bUseLibArchive == true && mimeType.name() == "application/zip") {
//            std::sort(offers.begin(), offers.end(), [](Plugin * p1, Plugin * p2) {
//                if (p1->metaData().name().contains("Libarchive")) {
//                    return true;
//                }
//                if (p2->metaData().name().contains("Libarchive")) {
//                    return false;
//                }

//                return p1->priority() > p2->priority();
//            });
//        }
    } else {
        offers = PluginManager::get_instance().preferredPluginsFor(mimeType);
    }

    if (offers.isEmpty()) {
        qInfo() << "Could not find a plugin to handle" << fileName;
        return nullptr;
    }

    //tar.lzo?????? ???P7zip????????????mimeFromContent???"application/x-7z-compressed"??????Libarchive????????????mimeFromContent???"application/x-lzop"
    // ??????????????????cli7z?????????????????????zip???tar?????????????????????7z?????????libzip???libarchive??????
    //??????P7zip???????????? mimeFromContent???"application/x-lzop" ?????????
    QMimeDatabase db;
    QMimeType mimeFromContent = db.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);
    bool remove7zFlag = false;

    if (eType != APT_Cli7z) {
        if (((!bWrite) && (mimeType.name() == QString("application/zip") || mimeType.name() == QString("application/x-tar")))
                || ("application/x-tzo" == mimeType.name() && "application/x-lzop" == mimeFromContent.name())) {
            remove7zFlag = true;
        }
    }

    // ????????????
    ReadOnlyArchiveInterface *pIface = nullptr;
    for (Plugin *plugin : offers) {
        //??????P7zip??????
        if (remove7zFlag && plugin->metaData().name().contains("7zip", Qt::CaseInsensitive)) {
            continue;
        }

        switch (eType) {
        // ????????????
        case APT_Auto:
            pIface = createInterface(fileName, mimeType, plugin);
            break;
        // cli7zplugin
        case APT_Cli7z:
            if (plugin->metaData().name().contains("7zip", Qt::CaseInsensitive)) {
                pIface = createInterface(fileName, mimeType, plugin);
            }
            break;
        // libarchive
        case APT_Libarchive:
            if (plugin->metaData().name().contains("libarchive", Qt::CaseInsensitive)) {
                pIface = createInterface(fileName, mimeType, plugin);
            }
            break;
        // libzipplugin
        case APT_Libzip:
            if (plugin->metaData().name().contains("libzip", Qt::CaseInsensitive)) {
                pIface = createInterface(fileName, mimeType, plugin);
            }
            break;
        }

        // Use the first valid plugin, according to the priority sorting.
        if (pIface) {
            qInfo() << "???????????????" << plugin->metaData().pluginId();
            break;
        }
    }

    return pIface;
}

ReadOnlyArchiveInterface *UiTools::createInterface(const QString &fileName, const CustomMimeType &mimeType, Plugin *plugin)
{
    Q_ASSERT(plugin);

    KPluginFactory *factory = KPluginLoader(plugin->metaData().fileName()).factory();
    if (!factory) {
        return nullptr;
    }

    const QVariantList args = {QVariant(QFileInfo(fileName).absoluteFilePath()),
                               QVariant().fromValue(plugin->metaData()),
                               QVariant::fromValue(mimeType)
                              };

    ReadOnlyArchiveInterface *iface = factory->create<ReadOnlyArchiveInterface>(nullptr, args);
    return iface;
}

void UiTools::transSplitFileName(QString &fileName, UnCompressParameter::SplitType &eSplitType)
{
    if (fileName.contains(".7z.")) {
        // 7z????????????
        QRegExp reg("^([\\s\\S]*.)[0-9]{3}$"); // QRegExp reg("[*.]part\\d+.rar$"); //rar???????????????

        if (reg.exactMatch(fileName)) {
            fileName = reg.cap(1) + "001"; //??????: *.7z.003 -> *.7z.001
            eSplitType = UnCompressParameter::ST_Other;
        }
    } else if (fileName.contains(".part") && fileName.endsWith(".rar")) {
        // rar????????????
        int x = fileName.lastIndexOf("part");
        int y = fileName.lastIndexOf(".");

        if ((y - x) > 5) {
            fileName.replace(x, y - x, "part01");
        } else {
            fileName.replace(x, y - x, "part1");
        }

        eSplitType = UnCompressParameter::ST_Other;
    } else if (fileName.contains(".zip.")) { // 1.zip.001??????
        QRegExp reg("^([\\s\\S]*.)[0-9]{3}$");
        if (reg.exactMatch(fileName)) {
            QFileInfo fi(reg.cap(1) + "001");
            if (fi.exists() == true) {
                fileName = reg.cap(1) + "001";
                eSplitType = UnCompressParameter::ST_Zip;
            }
        }
    } else if (fileName.endsWith(".zip")) { //1.zip 1.01??????
        /**
         * ??????123.zip???????????????123.z01??????????????????
         * ????????????????????????123.zip????????????
         */
        QFileInfo tmp(fileName.left(fileName.length() - 2) + "01");
        if (tmp.exists()) {
            eSplitType = UnCompressParameter::ST_Zip;
        }
    } else if (fileName.contains(".z")) {  //1.zip 1.01??????
        /**
         * ??????123.z01???????????????123.zip??????????????????
         * ????????????????????????123.z01????????????
         */
        QRegExp reg("^([\\s\\S]*.)z[0-9]+$");
        if (reg.exactMatch(fileName)) {
            fileName = reg.cap(1) + "zip";
            QFileInfo fi(fileName);
            if (fi.exists()) {
                eSplitType = UnCompressParameter::ST_Zip;
            }
        }
    }

    // ???????????????????????????????????????????????????????????????????????????????????????????????????
    QFileInfo info(fileName);
    if (info.isSymLink()) {
        fileName = info.symLinkTarget();
    }
}

QString UiTools::handleFileName(const QString &strFileName)
{
    QFileInfo info(strFileName);
    QString strName = info.completeBaseName();     // ??????????????????????????????????????????

    if (info.filePath().contains(".tar.")) {
        strName = strName.remove(".tar"); // ??????tar.gz?????????????????????????????????????????????.tar
    } else if (info.filePath().contains(".7z.")) {
        strName = strName.remove(".7z"); // 7z?????????????????????????????????????????????.7z
    } else if (info.filePath().contains(".part01.rar")) {
        strName = strName.remove(".part01"); // rar?????????????????????????????????????????????part01
    } else if (info.filePath().contains(".part1.rar")) {
        strName = strName.remove(".part1"); // rar?????????????????????????????????????????????.part1
    } else if (info.filePath().contains(".zip.")) {
        strName = strName.remove(".zip"); // zip?????????????????????????????????????????????.zip
    }

    return strName;
}

bool UiTools::isLocalDeviceFile(const QString &strFileName)
{
    QStorageInfo info(strFileName);
    return info.device().startsWith("/dev/");
}

QStringList UiTools::removeSameFileName(const QStringList &listFiles)
{
    QStringList listResult;
    QStringList listName;

    for (int i = 0; i < listFiles.count(); ++i) {
        QFileInfo info(listFiles[i]);
        if (listName.contains(info.fileName())) {
            continue;
        } else {
            listResult << listFiles[i];
            listName << info.fileName();
        }
    }

    return listResult;
}
