#pragma once

#include <QUrl>
#include <QVariantMap>

// Derives a settings.ini-compatible colour palette (primary/secondary/accent
// colours, their alpha variants, text colour and light/dark variant) from a
// wallpaper image. Kept separate from AtmosphereModel so this logic can be
// tuned/tested without touching the QML-facing API.
//
// Returned keys mirror settings.ini exactly: primaryColor, primaryAlphaColor,
// secondaryColor, secondaryAlphaColor, accentColor, textColor, variant.
// Colour values are "#rrggbb", ready to bind directly in QML; variant is
// "light" or "dark" with no leading '#'. Returns an empty map if the image
// couldn't be loaded.
class PaletteExtractor {
    public:
	static QVariantMap extract(const QUrl &wallpaperUrl);
};
