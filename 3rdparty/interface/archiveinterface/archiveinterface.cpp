/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2009-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "archiveinterface.h"
#include "datamanager.h"

#include <QDebug>
#include <sys/stat.h>
#include <QDir>
#include <QStorageInfo>

Q_DECLARE_METATYPE(KPluginMetaData)


ReadOnlyArchiveInterface::ReadOnlyArchiveInterface(QObject *parent, const QVariantList &args)
    : QObject(parent)
{
    Q_ASSERT(args.size() >= 3);
//    qInfo() << "Created read-only interface for" << args.first().toString();
    m_strArchiveName = args.first().toString();
    m_metaData = args.at(1).value<KPluginMetaData>();
    m_mimetype = args.at(2).value<CustomMimeType>();

    m_common = new Common(this);
}

ReadOnlyArchiveInterface::~ReadOnlyArchiveInterface()
{

}

bool ReadOnlyArchiveInterface::waitForFinished()
{
    return m_bWaitForFinished;
}

void ReadOnlyArchiveInterface::setPassword(const QString &strPassword)
{
    m_strPassword = strPassword;
}


QString ReadOnlyArchiveInterface::getPassword()
{
    return m_strPassword;
}

ErrorType ReadOnlyArchiveInterface::errorType()
{
    return m_eErrorType;
}

bool ReadOnlyArchiveInterface::doKill()
{
    return false;   // ????????????????????????
}

void ReadOnlyArchiveInterface::setWaitForFinishedSignal(bool value)
{
    m_bWaitForFinished = value;
}

QFileDevice::Permissions ReadOnlyArchiveInterface::getPermissions(const mode_t &perm)
{
    QFileDevice::Permissions pers = QFileDevice::Permissions();

    if (perm == 0) {
        pers |= (QFileDevice::ReadUser | QFileDevice::WriteUser | QFileDevice::ReadGroup | QFileDevice::ReadOther);
        return pers;
    }

    if (perm & S_IRUSR) {
        pers |= QFileDevice::ReadUser;
    }
    if (perm & S_IWUSR) {
        pers |= QFileDevice::WriteUser;
    }
    if (perm & S_IXUSR) {
        pers |= QFileDevice::ExeUser;
    }

    if (perm & S_IRGRP) {
        pers |= QFileDevice::ReadGroup;
    }
    if (perm & S_IWGRP) {
        pers |= QFileDevice::WriteGroup;
    }
    if (perm & S_IXGRP) {
        pers |= QFileDevice::ExeGroup;
    }

    if (perm & S_IROTH) {
        pers |= QFileDevice::ReadOther;
    }
    if (perm & S_IWOTH) {
        pers |= QFileDevice::WriteOther;
    }
    if (perm & S_IXOTH) {
        pers |= QFileDevice::ExeOther;
    }

    return pers;
}

void ReadOnlyArchiveInterface::handleEntry(const FileEntry &entry)
{
    ArchiveData &stArchiveData = DataManager::get_instance().archiveData();
    if (!entry.strFullPath.contains(QDir::separator()) || (entry.strFullPath.count(QDir::separator()) == 1 && entry.strFullPath.endsWith(QDir::separator()))) {
        if (m_setHasRootDirs.contains(entry.strFullPath)) {
            // ????????????????????????????????????????????????????????????
            for (int i = 0; i < stArchiveData.listRootEntry.count(); ++i) {
                if (stArchiveData.listRootEntry[i].strFullPath == entry.strFullPath) {
                    stArchiveData.listRootEntry[i] = entry;
                    break;
                }
            }
        } else {
            // ?????????????????????????????????'/'????????????????????????'/'???
            stArchiveData.listRootEntry.push_back(entry);
            m_setHasRootDirs.insert(entry.strFullPath);
        }
    } else {
        // ?????????????????????????????????????????????????????????
        int iIndex = entry.strFullPath.lastIndexOf(QDir::separator());
        QString strDir = entry.strFullPath.left(iIndex + 1);

        // ?????????????????????????????????????????????
        if (!m_setHasHandlesDirs.contains(strDir)) {
            m_setHasHandlesDirs.insert(strDir);
            // ??????????????????????????????????????????????????????
            QStringList fileDirs = entry.strFullPath.split(QDir::separator());
            QString folderAppendStr = "";
            for (int i = 0 ; i < fileDirs.size() - 1; ++i) {
                folderAppendStr += fileDirs[i] + QDir::separator();

                // ??????????????????
                FileEntry entryDir;
                entryDir.strFullPath = folderAppendStr;
                entryDir.strFileName = fileDirs[i];
                entryDir.isDirectory = true;

                // ??????????????????????????????????????????????????????????????????
                if (i == 0 && !m_setHasRootDirs.contains(folderAppendStr)) {
                    stArchiveData.listRootEntry.push_back(entryDir);
                    m_setHasRootDirs.insert(folderAppendStr);
                }

                // ?????????????????????
                if (stArchiveData.mapFileEntry.find(entryDir.strFullPath) == stArchiveData.mapFileEntry.end()) {
                    stArchiveData.mapFileEntry[entryDir.strFullPath] = entryDir;
                }
            }
        }
    }
}

bool ReadOnlyArchiveInterface::isInsufficientDiskSpace(QString diskPath, qint64 standard)
{
    QStorageInfo storage(QFileInfo(diskPath).absolutePath());
    qInfo() << "Available DiskSpace:" << diskPath << storage << storage.bytesAvailable();
    if (storage.isValid() && storage.bytesAvailable() < standard) {
        return true;
    } else {
        return false;
    }
}

//bool ReadOnlyArchiveInterface::getHandleCurEntry() const
//{
//    return m_bHandleCurEntry;
//}

//void ReadOnlyArchiveInterface::getFileEntry(QList<FileEntry> &listRootEntry, QMap<QString, FileEntry> &mapEntry)
//{
//    listRootEntry.clear();
//    mapEntry.clear();

//    listRootEntry = m_listRootEntry;
//    mapEntry = m_mapEntry;
//}



ReadWriteArchiveInterface::ReadWriteArchiveInterface(QObject *parent, const QVariantList &args)
    : ReadOnlyArchiveInterface(parent, args)
{

}

ReadWriteArchiveInterface::~ReadWriteArchiveInterface()
{

}

QString ReadWriteArchiveInterface::getArchiveName()
{
    return m_strArchiveName;
}
