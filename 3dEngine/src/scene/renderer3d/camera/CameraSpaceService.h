#ifndef URCHINENGINE_CAMERASPACESERVICE_H
#define URCHINENGINE_CAMERASPACESERVICE_H

#include "UrchinCommon.h"

#include "scene/renderer3d/camera/Camera.h"

namespace urchin
{

    class CameraSpaceService
    {
        public:
            CameraSpaceService(Camera *);

            Point2<float> worldSpacePointToScreenSpace(const Point3<float> &) const;
            Ray<float> screenPointToRay(const Point2<float> &, float) const;

        private:
            Camera *camera;
    };

}

#endif
