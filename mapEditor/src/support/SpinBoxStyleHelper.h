#ifndef URCHINENGINE_MAPEDITOR_SPINBOXSTYLEHELPER_H
#define URCHINENGINE_MAPEDITOR_SPINBOXSTYLEHELPER_H

#include <QtWidgets/QDoubleSpinBox>

namespace urchin
{

	class SpinBoxStyleHelper
	{
		public:
			static void applyDefaultStyleOn(QDoubleSpinBox *);
			static void applyAngleStyleOn(QDoubleSpinBox *);
			static void applyPercentageStyleOn(QDoubleSpinBox *);
	};

}

#endif
