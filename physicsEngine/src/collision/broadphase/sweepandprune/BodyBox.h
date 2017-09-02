#ifndef URCHINENGINE_BODYBOX_H
#define URCHINENGINE_BODYBOX_H

#include "body/work/AbstractWorkBody.h"
#include "collision/broadphase/PairContainer.h"

namespace urchin
{

	/**
	* Axis aligned bounding box representing an body during broad phase
	*/
	class BodyBox
	{
		public:
			BodyBox(AbstractWorkBody *, PairContainer *);

			AbstractWorkBody *getBody() const;

			AABBox<float> retrieveBodyAABBox() const;

			bool hasAlternativePairContainer() const;
			PairContainer *getAlternativePairContainer() const;

			//indices into the sorted end point arrays
			unsigned int min[3];
			unsigned int max[3];

		private:
			AbstractWorkBody *body;

			PairContainer *alternativePairContainer;
	};

}

#endif
