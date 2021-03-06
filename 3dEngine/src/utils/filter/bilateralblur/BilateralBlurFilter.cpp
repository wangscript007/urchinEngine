#include <GL/glew.h>
#include <stdexcept>

#include "BilateralBlurFilter.h"
#include "utils/filter/bilateralblur/BilateralBlurFilterBuilder.h"
#include "utils/shader/ShaderManager.h"

namespace urchin
{

    BilateralBlurFilter::BilateralBlurFilter(const BilateralBlurFilterBuilder *textureFilterBuilder, BlurDirection blurDirection):
        TextureFilter(textureFilterBuilder),
        blurDirection(blurDirection),
        blurSize(textureFilterBuilder->getBlurSize()),
        blurSharpness(textureFilterBuilder->getBlurSharpness()),
        depthTextureID(textureFilterBuilder->getDepthTextureID()),
        textureSize((BlurDirection::VERTICAL==blurDirection) ? getTextureHeight() : getTextureWidth()),
        cameraPlanesLoc(0)
    {
        if(blurSize<=1)
        {
            throw std::invalid_argument("Blur size must be greater than one. Value: " + std::to_string(blurSize));
        }else if(blurSize%2==0)
        {
            throw std::invalid_argument("Blur size must be an odd number. Value: " + std::to_string(blurSize));
        }

        std::vector<float> offsets = computeOffsets();
        offsetsTab = toShaderVectorValues(offsets);
    }

    void BilateralBlurFilter::onCameraProjectionUpdate(float nearPlane, float farPlane)
    {
        ShaderManager::instance()->bind(getTextureFilterShader());

        float cameraPlanes[2] = {nearPlane, farPlane};
        glUniform1fv(cameraPlanesLoc, 2, cameraPlanes);
    }

    std::string BilateralBlurFilter::getShaderName() const
    {
        return "bilateralBlurTex";
    }

    void BilateralBlurFilter::initializeAdditionalUniforms(unsigned int textureFilterShader)
    {
        int depthTexLoc = glGetUniformLocation(textureFilterShader, "depthTex");
        glUniform1i(depthTexLoc, GL_TEXTURE1-GL_TEXTURE0);

        cameraPlanesLoc = glGetUniformLocation(textureFilterShader, "cameraPlanes");
    }

    void BilateralBlurFilter::bindAdditionalTextures() const
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthTextureID);
    }

    void BilateralBlurFilter::completeShaderTokens(std::map<std::string, std::string> &shaderTokens) const
    {
        shaderTokens["IS_VERTICAL_BLUR"] = (blurDirection==BlurDirection::VERTICAL) ? "true" : "false";
        shaderTokens["KERNEL_RADIUS"] = std::to_string(blurSize / 2);
        shaderTokens["BLUR_SHARPNESS"] = std::to_string(blurSharpness);
        shaderTokens["OFFSETS_TAB"] = offsetsTab;
    }

    std::vector<float> BilateralBlurFilter::computeOffsets() const
    {
        unsigned int numOffsets = blurSize / 2;
        std::vector<float> offsets(numOffsets, 0.0f);

        if(textureSize!=0)
        {
            float pixelSize = 1.0f / static_cast<float>(textureSize);
            for(unsigned int i=1; i<=numOffsets; ++i)
            {
                offsets[i-1] = pixelSize * (float)i;
            }
        }

        return offsets;
    }
}
