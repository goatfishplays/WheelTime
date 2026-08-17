/**
 * @file Theme.hpp
 * @brief Stylesheet composition and rice layout settings for the radial wheel.
 */
#pragma once

#include <optional>

#include <QString>

namespace Application
{

// Temporarily widened for layout experiments; restore ~0.15–0.48 before release.
constexpr double kMinButtonRadiusFraction = 0.0;
constexpr double kMaxButtonRadiusFraction = 0.5;
constexpr double kDefaultButtonRadiusFraction = 0.32;
constexpr double kDefaultMouseOpenOffsetYFraction = 0.02;
constexpr double kMinMouseOpenOffsetFraction = -0.5;
constexpr double kMaxMouseOpenOffsetFraction = 0.5;

/// @brief Global (or fully resolved) wheel layout numbers. Colors stay in QSS.
struct RiceSettings
{
    /// @brief Distance from wheel center to buttons, as a fraction of the overlay's shorter side.
    double buttonRadiusFraction = kDefaultButtonRadiusFraction;
    std::optional<int> buttonRadiusPx;
    double startAngleDegrees = 0.0;
    /// @brief Hole radius as a fraction of the overlay's shorter side.
    double innerDeadzoneFraction = 0.0;
    std::optional<int> innerDeadzonePx;
    /// @brief Fraction of overlay width from wheel center; positive is right.
    double mouseOpenOffsetXFraction = 0.0;
    /// @brief Fraction of overlay height from wheel center; positive is up.
    double mouseOpenOffsetYFraction = kDefaultMouseOpenOffsetYFraction;
};

/// @brief Sparse per-menu overrides. Missing fields inherit the global rice.
struct RiceOverrides
{
    std::optional<double> buttonRadiusFraction;
    std::optional<int> buttonRadiusPx;
    std::optional<double> startAngleDegrees;
    std::optional<double> innerDeadzoneFraction;
    std::optional<int> innerDeadzonePx;
    std::optional<double> mouseOpenOffsetXFraction;
    std::optional<double> mouseOpenOffsetYFraction;
};

[[nodiscard]] bool operator==(const RiceSettings &lhs, const RiceSettings &rhs) noexcept;
[[nodiscard]] bool operator!=(const RiceSettings &lhs, const RiceSettings &rhs) noexcept;
[[nodiscard]] bool operator==(const RiceOverrides &lhs, const RiceOverrides &rhs) noexcept;
[[nodiscard]] bool operator!=(const RiceOverrides &lhs, const RiceOverrides &rhs) noexcept;

[[nodiscard]] bool riceOverridesEmpty(const RiceOverrides &overrides) noexcept;

namespace Theme
{

/// @brief Merge per-menu overrides onto global rice. Radius/deadzone overrides
/// replace the whole dual-mode pair so a fraction override cannot inherit a
/// leftover global pixel radius.
[[nodiscard]] RiceSettings resolve(const RiceSettings &global, const RiceOverrides &overrides);

/// @brief Bundled light/dark QSS, then the overlay file if @p overlayPath exists.
[[nodiscard]] QString composeStylesheet(bool dark, const QString &overlayPath);

/// @brief Absolute overlay path; relative paths are resolved against the config dir.
[[nodiscard]] QString resolveOverlayPath(const QString &overlayPath, const QString &configFilePath);

/// @brief Writes themes/example-overlay.qss next to the user config if missing.
void seedExampleOverlay(const QString &configFilePath);

} // namespace Theme

} // namespace Application
