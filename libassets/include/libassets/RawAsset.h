//
// Created by droc101 on 6/23/25.
//

#ifndef RAWASSET_H
#define RAWASSET_H
#include "Asset.h"

/**
 * A raw asset, exposed for direct data editing
 */
class RawAsset final: public Asset {

    using Asset::Asset;

    public:
        /**
         * Get the raw data of the asset
         */
        [[nodiscard]] uint8_t *GetData() const;
        /**
         * Get the size of the asset's data
         */
        [[nodiscard]] std::size_t GetDataSize() const;
        /**
         * Update the size of the asset's data
         * @param size The new size
         */
        void SetDataSize(std::size_t size);

        void FinishLoading() override;

    protected:
        uint8_t *data = nullptr;
        std::size_t data_size = 0;

        uint8_t* SaveToBuffer(std::size_t *outSize) override;
};



#endif //RAWASSET_H
