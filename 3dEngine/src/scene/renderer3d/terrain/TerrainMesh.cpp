#include <thread>
#include <functional>
#include <fstream>

#include "TerrainMesh.h"
#include "resources/MediaManager.h"

#define FRL_FILE_EXTENSION ".frl" //Extension for FRL files (Fast Resource Loading)
#define TERRAIN_FRL_FILE_VERSION 1

namespace urchin
{

    TerrainMesh::TerrainMesh(const std::string &heightFilename, float xzScale, float yScale) :
            heightFilename(heightFilename),
            xzScale(xzScale),
            yScale(yScale)
    {
        auto *imgTerrain = MediaManager::instance()->getMedia<Image>(heightFilename);
        if(imgTerrain->getImageFormat() != Image::IMAGE_GRAYSCALE)
        {
            imgTerrain->release();
            throw std::runtime_error("Height map must be grayscale. Image format: " + std::to_string(imgTerrain->getImageFormat()));
        }
        
        xSize = imgTerrain->getWidth();
        zSize = imgTerrain->getHeight();

        std::string terrainFilePath = FileSystem::instance()->getResourcesDirectory() + imgTerrain->getName();
        std::string terrainFrlFilePath = FileSystem::instance()->getSaveDirectory() + FileHandler::getFileNameNoExtension(terrainFilePath) + FRL_FILE_EXTENSION;
        std::string terrainMd5Sum = std::string(MD5().digestFile(terrainFilePath.c_str()));

        std::ifstream terrainFrlFile;
        terrainFrlFile.open(terrainFrlFilePath, std::ios::in | std::ios::binary);

        if(terrainFrlFile.is_open() && readVersion(terrainFrlFile)==TERRAIN_FRL_FILE_VERSION && readMd5(terrainFrlFile)==terrainMd5Sum)
        {
            loadTerrainMeshFile(terrainFrlFile);
        } else
        {
            terrainFrlFile.close();

            buildVertices(imgTerrain);
            buildIndices();
            buildNormals();

            writeTerrainMeshFile(terrainFrlFilePath, terrainMd5Sum);
        }

        heightfieldPointHelper = std::make_unique<HeightfieldPointHelper<float>>(vertices, xSize);

        imgTerrain->release();
    }

    const std::string &TerrainMesh::getHeightFilename() const
    {
        return heightFilename;
    }

    float TerrainMesh::getXZScale() const
    {
        return xzScale;
    }

    float TerrainMesh::getYScale() const
    {
        return yScale;
    }

    unsigned int TerrainMesh::getXSize() const
    {
        return xSize;
    }

    unsigned int TerrainMesh::getZSize() const
    {
        return zSize;
    }

    /**
     * @return Terrain local vertices. First point is the far left point (min X, min Z) and the last pint the near right point (max X, max Z).
     */
    const std::vector<Point3<float>> &TerrainMesh::getVertices() const
    {
        return vertices;
    }

    const std::vector<Vector3<float>> &TerrainMesh::getNormals() const
    {
        return normals;
    }

    const std::vector<unsigned int> &TerrainMesh::getIndices() const
    {
        return indices;
    }

    Point3<float> TerrainMesh::findPointAt(const Point2<float> &xzCoordinate) const
    {
        return heightfieldPointHelper->findPointAt(xzCoordinate);
    }

    float TerrainMesh::findHeightAt(const Point2<float> &xzCoordinate) const
    {
        return heightfieldPointHelper->findHeightAt(xzCoordinate);
    }

    unsigned int TerrainMesh::computeNumberVertices() const
    {
        return xSize * zSize;
    }

    unsigned int TerrainMesh::computeNumberIndices() const
    {
        return ((zSize-1) * xSize * 2) + (zSize-1);
    }

    unsigned int TerrainMesh::computeNumberNormals() const
    {
        return xSize * zSize;
    }

