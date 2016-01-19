#ifndef ENGINE_AMBIENTOCCLUSIONMANAGER_H
#define ENGINE_AMBIENTOCCLUSIONMANAGER_H

#include "UrchinCommon.h"

#include "scene/renderer3d/camera/Camera.h"
#include "utils/display/quad/QuadDisplayer.h"
#include "utils/filter/bilateralblur/BilateralBlurFilter.h"

namespace urchin
{

	/**
	* Manager for ambient occlusion (horizon based ambient occlusion - HBAO)
	*/
	class AmbientOcclusionManager
	{
		public:
			enum AOTextureSize
			{
				FULL_SIZE = 0,
				HALF_SIZE = 1
			};

			AmbientOcclusionManager();
			~AmbientOcclusionManager();

			void initialize(unsigned int, unsigned int, unsigned int);
			void onResize(int, int);
			void createOrUpdateTexture();
			void onCameraProjectionUpdate(const Camera *const);

			void setTextureSize(AOTextureSize);
			void setNumDirections(unsigned int);
			void setNumSteps(unsigned int);
			void setRadius(float);
			void setBlurSize(unsigned int);
			void setBlurSharpness(float);

			unsigned int getAmbientOcclusionTextureID() const;

			void updateAOTexture(const Camera *const);
			void loadAOTexture(unsigned int) const;

		private:
			void computeTextureSize();

			//scene information
			bool isInitialized;
			int sceneWidth, sceneHeight;
			float nearPlane, farPlane;

			//tweak
			AOTextureSize textureSize;
			int textureSizeX, textureSizeY;
			unsigned int numDirections;
			unsigned int numSteps;
			float radius;
			unsigned int blurSize;
			float blurSharpness;

			//frame buffer object
			unsigned int fboID;
			unsigned int ambientOcclusionTexID;

			//ambient occlusion shader
			unsigned int hbaoShader;
			int mInverseViewProjectionLoc;
			int cameraPlanesLoc;
			int invResolutionLoc;
			int nearPlaneScreenRadiusLoc;

			//visual data
			unsigned int depthTexID;
			unsigned int normalAndAmbientTexID;
			unsigned int ambienOcclusionTexLoc;
			std::shared_ptr<QuadDisplayer> quadDisplayer;
			std::shared_ptr<BilateralBlurFilter> verticalBlurFilter;
			std::shared_ptr<BilateralBlurFilter> horizontalBlurFilter;
	};

}

#endif
