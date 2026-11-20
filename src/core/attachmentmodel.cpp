#include "attachmentmodel.h"

#include "appconfig.h"

#include <QBuffer>
#include <QClipboard>
#include <QDir>
#include <QLocale>
#include <QDateTime>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMimeData>
#include <QMimeDatabase>
#include <QUrl>

#include <utility>

namespace {
constexpr qint64 kMiB = 1024 * 1024;

QString sizeText(qint64 bytes)
{
    if (bytes < 0) return QObject::tr("unknown size");
    return QLocale().formattedDataSize(bytes);
}
} // namespace

AttachmentModel::AttachmentModel(AppConfig *config, QObject *parent)
    : QAbstractListModel(parent), m_config(config)
{
}

int AttachmentModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant AttachmentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const Item &it = m_items.at(index.row());
    switch (role) {
    case UriRole: return it.uri;
    case NameRole: return it.name;
    case MimeRole: return it.mime;
    case SizeRole: return it.size;
    case DataUrlRole: return it.mime.startsWith(QLatin1String("image/")) ? it.uri : QString();
    case SourceTypeRole: return it.sourceType;
    case IsImageRole: return it.mime.startsWith(QLatin1String("image/"));
    }
    return {};
}

QHash<int, QByteArray> AttachmentModel::roleNames() const
{
    return {{UriRole, "uri"}, {NameRole, "name"}, {MimeRole, "mime"}, {SizeRole, "size"},
            {DataUrlRole, "dataUrl"}, {SourceTypeRole, "sourceType"}, {IsImageRole, "isImage"}};
}

bool AttachmentModel::isSupportedMime(const QString &mime)
{
    if (mime.startsWith(QLatin1String("text/")))
        return true;
    static const QStringList exact = {
        QStringLiteral("image/png"), QStringLiteral("image/jpeg"), QStringLiteral("image/gif"),
        QStringLiteral("image/webp"), QStringLiteral("application/pdf"), QStringLiteral("application/json"),
        QStringLiteral("application/xml"), QStringLiteral("application/yaml"),
        QStringLiteral("application/x-yaml"), QStringLiteral("application/toml"),
        QStringLiteral("application/javascript"), QStringLiteral("application/x-sh"),
    };
    return exact.contains(mime);
}

qint64 AttachmentModel::sizeLimitBytes() const
{
    const int mb = m_config ? m_config->maxAttachmentSizeMB() : 0;
    return mb > 0 ? static_cast<qint64>(mb) * kMiB : 0;   // 0 = no limit
}

QString AttachmentModel::addFile(const QString &urlOrPath)
{
    const QString path = urlOrPath.startsWith(QLatin1String("file://"))
                             ? QUrl(urlOrPath).toLocalFile()
                             : urlOrPath;
    return addLocalFile(path, QFileInfo(path).fileName());
}

QVariantList AttachmentModel::listDirectory(const QString &dirPath) const
{
    QVariantList out;
    const QDir dir(dirPath);
    if (!dir.exists() || !dir.isReadable()) {
        out << QVariantMap{{QStringLiteral("key"), QString()}, {QStringLiteral("label"), tr("This folder cannot be opened")},
                           {QStringLiteral("sub"), QString()}, {QStringLiteral("tag"), QString()},
                           {QStringLiteral("kind"), QStringLiteral("none")}};
        return out;
    }
    QDir parent(dirPath);
    if (parent.cdUp()) {   // parent folder (not offered at the filesystem root)
        out << QVariantMap{{QStringLiteral("key"), parent.absolutePath()}, {QStringLiteral("label"), QStringLiteral("..")},
                           {QStringLiteral("sub"), tr("Parent folder")}, {QStringLiteral("tag"), QString()},
                           {QStringLiteral("kind"), QStringLiteral("dir")}};
    }
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable,
                                                     QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    QVariantList folders, files;
    for (const QFileInfo &fi : entries) {
        const QString name = fi.fileName();
        if (name.startsWith(QLatin1Char('.')))           // hidden entries
            continue;
        if (fi.isDir()) {
            folders << QVariantMap{{QStringLiteral("key"), fi.absoluteFilePath()},
                                   {QStringLiteral("label"), name + QLatin1Char('/')},
                                   {QStringLiteral("sub"), tr("Folder")}, {QStringLiteral("tag"), QString()},
                                   {QStringLiteral("kind"), QStringLiteral("dir")}};
            continue;
        }
        const QString mime = QMimeDatabase().mimeTypeForFile(fi).name();
        if (!isSupportedMime(mime))                         // only file types Qute can send
            continue;
        const qint64 size = fi.size();
        const QString sub = size < 1024 ? tr("%1 B").arg(size)
                          : size < 1048576 ? tr("%1 KB").arg(size / 1024)
                          : tr("%1 MB").arg(QString::number(size / 1048576.0, 'f', 1));
        files << QVariantMap{{QStringLiteral("key"), fi.absoluteFilePath()}, {QStringLiteral("label"), name},
                             {QStringLiteral("sub"), sub}, {QStringLiteral("tag"), QString()},
                             {QStringLiteral("kind"), QStringLiteral("file")}};
    }
    out << folders << files;
    if (folders.isEmpty() && files.isEmpty())
        out << QVariantMap{{QStringLiteral("key"), QString()}, {QStringLiteral("label"), tr("No supported files here")},
                           {QStringLiteral("sub"), QString()}, {QStringLiteral("tag"), QString()},
                           {QStringLiteral("kind"), QStringLiteral("none")}};
    return out;
}

