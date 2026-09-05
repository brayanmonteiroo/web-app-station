// SPDX-License-Identifier: MIT
#include "FaviconService.h"

#include "Paths.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryFile>
#include <QUrl>
#include <QtConcurrent>

#include <algorithm>

FaviconService::FaviconService(QObject *parent)
    : QObject(parent)
{
}

void FaviconService::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    Q_EMIT busyChanged();
}

QString FaviconService::guessThemeIcon(const QString &url) const
{
    QUrl parsed(url);
    if (!parsed.isValid()) {
        return {};
    }
    QString host = parsed.host().toLower();
    if (host.startsWith(QStringLiteral("www."))) {
        host = host.mid(4);
    }
    const QString domain = host.section(QLatin1Char('.'), 0, 0);
    if (domain.isEmpty()) {
        return {};
    }

    if ((domain == QStringLiteral("google")
         && host.contains(QStringLiteral("mail")))
        || (domain == QStringLiteral("mail")
            && host.contains(QStringLiteral("google")))
        || domain == QStringLiteral("gmail")) {
        return QStringLiteral("web-google-gmail");
    }
    if (domain == QStringLiteral("youtube")) {
        return QStringLiteral("web-google-youtube");
    }
    return QStringLiteral("web-%1").arg(domain);
}

QString FaviconService::persistIcon(const QString &tempPath,
                                    const QString &appName) const
{
    Paths::ensureUserDirs();
    QString safe;
    for (QChar c : appName) {
        if (c.isLetter()) {
            safe.append(c);
        }
    }
    if (safe.isEmpty()) {
        safe = QStringLiteral("webapp");
    }
    const QString dest =
        QDir(Paths::iconsDir()).filePath(safe + QStringLiteral(".png"));
    QFile::remove(dest);
    if (!QFile::copy(tempPath, dest)) {
        return tempPath;
    }
    QFile::setPermissions(dest,
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner
                              | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    return dest;
}

void FaviconService::selectIcon(const QString &path)
{
    m_selectedPath = path;
    Q_EMIT iconSelected(path);
}

void FaviconService::findIcons(const QString &url)
{
    if (m_busy) {
        return;
    }
    setBusy(true);

    const QString target = url.trimmed();
    auto *watcher = new QFutureWatcher<QVariantList>(this);
    connect(watcher, &QFutureWatcher<QVariantList>::finished, this,
            [this, watcher]() {
                const QVariantList icons = watcher->result();
                watcher->deleteLater();
                setBusy(false);
                Q_EMIT iconsFound(icons);
            });

    watcher->setFuture(QtConcurrent::run([target]() -> QVariantList {
        QVariantList results;
        QUrl base(target);
        if (!base.scheme().startsWith(QStringLiteral("http"))) {
            base = QUrl(QStringLiteral("http://") + target);
        }
        if (!base.isValid()) {
            return results;
        }

        QNetworkAccessManager nam;
        QEventLoop loop;
        QNetworkReply *reply =
            nam.get(QNetworkRequest(base));
        QObject::connect(reply, &QNetworkReply::finished, &loop,
                         &QEventLoop::quit);
        loop.exec();

        QString html;
        if (reply->error() == QNetworkReply::NoError) {
            html = QString::fromUtf8(reply->readAll());
        }
        reply->deleteLater();

        QStringList candidates;
        const QRegularExpression linkRe(
            QStringLiteral(
                R"re(<link[^>]+rel=["']([^"']*icon[^"']*)["'][^>]*href=["']([^"']+)["'])re"),
            QRegularExpression::CaseInsensitiveOption);
        auto it = linkRe.globalMatch(html);
        while (it.hasNext()) {
            candidates.append(it.next().captured(2));
        }
        candidates.append(QStringLiteral("/favicon.ico"));
        candidates.append(
            QStringLiteral("https://www.google.com/s2/favicons?sz=64&domain=%1")
                .arg(base.host()));

        QSet<QString> seen;
        for (QString link : candidates) {
            if (link.startsWith(QStringLiteral("//"))) {
                link = base.scheme() + QLatin1Char(':') + link;
            } else if (link.startsWith(QLatin1Char('/'))) {
                link = base.scheme() + QStringLiteral("://") + base.host()
                    + link;
            } else if (!link.contains(QStringLiteral("://"))) {
                link = base.resolved(QUrl(link)).toString();
            }
            if (seen.contains(link)) {
                continue;
            }
            seen.insert(link);

            QNetworkReply *imgReply =
                nam.get(QNetworkRequest(QUrl(link)));
            QEventLoop imgLoop;
            QObject::connect(imgReply, &QNetworkReply::finished, &imgLoop,
                             &QEventLoop::quit);
            imgLoop.exec();
            if (imgReply->error() != QNetworkReply::NoError) {
                imgReply->deleteLater();
                continue;
            }
            QByteArray data = imgReply->readAll();
            imgReply->deleteLater();

            QImage image;
            if (!image.loadFromData(data)) {
                continue;
            }
            if (image.height() > 256) {
                image = image.scaled(256, 256, Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
            }

            QTemporaryFile tmp(
                QDir::temp().filePath(QStringLiteral("was-icon-XXXXXX.png")));
            tmp.setAutoRemove(false);
            if (!tmp.open()) {
                continue;
            }
            image.save(&tmp, "PNG");
            tmp.close();

            QVariantMap entry;
            entry.insert(QStringLiteral("path"), tmp.fileName());
            entry.insert(QStringLiteral("width"), image.width());
            entry.insert(QStringLiteral("height"), image.height());
            entry.insert(QStringLiteral("origin"), link);
            results.append(entry);
            if (results.size() >= 8) {
                break;
            }
        }

        std::sort(results.begin(), results.end(),
                  [](const QVariant &a, const QVariant &b) {
                      return a.toMap().value(QStringLiteral("height")).toInt()
                          > b.toMap()
                                .value(QStringLiteral("height"))
                                .toInt();
                  });
        return results;
    }));
}
