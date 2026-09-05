// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QString>

class WebApp
{
    Q_GADGET
    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(QString codename READ codename CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(QString exec READ exec CONSTANT)
    Q_PROPERTY(QString category READ category CONSTANT)
    Q_PROPERTY(QString browserName READ browserName CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(QString customParameters READ customParameters CONSTANT)
    Q_PROPERTY(bool isolateProfile READ isolateProfile CONSTANT)
    Q_PROPERTY(bool navbar READ navbar CONSTANT)
    Q_PROPERTY(bool privateWindow READ privateWindow CONSTANT)
    Q_PROPERTY(bool valid READ isValid CONSTANT)

public:
    WebApp() = default;

    [[nodiscard]] QString path() const { return m_path; }
    void setPath(const QString &path) { m_path = path; }

    [[nodiscard]] QString codename() const { return m_codename; }
    void setCodename(const QString &codename) { m_codename = codename; }

    [[nodiscard]] QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    [[nodiscard]] QString description() const { return m_description; }
    void setDescription(const QString &description)
    {
        m_description = description;
    }

    [[nodiscard]] QString icon() const { return m_icon; }
    void setIcon(const QString &icon) { m_icon = icon; }

    [[nodiscard]] QString exec() const { return m_exec; }
    void setExec(const QString &exec) { m_exec = exec; }

    [[nodiscard]] QString category() const { return m_category; }
    void setCategory(const QString &category) { m_category = category; }

    [[nodiscard]] QString browserName() const { return m_browserName; }
    void setBrowserName(const QString &browserName)
    {
        m_browserName = browserName;
    }

    [[nodiscard]] QString url() const { return m_url; }
    void setUrl(const QString &url) { m_url = url; }

    [[nodiscard]] QString customParameters() const
    {
        return m_customParameters;
    }
    void setCustomParameters(const QString &params)
    {
        m_customParameters = params;
    }

    [[nodiscard]] bool isolateProfile() const { return m_isolateProfile; }
    void setIsolateProfile(bool value) { m_isolateProfile = value; }

    [[nodiscard]] bool navbar() const { return m_navbar; }
    void setNavbar(bool value) { m_navbar = value; }

    [[nodiscard]] bool privateWindow() const { return m_privateWindow; }
    void setPrivateWindow(bool value) { m_privateWindow = value; }

    [[nodiscard]] bool isValid() const { return m_valid; }
    void setValid(bool valid) { m_valid = valid; }

private:
    QString m_path;
    QString m_codename;
    QString m_name;
    QString m_description;
    QString m_icon;
    QString m_exec;
    QString m_category;
    QString m_browserName;
    QString m_url;
    QString m_customParameters;
    bool m_isolateProfile = false;
    bool m_navbar = false;
    bool m_privateWindow = false;
    bool m_valid = false;
};

Q_DECLARE_METATYPE(WebApp)
