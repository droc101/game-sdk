//
// Created by droc101 on 8/10/25.
//

#pragma once

#include <kio/thumbnailcreator.h>
#include <QObject>

class gtex_thumbs final: public KIO::ThumbnailCreator
{
    Q_OBJECT

    public:
        using ThumbnailCreator::ThumbnailCreator;

        KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};