QString AttachmentModel::homeDirectory() const
{
    return QDir::homePath();
}

bool AttachmentModel::isDirectory(const QString &path) const
{
    return QFileInfo(path).isDir();
}

QString AttachmentModel::defaultDirectory() const
{
    const QString work = m_config ? m_config->workDir() : QString();
    return QDir(work).exists() ? work : QDir::homePath();
}

QString AttachmentModel::addMention(const QString &relativePath, const QString &baseDir)
{
    const QString path = QDir::cleanPath(baseDir + QLatin1Char('/') + relativePath);
    return addLocalFile(path, relativePath);
}

QString AttachmentModel::addLocalFile(const QString &path, const QString &displayName)
{
    const QFileInfo fi(path);
    const QString name = displayName.isEmpty() ? fi.fileName() : displayName;
    if (!fi.exists())
        return tr("File not found: %1").arg(name);
    if (fi.isDir())
        return tr("Folders can't be attached: %1").arg(name);
    if (!fi.isReadable())
        return tr("Can't read the file: %1").arg(name);

    const qint64 size = fi.size();
    const qint64 limit = sizeLimitBytes();
    if (limit > 0 && size > limit)
        return tr("File too large: %1 (%2, limit %3)").arg(name, sizeText(size), sizeText(limit));

    const QString mime = QMimeDatabase().mimeTypeForFile(fi).name();
    if (!isSupportedMime(mime))
        return tr("Unsupported file type (%1): %2").arg(mime, name);

    const QString uri = QUrl::fromLocalFile(fi.absoluteFilePath()).toString();
    for (const Item &it : std::as_const(m_items))
        if (it.uri == uri)
            return QString();          // already attached: nothing to do

    if (m_items.size() >= kMaxAttachments)
        return tr("Too many attachments (max %1)").arg(kMaxAttachments);

    beginInsertRows({}, m_items.size(), m_items.size());
    m_items.append(Item{uri, name, mime, size, QStringLiteral("uri")});
    endInsertRows();
    emit countChanged();
    return QString();
}

QString AttachmentModel::attachImage(const QImage &image, const QString &name)
{
    if (image.isNull())
        return tr("The clipboard image could not be read");
    QByteArray png;
    {
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "PNG"))
            return tr("The clipboard image could not be encoded");
    }
    const qint64 limit = sizeLimitBytes();
    if (limit > 0 && png.size() > limit)
        return tr("Image too large: %1 (limit %2)").arg(sizeText(png.size()), sizeText(limit));
    if (m_items.size() >= kMaxAttachments)
        return tr("Too many attachments (max %1)").arg(kMaxAttachments);

    const QString label = name.isEmpty()
                              ? QStringLiteral("pasted-%1.png").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")))
                              : name;
    const QString uri = QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64());
    beginInsertRows({}, m_items.size(), m_items.size());
    m_items.append(Item{uri, label, QStringLiteral("image/png"), png.size(), QStringLiteral("inline")});
    endInsertRows();
    emit countChanged();
    return QString();
}

bool AttachmentModel::clipboardHasText() const
{
    const QClipboard *cb = QGuiApplication::clipboard();
    return cb && cb->mimeData() && cb->mimeData()->hasText();
}

bool AttachmentModel::clipboardHasImage() const
{
    const QClipboard *cb = QGuiApplication::clipboard();
    return cb && cb->mimeData() && cb->mimeData()->hasImage();
}

QString AttachmentModel::pasteClipboardImage()
{
    const QClipboard *cb = QGuiApplication::clipboard();
    if (!cb)
        return tr("No clipboard available");
    return attachImage(cb->image());
}

void AttachmentModel::remove(int row)
{
    if (row < 0 || row >= m_items.size())
        return;
    beginRemoveRows({}, row, row);
    m_items.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

void AttachmentModel::clear()
{
    if (m_items.isEmpty())
        return;
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

QVariantList AttachmentModel::promptFiles() const
{
    QVariantList out;
    for (const Item &it : m_items)
        out << QVariantMap{{QStringLiteral("uri"), it.uri}, {QStringLiteral("name"), it.name}};
    return out;
}

QVariantList AttachmentModel::agentRefs(const QStringList &names) const
{
    QVariantList out;
    for (const QString &n : names)
        if (!n.isEmpty())
            out << QVariantMap{{QStringLiteral("name"), n}};
    return out;
}
