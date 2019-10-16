#include <GL/glew.h>
#include <cmath>
#include <utility>

#include "utils/display/geometry/cylinder/CylinderModel.h"

namespace urchin
{

	CylinderModel::CylinderModel(Cylinder<float> cylinder, int sides):
			cylinder(std::move(cylinder)),
			sides(sides)
	{
		initialize();
	}

	Matrix4<float> CylinderModel::retrieveModelMatrix() const
	{
		Matrix4<float> modelMatrix;
		modelMatrix.buildTranslation(cylinder.getCenterOfMass().X, cylinder.getCenterOfMass().Y, cylinder.getCenterOfMass().Z);

		return modelMatrix;
	}

	std::vector<Point3<float>> CylinderModel::retrieveVertexArray() const
	{
		std::vector<Point3<float>> vertexArray;
		vertexArray.reserve(2*(sides+1));

		float radius = cylinder.getRadius();
		float halfHeight = cylinder.getHeight() / 2.0f;
		float angle = (2.0f * (float)PI_VALUE) / (float)sides;

		Quaternion<float> qCylinderOrientation;
		CylinderShape<float>::CylinderOrientation cylinderOrientation = cylinder.getCylinderOrientation();
		if(cylinderOrientation==CylinderShape<float>::CYLINDER_X)
		{
			qCylinderOrientation = Quaternion<float>(Vector3<float>(0.0, 1.0, 0.0), PI_VALUE/2.0);
		}else if(cylinderOrientation==CylinderShape<float>::CYLINDER_Y)
		{
			qCylinderOrientation = Quaternion<float>(Vector3<float>(1.0, 0.0, 0.0), PI_VALUE/2.0);
		}else if(cylinderOrientation==CylinderShape<float>::CYLINDER_Z)
		{
			qCylinderOrientation = Quaternion<float>(0.0, 0.0, 0.0, 1.0);
		}

		Quaternion<float> localOrientation = cylinder.getOrientation() * qCylinderOrientation;
		for (int i = 0; i <= sides; i++)
		{
		    float x = std::cos((float)i * angle) * radius;
		    float y = std::sin((float)i * angle) * radius;

		    Point3<float> p1 = localOrientation.rotatePoint(Point3<float>(x, y, halfHeight));
		    Point3<float> p2 = localOrientation.rotatePoint(Point3<float>(x, y, -halfHeight));

		    vertexArray.push_back(p1);
		    vertexArray.push_back(p2);
		}

		return vertexArray;
	}

	void CylinderModel::drawGeometry() const
	{
		glDrawArrays(GL_QUAD_STRIP, 0, 2*(sides+1));
	}

}
