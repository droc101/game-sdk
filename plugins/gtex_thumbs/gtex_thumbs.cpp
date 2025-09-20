//
// Created by droc101 on 8/10/25.
//

#include "gtex_thumbs.h"
#include <cstdint>
#include <kio/thumbnailcreator.h>
#include <KPluginFactory>
#include <libassets/asset/TextureAsset.h>
#include <libassets/util/Error.h>
#include <QFile>
#include <QImage>
#include <vector>

K_PLUGIN_CLASS_WITH_JSON(gtex_thumbs, "gtex_thumbs.json")

#include "gtex_thumbs.moc"
#include "moc_gtex_thumbs.cpp"

KIO::ThumbnailResult gtex_thumbs::create(const KIO::ThumbnailRequest &request)
{
    if (!request.url().isLocalFile())
    {
        return KIO::ThumbnailResult::fail();
    }
    TextureAsset t;
    const Error::ErrorCode e = TextureAsset::CreateFromAsset(request.url().toLocalFile().toUtf8().data(), t);
    if (e != Error::ErrorCode::OK)
    {
        return KIO::ThumbnailResult::fail();
    }
    std::vector<uint32_t> pixels;
    t.GetPixelsRGBA(pixels);
    for (uint32_t &pixel: pixels)
    {
        pixel = __builtin_bswap32(pixel);
    }
    const QImage texture = QImage(reinterpret_cast<const uint8_t *>(pixels.data()),
                                  t.GetWidth(),
                                  t.GetHeight(),
                                  QImage::Format_RGBA8888)
                                   .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    if (texture.isNull())
    {
        return KIO::ThumbnailResult::fail();
    }
    return KIO::ThumbnailResult::pass(texture);
}
