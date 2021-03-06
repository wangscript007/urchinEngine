#include <algorithm>
#include <numeric>
#include <stdexcept>
#include "UrchinCommon.h"

#include "GaussianBlurFilter.h"
#include "utils/filter/gaussianblur/GaussianBlurFilterBuilder.h"

namespace urchin
{

    GaussianBlurFilter::GaussianBlurFilter(const GaussianBlurFilterBuilder *textureFilterBuilder, BlurDirection blurDirection):
        TextureFilter(textureFilterBuilder),
        blurDirection(blurDirection),
        blurSize(textureFilterBuilder->getBlurSize()),
        nbTextureFetch(std::ceil((float)blurSize / 2.0f)),
        textureSize((BlurDirection::VERTICAL==blurDirection) ? getTextureHeight() : getTextureWidth())
    { //See http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/

        if(blurSize<=1)
        {
            throw std::invalid_argument("Blur size must be greater than one. Value: " + std::to_string(blurSize));
        }else if(blurSize%2==0)
        {
            throw std::invalid_argument("Blur size must be an odd number. Value: " + std::to_string(blurSize));
        }

        std::vector<float> weights = computeWeights();

        std::vector<float> weightsLinearSampling = computeWeightsLinearSampling(weights);
        weightsTab = toShaderVectorValues(weightsLinearSampling);

        std::vector<float> offsetsLinearSampling = computeOffsetsLinearSampling(weights, weightsLinearSampling);
        offsetsTab = toShaderVectorValues(offsetsLinearSampling);
    }

    std::string GaussianBlurFilter::getShaderName() const
    {
        return "gaussianBlurTex";
    }

    void GaussianBlurFilter::completeShaderTokens(std::map<std::string, std::string> &shaderTokens) const
    {
        shaderTokens["IS_VERTICAL_BLUR"] = (blurDirection==BlurDirection::VERTICAL) ? "true" : "false";
        shaderTokens["NB_TEXTURE_FETCH"] = std::to_string(nbTextureFetch);
        shaderTokens["WEIGHTS_TAB"] = weightsTab;
        shaderTokens["OFFSETS_TAB"] = offsetsTab;
    }

    std::vector<float> GaussianBlurFilter::computeWeights() const
    {
        std::vector<unsigned int> pascalTriangleLineValues = PascalTriangle::lineValues(blurSize+3);

        //exclude outermost because the values are too insignificant and haven't enough impact on 32bits value
        std::vector<unsigned int> gaussianFactors(blurSize);
        std::copy_n(pascalTriangleLineValues.begin()+2, blurSize, gaussianFactors.begin());

        float factorsSum = std::accumulate(gaussianFactors.begin(), gaussianFactors.end(), 0.0);
        std::vector<float> weights(blurSize);
        for(unsigned int i=0; i<blurSize; ++i)
        {
            weights[i] = gaussianFactors[i]/factorsSum;
        }

        return weights;
    }

    std::vector<float> GaussianBlurFilter::computeWeightsLinearSampling(const std::vector<float> &weights) const
    {
        std::vector<float> weightsLinearSampling(nbTextureFetch);

        for(unsigned int i=0; i<nbTextureFetch; ++i)
        {
            if(i*2+1>=blurSize)
            {
                weightsLinearSampling[i] = weights[i*2];
            }else
            {
                weightsLinearSampling[i] = weights[i*2] + weights[i*2+1];
            }
        }

        return weightsLinearSampling;
    }

    std::vector<float> GaussianBlurFilter::computeOffsetsLinearSampling(const std::vector<float> &weights,
            const std::vector<float> &weightsLinearSampling) const
    {
        std::vector<float> offsetsLinearSampling(nbTextureFetch);

        int firstOffset = -(int)(std::floor((float)blurSize / 2.0f));
        for(std::size_t i=0; i < nbTextureFetch; ++i)
        {
            if(i * 2 + 1 >= blurSize)
            {
                offsetsLinearSampling[i] = firstOffset+i*2;
            }else
            {
                int offset1 = firstOffset + i * 2;
                int offset2 = offset1 + 1;
                offsetsLinearSampling[i] = ((float)offset1 * weights[i * 2] + (float)offset2 * weights[i * 2 + 1]) / weightsLinearSampling[i];
            }

            offsetsLinearSampling[i] /= static_cast<float>(textureSize);
        }

        return offsetsLinearSampling;
    }

}
