#ifndef URCHINENGINE_SOUNDBOXREADERWRITER_H
#define URCHINENGINE_SOUNDBOXREADERWRITER_H

#include <memory>

#include "UrchinCommon.h"
#include "UrchinSoundEngine.h"
#include "SoundShapeReaderWriter.h"

namespace urchin
{

    class SoundBoxReaderWriter : public SoundShapeReaderWriter
    {
        //XML tags
        #define POSITION_TAG "position"
        #define HALF_SIZES_TAG "halfSizes"
        #define ORIENTATION_TAG "orientation"
        #define MARGIN_TAG "margin"
        #define AXIS_TAG "axis"
        #define ANGLE_TAG "angle"

        public:
            ~SoundBoxReaderWriter() override = default;

            SoundShape *loadFrom(const std::shared_ptr<XmlChunk> &, const XmlParser &) const override;
            void writeOn(const std::shared_ptr<XmlChunk> &, const SoundShape *, XmlWriter &) const override;
    };

}

#endif
