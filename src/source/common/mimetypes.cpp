/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     dongsen <dongsen@deepin.com>
 *
 * Maintainer: dongsen <dongsen@deepin.com>
 *             AaronZhang <ya.zhang@archermind.com>
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
#include "mimetypes.h"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QDebug>
#include <QProcess>

CustomMimeType determineMimeType(const QString &filename)
{
    QMimeDatabase db;
    CustomMimeType stMimeType;

    QFileInfo fileinfo(filename);
    QString inputFile = filename;

    // #328815: since detection-by-content does not work for compressed tar archives (see below why)
    // we cannot rely on it when the archive extension is wrong; we need to validate by hand.
    if (fileinfo.completeSuffix().toLower().remove(QRegularExpression(QStringLiteral("[^a-z\\.]"))).contains(QStringLiteral("tar."))) {
        inputFile.chop(fileinfo.completeSuffix().length());
        QString cleanExtension(fileinfo.completeSuffix().toLower());

        // tar.bz2 and tar.lz4 need special treatment since they contain numbers.
        bool isBZ2 = false;
        bool isLZ4 = false;
        bool is7Z = false;
        if (fileinfo.completeSuffix().toLower().contains(QStringLiteral("bz2"))) {
            cleanExtension.remove(QStringLiteral("bz2"));
            isBZ2 = true;
        }

        if (fileinfo.completeSuffix().toLower().contains(QStringLiteral("lz4"))) {
            cleanExtension.remove(QStringLiteral("lz4"));
            isLZ4 = true;
        }

        if (fileinfo.completeSuffix().toLower().contains(QStringLiteral("7z"))) {
            cleanExtension.remove(QStringLiteral("7z"));
            is7Z = true;
        }

        // We remove non-alpha chars from the filename extension, but not periods.
        // If the filename is e.g. "foo.tar.gz.1", we get the "foo.tar.gz." string,
        // so we need to manually drop the last period character from it.
        cleanExtension.remove(QRegularExpression(QStringLiteral("[^a-z\\.]")));
        if (cleanExtension.endsWith(QLatin1Char('.'))) {
            cleanExtension.chop(1);
        }

        // Re-add extension for tar.bz2 and tar.lz4.
        if (isBZ2) {
            cleanExtension.append(QStringLiteral(".bz2"));
        }

        if (isLZ4) {
            cleanExtension.append(QStringLiteral(".lz4"));
        }

        if (is7Z) {
            cleanExtension.append(QStringLiteral(".7z"));
        }

        inputFile += cleanExtension;
    }

//    QMimeType mimeFromDefault = db.mimeTypeForFile(inputFile, QMimeDatabase::MatchDefault);
    QMimeType mimeFromExtension = db.mimeTypeForFile(inputFile, QMimeDatabase::MatchExtension);
    QMimeType mimeFromContent = db.mimeTypeForFile(filename, QMimeDatabase::MatchContent);


//    qInfo() << "mimeFromDefault******************" << mimeFromDefault.name() << mimeFromDefault.parentMimeTypes();
//    qInfo() << "mimeFromExtension******************" << mimeFromExtension.name() << mimeFromExtension.parentMimeTypes();
//    qInfo() << "mimeFromContent****************" << mimeFromContent.name() << mimeFromContent.parentMimeTypes();

    // mimeFromContent will be "application/octet-stream" when file is
    // unreadable, so use extension.
    if (!fileinfo.isReadable()) {
        stMimeType.m_mimeType = mimeFromExtension;
        return stMimeType;
    }

    // Compressed tar-archives are detected as single compressed files when
    // detecting by content. The following code fixes detection of tar.gz, tar.bz2, tar.xz,
    // tar.lzo, tar.lz, tar.lrz and tar.zst.
    if ((mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-compressed-tar"))
            && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/gzip")))
            || (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-bzip-compressed-tar"))
                && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-bzip")))
            || (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-xz-compressed-tar"))
                && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-xz")))
            || (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-tarz"))
                && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-compress")))
            || (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-tzo"))
                && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lzop")))
            || (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-lzip-compressed-tar"))
                && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lzip")))
            || (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-lrzip-compressed-tar"))
                && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lrzip")))
            || (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-lz4-compressed-tar"))
                && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/x-lz4")))
            || (mimeFromExtension == db.mimeTypeForName(QStringLiteral("application/x-zstd-compressed-tar"))
                && mimeFromContent == db.mimeTypeForName(QStringLiteral("application/zstd")))) {
        stMimeType.m_mimeType = mimeFromExtension;
        return stMimeType;
    }

    /* ???????????????????????????"application/octet-stream"?????????"file --mime-type"?????????????????????????????????zip????????????????????????
    *  zip??????????????????????????????"application/octet-stream"??????????????????"application/zip"???file???????????????"application/zip"
    *  ????????????zip??????????????????"application/octet-stream"??????????????????"application/zip"???file???????????????"application/x-chrome-extension"
    *  ????????????crx??????????????????"application/octet-stream"??????????????????"application/octet-stream"???file???????????????"application/x-chrome-extension"
    *  zip???????????????????????????"application/octet-stream"??????????????????"application/zip"???file???????????????"application/octet-stream"
    *  iso??????????????????"application/octet-stream"??????????????????"application/x-cd-image"???file???????????????"application/x-iso9660-image"
    */
    if (mimeFromContent.isDefault()) {

        QProcess process;
        QStringList args;
        args << "--mime-type" << filename;
        process.setProgram("file");
        process.setArguments(args);
        process.start();
        process.waitForFinished();
        const QString output = QString::fromUtf8(process.readAllStandardOutput());

        stMimeType.m_bUnKnown = true;
        if (output.contains("application/octet-stream")) {
            stMimeType.m_strTypeName = mimeFromExtension.name();
            return stMimeType;
        } else if (output.contains("application/x-chrome-extension")) {
            stMimeType.m_strTypeName = "application/x-chrome-extension";
            return stMimeType;
        } else if (output.contains("application/zip")) {
            stMimeType.m_strTypeName = "application/zip";
            return stMimeType;
        } else if (output.contains("application/x-iso9660-image")) {
            stMimeType.m_strTypeName = "application/x-iso9660-image";
            return stMimeType;
        } else {        // ????????????????????????????????????????????????????????????
            stMimeType.m_bUnKnown = false;
            stMimeType.m_mimeType = mimeFromExtension;
            return stMimeType;
        }
    }

    stMimeType.m_bUnKnown = false;
    // ??????????????????????????????????????????????????????
    if (mimeFromExtension != mimeFromContent) {
        if ((mimeFromContent.inherits(QStringLiteral("text/x-qml")) && fileinfo.completeSuffix().toLower().contains(QStringLiteral("rar")))
                || (mimeFromContent.name() == QStringLiteral("image/svg+xml") && mimeFromExtension.name() == QStringLiteral("application/zip"))) {
            stMimeType.m_mimeType = mimeFromExtension;
            return stMimeType;
        }

    }

    stMimeType.m_mimeType = mimeFromContent;
    return stMimeType;
}
