#ifndef URCHINENGINE_ISLANDCONTAINERTEST_H
#define URCHINENGINE_ISLANDCONTAINERTEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/Test.h>
#include <vector>

#include "UrchinPhysicsEngine.h"

class IslandContainerTest : public CppUnit::TestFixture
{
    public:
        static CppUnit::Test *suite();

        void cascadeMergeIslands();
        void mergeAllIslands();
        void createTwoSeparateIslands();
};

class TestBody : public urchin::IslandElement
{
    public:
        void setIslandElementId(unsigned int id) override
        {
            this->id = id;
        }

        unsigned int getIslandElementId() const override
        {
            return id;
        }

        bool isActive() const override
        {
            return true;
        }

    private:
        unsigned int id;
};

#endif
