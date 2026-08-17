/**
 * @file Theme.cpp
 * @brief Stylesheet composition and rice layout helpers.
 */

#include "App/Theme.hpp"

#include <algorithm>
#include <cmath>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace Application
{

bool operator==(const RiceSettings &lhs, const RiceSettings &rhs) noexcept
{
    return lhs.buttonRadiusFraction == rhs.buttonRadiusFraction
        && lhs.buttonRadiusPx == rhs.buttonRadiusPx
        && lhs.startAngleDegrees == rhs.startAngleDegrees
        && lhs.innerDeadzoneFraction == rhs.innerDeadzoneFraction
        && lhs.innerDeadzonePx == rhs.innerDeadzonePx
        && lhs.mouseOpenOffsetXFraction == rhs.mouseOpenOffsetXFraction
        && lhs.mouseOpenOffsetYFraction == rhs.mouseOpenOffsetYFraction;
}

bool operator!=(const RiceSettings &lhs, const RiceSettings &rhs) noexcept
{
    return !(lhs == rhs);
}

bool operator==(const RiceOverrides &lhs, const RiceOverrides &rhs) noexcept
{
    return lhs.buttonRadiusFraction == rhs.buttonRadiusFraction
        && lhs.buttonRadiusPx == rhs.buttonRadiusPx
        && lhs.startAngleDegrees == rhs.startAngleDegrees
        && lhs.innerDeadzoneFraction == rhs.innerDeadzoneFraction
        && lhs.innerDeadzonePx == rhs.innerDeadzonePx
        && lhs.mouseOpenOffsetXFraction == rhs.mouseOpenOffsetXFraction
        && lhs.mouseOpenOffsetYFraction == rhs.mouseOpenOffsetYFraction;
}

bool operator!=(const RiceOverrides &lhs, const RiceOverrides &rhs) noexcept
{
    return !(lhs == rhs);
}

bool riceOverridesEmpty(const RiceOverrides &overrides) noexcept
{
    return !overrides.buttonRadiusFraction.has_value()
        && !overrides.buttonRadiusPx.has_value()
        && !overrides.startAngleDegrees.has_value()
        && !overrides.innerDeadzoneFraction.has_value()
        && !overrides.innerDeadzonePx.has_value()
        && !overrides.mouseOpenOffsetXFraction.has_value()
        && !overrides.mouseOpenOffsetYFraction.has_value();
}

namespace Theme
{
namespace
{

double clampRadiusFraction(double fraction)
{
    return std::clamp(fraction, kMinButtonRadiusFraction, kMaxButtonRadiusFraction);
}

double clampDeadzoneFraction(double fraction)
{
    if (!std::isfinite(fraction))
    {
        return 0.0;
    }
    return std::clamp(fraction, 0.0, 1.0);
}

double clampMouseOpenOffset(double fraction)
{
    if (!std::isfinite(fraction))
    {
        return 0.0;
    }
    return std::clamp(fraction, kMinMouseOpenOffsetFraction, kMaxMouseOpenOffsetFraction);
}

} // namespace

RiceSettings resolve(const RiceSettings &global, const RiceOverrides &overrides)
{
    RiceSettings out = global;
    out.buttonRadiusFraction = clampRadiusFraction(out.buttonRadiusFraction);
    out.innerDeadzoneFraction = clampDeadzoneFraction(out.innerDeadzoneFraction);

    const bool radiusOverridden = overrides.buttonRadiusFraction.has_value()
        || overrides.buttonRadiusPx.has_value();
    if (radiusOverridden)
    {
        out.buttonRadiusFraction = clampRadiusFraction(
            overrides.buttonRadiusFraction.value_or(global.buttonRadiusFraction));
        out.buttonRadiusPx = overrides.buttonRadiusPx;
    }

    if (overrides.startAngleDegrees.has_value())
    {
        out.startAngleDegrees = *overrides.startAngleDegrees;
    }

    const bool deadzoneOverridden = overrides.innerDeadzoneFraction.has_value()
        || overrides.innerDeadzonePx.has_value();
    if (deadzoneOverridden)
    {
        out.innerDeadzoneFraction = clampDeadzoneFraction(
            overrides.innerDeadzoneFraction.value_or(0.0));
        out.innerDeadzonePx = overrides.innerDeadzonePx;
    }

    const bool mouseOffsetOverridden = overrides.mouseOpenOffsetXFraction.has_value()
        || overrides.mouseOpenOffsetYFraction.has_value();
    if (mouseOffsetOverridden)
    {
        out.mouseOpenOffsetXFraction = clampMouseOpenOffset(
            overrides.mouseOpenOffsetXFraction.value_or(global.mouseOpenOffsetXFraction));
        out.mouseOpenOffsetYFraction = clampMouseOpenOffset(
            overrides.mouseOpenOffsetYFraction.value_or(global.mouseOpenOffsetYFraction));
    }
    else
    {
        out.mouseOpenOffsetXFraction = clampMouseOpenOffset(out.mouseOpenOffsetXFraction);
        out.mouseOpenOffsetYFraction = clampMouseOpenOffset(out.mouseOpenOffsetYFraction);
    }

    if (out.buttonRadiusPx.has_value())
    {
        out.buttonRadiusPx = std::max(1, *out.buttonRadiusPx);
    }
    if (out.innerDeadzonePx.has_value())
    {
        out.innerDeadzonePx = std::max(0, *out.innerDeadzonePx);
    }

    return out;
}

QString composeStylesheet(bool dark, const QString &overlayPath)
{
    QFile base(dark ? QStringLiteral(":/styles/darkWheel.qss")
                    : QStringLiteral(":/styles/defaultWheel.qss"));
    QString sheet;
    if (base.open(QFile::ReadOnly))
    {
        sheet = QString::fromUtf8(base.readAll());
    }
    else
    {
        qWarning() << "Could not read bundled theme stylesheet";
    }

    if (overlayPath.isEmpty())
    {
        return sheet;
    }

    QFile overlay(overlayPath);
    if (!overlay.open(QFile::ReadOnly))
    {
        qWarning() << "theme overlay missing:" << overlayPath;
        return sheet;
    }

    if (!sheet.isEmpty())
    {
        sheet += QLatin1Char('\n');
    }
    sheet += QString::fromUtf8(overlay.readAll());
    return sheet;
}

QString resolveOverlayPath(const QString &overlayPath, const QString &configFilePath)
{
    const QString trimmed = overlayPath.trimmed();
    if (trimmed.isEmpty())
    {
        return {};
    }

    const QFileInfo given(trimmed);
    if (given.isAbsolute())
    {
        return given.absoluteFilePath();
    }

    const QDir configDir = QFileInfo(configFilePath).absoluteDir();
    return QFileInfo(configDir.filePath(trimmed)).absoluteFilePath();
}

void seedExampleOverlay(const QString &configFilePath)
{
    if (configFilePath.isEmpty())
    {
        return;
    }

    const QDir configDir = QFileInfo(configFilePath).absoluteDir();
    const QString themesDirPath = configDir.filePath(QStringLiteral("themes"));
    const QString destination = QDir(themesDirPath).filePath(QStringLiteral("example-overlay.qss"));
    if (QFile::exists(destination))
    {
        return;
    }

    QDir().mkpath(themesDirPath);

    QFile source(QStringLiteral(":/styles/exampleOverlay.qss"));
    if (!source.open(QFile::ReadOnly))
    {
        qWarning() << "Could not read bundled example overlay";
        return;
    }

    QFile out(destination);
    if (!out.open(QFile::WriteOnly | QFile::Truncate))
    {
        qWarning() << "Could not write example overlay to" << destination;
        return;
    }

    out.write(source.readAll());
}

} // namespace Theme

} // namespace Application
