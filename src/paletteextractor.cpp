#include "paletteextractor.h"

#include <QColor>
#include <QImage>
#include <QMap>
#include <algorithm>
#include <cmath>
#include <vector>

namespace {

struct Cluster {
	quint64 r = 0, g = 0, b = 0;
	int count = 0;

	QColor average() const
	{
		return QColor(int(r / count), int(g / count), int(b / count));
	}
};

// Buckets pixels into a coarse RGB grid (8 levels/channel = 512 buckets)
// while keeping a running true-colour average per bucket, so the result
// isn't just the bucket's quantized centre.
QMap<int, Cluster> buildHistogram(const QImage &image)
{
	QMap<int, Cluster> histogram;
	for (int y = 0; y < image.height(); y++) {
		for (int x = 0; x < image.width(); x++) {
			QColor c = image.pixelColor(x, y);
			int key = ((c.red() >> 5) << 6) |
				  ((c.green() >> 5) << 3) | (c.blue() >> 5);
			Cluster &cluster = histogram[key];
			cluster.r += c.red();
			cluster.g += c.green();
			cluster.b += c.blue();
			cluster.count++;
		}
	}
	return histogram;
}

double luminance(const QColor &c)
{
	return 0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue();
}

int colorDistance(const QColor &a, const QColor &b)
{
	return std::abs(a.red() - b.red()) + std::abs(a.green() - b.green()) +
	       std::abs(a.blue() - b.blue());
}

QString toHex(const QColor &c)
{
	return QString("%1%2%3")
		.arg(c.red(), 2, 16, QChar('0'))
		.arg(c.green(), 2, 16, QChar('0'))
		.arg(c.blue(), 2, 16, QChar('0'));
}

// Picks up to `count` mutually-distinct, non-extreme colours from the
// histogram, ordered by frequency. Falls back to whatever's available
// (extremes included) if the image is too monochrome to yield enough
// distinct clusters otherwise.
std::vector<QColor> pickColors(const std::vector<Cluster> &sortedClusters,
				int count)
{
	auto isExtreme = [](const QColor &c) {
		double l = luminance(c);
		return l < 20 || l > 235;
	};

	std::vector<QColor> picked;
	for (const Cluster &cluster : sortedClusters) {
		if (int(picked.size()) == count)
			return picked;
		QColor candidate = cluster.average();
		if (isExtreme(candidate))
			continue;
		bool tooClose = std::any_of(
			picked.begin(), picked.end(), [&](const QColor &c) {
				return colorDistance(candidate, c) < 60;
			});
		if (!tooClose)
			picked.push_back(candidate);
	}
	for (const Cluster &cluster : sortedClusters) {
		if (int(picked.size()) == count)
			break;
		QColor candidate = cluster.average();
		if (std::find(picked.begin(), picked.end(), candidate) ==
		    picked.end())
			picked.push_back(candidate);
	}
	return picked;
}

} // namespace

QVariantMap PaletteExtractor::extract(const QUrl &wallpaperUrl)
{
	QVariantMap result;

	QImage image(wallpaperUrl.toLocalFile());
	if (image.isNull())
		return result;

	// Only the colour distribution matters, not pixel-accurate detail,
	// so downscale before histogramming.
	image = image.convertToFormat(QImage::Format_RGB32)
			.scaled(64, 64, Qt::IgnoreAspectRatio,
				Qt::FastTransformation);

	QMap<int, Cluster> histogram = buildHistogram(image);
	std::vector<Cluster> clusters;
	clusters.reserve(histogram.size());
	for (const Cluster &c : histogram)
		clusters.push_back(c);
	std::sort(clusters.begin(), clusters.end(),
		  [](const Cluster &a, const Cluster &b) {
			  return a.count > b.count;
		  });

	std::vector<QColor> picked = pickColors(clusters, 3);
	if (picked.empty())
		return result;

	QColor primary = picked[0];
	QColor secondary = picked.size() > 1 ? picked[1] : primary.lighter(130);
	QColor accent;
	if (picked.size() > 2) {
		accent = picked[2];
	} else {
		// Not enough distinct clusters in the wallpaper - rotate the
		// primary colour's hue instead of leaving accent undefined.
		int h, s, v, a;
		primary.getHsv(&h, &s, &v, &a);
		accent = QColor::fromHsv((h + 40) % 360, qMax(s, 150),
					  qMax(v, 180));
	}

	bool isDark = luminance(primary) < 128;
	QString variant = isDark ? "dark" : "light";
	QString textColor = isDark ? "ffffff" : "000000";

	result["primaryColor"] = "#" + toHex(primary);
	result["primaryAlphaColor"] = "#80" + toHex(primary);
	result["secondaryColor"] = "#" + toHex(secondary);
	result["secondaryAlphaColor"] = "#65" + toHex(secondary);
	result["accentColor"] = "#" + toHex(accent);
	result["textColor"] = "#" + textColor;
	result["variant"] = variant;

	return result;
}