    std::vector<Point3<float>> TerrainMesh::buildVertices(const Image *imgTerrain)
    {
        vertices.reserve(computeNumberVertices());

        float xStart = (-((float)xSize * xzScale) / 2.0f) + (xzScale / 2.0f);
        float zStart = (-((float)zSize * xzScale) / 2.0f) + (xzScale / 2.0f);

        for(unsigned int z=0; z<zSize; ++z)
        {
            float zFloat = zStart + (float)z * xzScale;
            for (unsigned int x = 0; x < xSize; ++x)
            {
                float elevation = 0.0f;
                if(imgTerrain->getChannelPrecision()==Image::CHANNEL_8)
                {
                    elevation = (float)imgTerrain->getTexels()[x + xSize * z] * yScale;
                }else if(imgTerrain->getChannelPrecision()==Image::CHANNEL_16)
                {
                    constexpr float scale16BitsTo8Bits = 255.0f / 65535.0f;
                    elevation = (float)imgTerrain->getTexels16Bits()[x + xSize * z] * scale16BitsTo8Bits * yScale;
                }

                float xFloat = xStart + (float)x * xzScale;
                vertices.emplace_back(Point3<float>(xFloat, elevation, zFloat));
            }
        }

        return vertices;
    }

    std::vector<unsigned int> TerrainMesh::buildIndices()
    {
        indices.reserve(computeNumberIndices());

        for(unsigned int z = 0; z < zSize - 1; ++z)
        {
            for (unsigned int x = 0; x < xSize; ++x)
            {
                indices.push_back(x + xSize * (z + 1));
                indices.push_back(x + xSize * z);
            }

            indices.push_back(RESTART_INDEX);
        }
        return indices;
    }

    std::vector<Vector3<float>> TerrainMesh::buildNormals()
    {
        const unsigned int NUM_THREADS = std::max(2u, std::thread::hardware_concurrency());

        //1. compute normal of triangles
        unsigned int totalTriangles = ((zSize - 1) * (xSize - 1)) * 2;
        unsigned int xLineQuantity = (xSize * 2) + 1;
        std::vector<Vector3<float>> normalTriangles;
        normalTriangles.resize(totalTriangles);
        unsigned int numLoopNormalTriangle = indices.size() - 2;
        std::vector<std::thread> threadsNormalTriangle(NUM_THREADS);
        for(unsigned int threadI=0; threadI<NUM_THREADS; threadI++)
        {
            unsigned int beginI = threadI * numLoopNormalTriangle / NUM_THREADS;
            unsigned int endI = (threadI + 1)==NUM_THREADS ? numLoopNormalTriangle : (threadI + 1) * numLoopNormalTriangle / NUM_THREADS;
            threadsNormalTriangle[threadI] = std::thread(std::bind([&](unsigned int beginI, unsigned int endI)
            {
                for(unsigned int i = beginI; i<endI; i++)
                {
                    if(indices[i+2] != RESTART_INDEX)
                    {
                        Point3<float> point1 = vertices[indices[i]];
                        Point3<float> point2 = vertices[indices[i+1]];
                        Point3<float> point3 = vertices[indices[i+2]];

                        bool isCwTriangle = (i % xLineQuantity) % 2 == 0;
                        Vector3<float> normal;
                        if(isCwTriangle)
                        {
                            normal = (point1.vector(point2).crossProduct(point3.vector(point1)));
                        }else
                        {
                            normal = (point1.vector(point2).crossProduct(point1.vector(point3)));
                        }

                        unsigned int normalTriangleIndex = i - ((i / xLineQuantity) * 3);
                        normalTriangles[normalTriangleIndex] = normal.normalize();
                    }else
                    {
                        i += 2;
                    }
                }
            }, beginI, endI));
        }
        std::for_each(threadsNormalTriangle.begin(), threadsNormalTriangle.end(), [](std::thread& x){x.join();});
        assert(totalTriangles == normalTriangles.size());

        //2. compute normal of vertex
        normals.resize(computeNumberNormals());
        unsigned int numLoopNormalVertex = vertices.size();
        std::vector<std::thread> threadsNormalVertex(NUM_THREADS);
        for(unsigned int threadI=0; threadI<NUM_THREADS; threadI++)
        {
            unsigned int beginI = threadI * numLoopNormalVertex / NUM_THREADS;
            unsigned int endI = (threadI + 1)==NUM_THREADS ? numLoopNormalVertex : (threadI + 1) * numLoopNormalVertex / NUM_THREADS;

            threadsNormalVertex[threadI] = std::thread(std::bind([&](unsigned int beginI, unsigned int endI)
            {
                for(unsigned int i = beginI; i<endI; i++)
                {
                    Vector3<float> vertexNormal(0.0, 0.0, 0.0);
                    for(unsigned int triangleIndex : findTriangleIndices(i))
                    {
                        vertexNormal += normalTriangles[triangleIndex];
                    }
                    normals[i] = vertexNormal.normalize();
                }
            }, beginI, endI));
        }
        std::for_each(threadsNormalVertex.begin(), threadsNormalVertex.end(), [](std::thread& x){x.join();});

        return normals;
    }

