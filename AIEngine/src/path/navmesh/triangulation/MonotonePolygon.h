#ifndef URCHINENGINE_MONOTONEPOLYGON_H
#define URCHINENGINE_MONOTONEPOLYGON_H

#include <cstdint>
#include <vector>
#include <set>

namespace urchin
{

    class MonotonePolygon
    {
        public:
            void setCcwPoints(const std::vector<std::size_t> &);
            const std::vector<std::size_t> &getCcwPoints() const;

            void addSharedEdge(unsigned int, unsigned int);
            const std::set<uint_fast64_t> &getSharedEdges() const;
            bool isSharedEdge(unsigned int, unsigned int) const;

        private:
            uint_fast64_t computeEdgeId(unsigned int, unsigned int) const;

            std::vector<std::size_t> ccwPoints;
            std::set<uint_fast64_t> sharedEdges;
    };

}

#endif
