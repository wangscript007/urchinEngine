#ifndef ENGINE_COLLISIONCONEREADERWRITER_H
#define ENGINE_COLLISIONCONEREADERWRITER_H

#include <memory>
#include "UrchinCommon.h"
#include "UrchinPhysicsEngine.h"

#include "CollisionShapeReaderWriter.h"

namespace urchin
{

	class CollisionConeReaderWriter : public CollisionShapeReaderWriter
	{
		//XML tags
		#define ORIENTATION_TAG "orientation"
		#define RADIUS_TAG "radius"
		#define HEIGHT_TAG "height"

		//XML value
		#define X_POSITIVE_VALUE "XPositive"
		#define X_NEGATIVE_VALUE "XNegative"
		#define Y_POSITIVE_VALUE "YPositive"
		#define Y_NEGATIVE_VALUE "YNegative"
		#define Z_POSITIVE_VALUE "ZPositive"
		#define Z_NEGATIVE_VALUE "ZNegative"

		public:
			CollisionConeReaderWriter();
			virtual ~CollisionConeReaderWriter();

			CollisionShape3D *loadFrom(std::shared_ptr<XmlChunk>, const XmlParser &) const;
			void writeOn(std::shared_ptr<XmlChunk>, const CollisionShape3D *, XmlWriter &) const;
	};

}

#endif