    std::vector<unsigned int> TerrainMesh::findTriangleIndices(unsigned int vertexIndex) const
    {
        std::vector<unsigned int> triangleIndices;
        triangleIndices.reserve(6);

        unsigned int rowNum = vertexIndex / xSize;
        long squareIndex = vertexIndex - xSize - rowNum;

        bool isLeftBorderVertex = (vertexIndex % xSize) == 0;
        bool isRightBorderVertex = ((vertexIndex + 1) % xSize) == 0;
        bool isFirstRowVertex = rowNum == 0;
        bool isLastRowVertex = rowNum == (zSize - 1);

        //above triangles to the vertex
        if(!isFirstRowVertex)
        {
            long firstLeftTopTriangle = (squareIndex * 2) + 1;

            //left triangle
            if (!isLeftBorderVertex)
            {
                triangleIndices.emplace_back(firstLeftTopTriangle);
            }

            //right triangles
            if (!isRightBorderVertex)
            {
                triangleIndices.emplace_back(firstLeftTopTriangle + 1);
                triangleIndices.emplace_back(firstLeftTopTriangle + 2);
            }
        }

        //below triangles to the vertex
        if(!isLastRowVertex)
        {
            long firstLeftBottomTriangle = (squareIndex * 2) + (xSize - 1) * 2;

            //left triangles
            if (!isLeftBorderVertex)
            {
                triangleIndices.emplace_back(firstLeftBottomTriangle);
                triangleIndices.emplace_back(firstLeftBottomTriangle + 1);
            }

            //right triangle
            if (!isRightBorderVertex)
            {
                triangleIndices.emplace_back(firstLeftBottomTriangle + 2);
            }
        }

        return triangleIndices;
    }

    void TerrainMesh::writeTerrainMeshFile(const std::string &filePath, const std::string &md5) const
    {
        std::ofstream file;
        file.open(filePath, std::ios::out | std::ios::binary | std::ios::trunc);

        writeVersion(file, TERRAIN_FRL_FILE_VERSION);
        writeMd5(file, md5);

        file.write(reinterpret_cast<const char*>(&vertices[0]), vertices.size()*sizeof(float)*3);
        file.write(reinterpret_cast<const char*>(&indices[0]), indices.size()*sizeof(unsigned int));
        file.write(reinterpret_cast<const char*>(&normals[0]), normals.size()*sizeof(float)*3);

        file.close();
    }

    void TerrainMesh::writeVersion(std::ofstream &file, unsigned int version) const
    {
        file.write(reinterpret_cast<char*>(&version), sizeof(version));
    }

    void TerrainMesh::writeMd5(std::ofstream &file, const std::string &md5) const
    {
        file.write(md5.c_str(), md5.size()*sizeof(char));
    }

    void TerrainMesh::loadTerrainMeshFile(std::ifstream &file)
    {
        vertices.resize(computeNumberVertices());
        file.read(reinterpret_cast<char*>(&vertices[0]), vertices.size()*sizeof(float)*3);

        indices.resize(computeNumberIndices());
        file.read(reinterpret_cast<char*>(&indices[0]), indices.size()*sizeof(unsigned int));

        normals.resize(computeNumberNormals());
        file.read(reinterpret_cast<char*>(&normals[0]), normals.size()*sizeof(float)*3);

        file.close();
    }

    unsigned int TerrainMesh::readVersion(std::ifstream &file) const
    {
        unsigned int version;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        return version;
    }

    std::string TerrainMesh::readMd5(std::ifstream &file) const
    {
        constexpr unsigned int md5Size = 32;
        char md5[md5Size];
        file.read(&md5[0], sizeof(md5));
        return std::string(md5, md5Size);
    }

}
