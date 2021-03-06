#include "collision/constraintsolver/ConstraintSolverManager.h"

namespace urchin
{

    ConstraintSolverManager::ConstraintSolverManager() :
            constraintSolverIteration(ConfigService::instance()->getUnsignedIntValue("constraintSolver.constraintSolverIteration")),
            biasFactor(ConfigService::instance()->getFloatValue("constraintSolver.biasFactor")),
            useWarmStarting(ConfigService::instance()->getBoolValue("constraintSolver.useWarmStarting")),
            restitutionVelocityThreshold(ConfigService::instance()->getFloatValue("constraintSolver.restitutionVelocityThreshold"))
    {
        unsigned int constraintSolvingPoolSize = ConfigService::instance()->getUnsignedIntValue("constraintSolver.constraintSolvingPoolSize");
        constraintSolvingPool = new FixedSizePool<ConstraintSolving>("constraintSolvingPool", sizeof(ConstraintSolving), constraintSolvingPoolSize);
    }

    ConstraintSolverManager::~ConstraintSolverManager()
    {
        for (auto &constraintSolving : constraintsSolving)
        {
            constraintSolvingPool->free(constraintSolving);
        }

        delete constraintSolvingPool;
    }

    /**
     * Solve constraints
     * @param dt Delta of time (sec.) between two simulation steps
     * @param manifoldResults Constraints to solve
     */
    void ConstraintSolverManager::solveConstraints(float dt, std::vector<ManifoldResult> &manifoldResults)
    {
        ScopeProfiler profiler("physics", "solveConstraint");

        //setup step to solve constraints
        setupConstraints(manifoldResults, dt);

        //iterative constraint solver
        for(unsigned int i=0; i<constraintSolverIteration; ++i)
        {
            solveConstraints();
        }
    }

    void ConstraintSolverManager::setupConstraints(std::vector<ManifoldResult> &manifoldResults, float dt)
    { //See http://en.wikipedia.org/wiki/Collision_response for formulas

        //clear constraints solving
        for (auto &constraintSolving : constraintsSolving)
        {
            constraintSolvingPool->free(constraintSolving);
        }
        constraintsSolving.clear();

        //setup constraints solving
        for (auto &manifoldResult : manifoldResults)
        {
            for(unsigned int j=0; j< manifoldResult.getNumContactPoints(); ++j)
            {
                ManifoldContactPoint &contact = manifoldResult.getManifoldContactPoint(j);
                if(contact.getDepth() > 0.0 && !contact.isPredictive())
                {
                    continue;
                }

                WorkRigidBody *body1 = WorkRigidBody::upCast(manifoldResult.getBody1());
                WorkRigidBody *body2 = WorkRigidBody::upCast(manifoldResult.getBody2());
                void *memPtr = constraintSolvingPool->allocate(sizeof(ConstraintSolving));
                auto *constraintSolving = new(memPtr) ConstraintSolving(body1, body2, contact);

                const CommonSolvingData &commonSolvingData = fillCommonSolvingData(manifoldResult, contact);
                constraintSolving->setCommonData(commonSolvingData);

                const ImpulseSolvingData &impulseSolvingData = fillImpulseSolvingData(commonSolvingData, dt);
                constraintSolving->setImpulseData(impulseSolvingData);

                if(useWarmStarting)
                {
                    const Vector3<float> normalImpulseVector = contact.getAccumulatedSolvingData().accNormalImpulse * commonSolvingData.contactNormal;
                    applyImpulse(constraintSolving->getBody1(), constraintSolving->getBody2(), commonSolvingData, normalImpulseVector);

                    const Vector3<float> tangentImpulseVector = contact.getAccumulatedSolvingData().accTangentImpulse * commonSolvingData.contactTangent;
                    applyImpulse(constraintSolving->getBody1(), constraintSolving->getBody2(), commonSolvingData, tangentImpulseVector);
                }

                constraintsSolving.push_back(constraintSolving);
            }
        }
    }

    void ConstraintSolverManager::solveConstraints()
    {
        //solve tangent constraint first because non-penetration is more important than friction
        for (auto &constraintSolving : constraintsSolving)
        {
            solveTangentConstraint(constraintSolving);
        }

        //solve normal constraint
        for (auto &constraintSolving : constraintsSolving)
        {
            solveNormalConstraint(constraintSolving);
        }
    }

    CommonSolvingData ConstraintSolverManager::fillCommonSolvingData(const ManifoldResult &manifoldResult, const ManifoldContactPoint &contact)
    {
        CommonSolvingData commonSolvingData;

        WorkRigidBody *body1 = WorkRigidBody::upCast(manifoldResult.getBody1());
        WorkRigidBody *body2 = WorkRigidBody::upCast(manifoldResult.getBody2());

        commonSolvingData.body1 = body1;
        commonSolvingData.body2 = body2;

        commonSolvingData.invInertia1 = body1->getInvWorldInertia();
        commonSolvingData.invInertia2 = body2->getInvWorldInertia();
        commonSolvingData.r1 = body1->getPosition().vector(contact.getPointOnObject2());
        commonSolvingData.r2 = body2->getPosition().vector(contact.getPointOnObject2());

        commonSolvingData.depth = contact.getDepth();
        commonSolvingData.contactNormal = contact.getNormalFromObject2();
        commonSolvingData.contactTangent = computeTangent(commonSolvingData, contact.getNormalFromObject2());

        return commonSolvingData;
    }

    ImpulseSolvingData ConstraintSolverManager::fillImpulseSolvingData(const CommonSolvingData &commonData, float dt) const
    {
        ImpulseSolvingData impulseSolvingData;

        //friction
        impulseSolvingData.friction = std::sqrt(commonData.body1->getFriction() * commonData.body2->getFriction());

        //impulses
        impulseSolvingData.normalImpulseDenominator = commonData.body1->getInvMass() + commonData.body2->getInvMass() +
                (commonData.invInertia1 * (commonData.r1.crossProduct(commonData.contactNormal)).crossProduct(commonData.r1) +
                commonData.invInertia2 * (commonData.r2.crossProduct(commonData.contactNormal)).crossProduct(commonData.r2)
                ).dotProduct(commonData.contactNormal);

        if(impulseSolvingData.normalImpulseDenominator == 0.0)
        {
            logCommonData("Invalid normal impulse denominator", commonData);
        }

        impulseSolvingData.tangentImpulseDenominator = commonData.body1->getInvMass() + commonData.body2->getInvMass() +
                (commonData.invInertia1 * (commonData.r1.crossProduct(commonData.contactTangent)).crossProduct(commonData.r1) +
                commonData.invInertia2 * (commonData.r2.crossProduct(commonData.contactTangent)).crossProduct(commonData.r2)
                ).dotProduct(commonData.contactTangent);

        if(impulseSolvingData.tangentImpulseDenominator == 0.0)
        {
            logCommonData("Invalid tangent impulse denominator", commonData);
        }

        //bias
        float invDeltaTime = dt > 0.0f ? 1.0f / dt : 0.0f;
        float restitution = std::max(commonData.body1->getRestitution(), commonData.body2->getRestitution());

        float normalRelativeVelocity = computeRelativeVelocity(commonData).dotProduct(commonData.contactNormal);
        float depthBias = biasFactor * invDeltaTime * commonData.depth;
        float restitutionBias = 0.0f;
        if(normalRelativeVelocity > restitutionVelocityThreshold)
        {
            restitutionBias =  -restitution * normalRelativeVelocity;
        }
        impulseSolvingData.bias = std::min(depthBias, restitutionBias);

        return impulseSolvingData;
    }

    /**
     * Solve normal constraint. Normal constraint is related to non-penetration
     */
    void ConstraintSolverManager::solveNormalConstraint(ConstraintSolving *constraintSolving)
    {
        const CommonSolvingData &commonSolvingData = constraintSolving->getCommonData();
        const ImpulseSolvingData &impulseSolvingData = constraintSolving->getImpulseData();
        AccumulatedSolvingData &accumulatedSolvingData = constraintSolving->getAccumulatedData();

        float normalRelativeVelocity = computeRelativeVelocity(commonSolvingData).dotProduct(commonSolvingData.contactNormal);

        float normalImpulseNumerator = (-normalRelativeVelocity) + impulseSolvingData.bias;
        float normalImpulse = normalImpulseNumerator / impulseSolvingData.normalImpulseDenominator;

        float oldAccNormalImpulse = accumulatedSolvingData.accNormalImpulse;
        accumulatedSolvingData.accNormalImpulse = std::min(oldAccNormalImpulse + normalImpulse, 0.0f);
        normalImpulse = accumulatedSolvingData.accNormalImpulse - oldAccNormalImpulse;

        const Vector3<float> normalImpulseVector = normalImpulse * commonSolvingData.contactNormal;
        applyImpulse(constraintSolving->getBody1(), constraintSolving->getBody2(), commonSolvingData, normalImpulseVector);
    }

    /**
     * Solve tangent constraint. Tangent constraint is related to friction
     */
    void ConstraintSolverManager::solveTangentConstraint(ConstraintSolving *constraintSolving)
    {
        const CommonSolvingData &commonSolvingData = constraintSolving->getCommonData();
        const ImpulseSolvingData &impulseSolvingData = constraintSolving->getImpulseData();
        AccumulatedSolvingData &accumulatedSolvingData = constraintSolving->getAccumulatedData();

        float tangentRelativeVelocity = computeRelativeVelocity(commonSolvingData).dotProduct(commonSolvingData.contactTangent);

        float tangentImpulseNumerator = -tangentRelativeVelocity;
        float tangentImpulse = tangentImpulseNumerator / impulseSolvingData.tangentImpulseDenominator;
        float maxFriction = -(impulseSolvingData.friction * accumulatedSolvingData.accNormalImpulse);

        float oldAccTangentImpulse = accumulatedSolvingData.accTangentImpulse;
        accumulatedSolvingData.accTangentImpulse = MathAlgorithm::clamp(oldAccTangentImpulse + tangentImpulse, -maxFriction, maxFriction);
        tangentImpulse = accumulatedSolvingData.accTangentImpulse - oldAccTangentImpulse;

        const Vector3<float> tangentImpulseVector = tangentImpulse * commonSolvingData.contactTangent;
        applyImpulse(constraintSolving->getBody1(), constraintSolving->getBody2(), commonSolvingData, tangentImpulseVector);
    }

    void ConstraintSolverManager::applyImpulse(WorkRigidBody *body1, WorkRigidBody *body2, const CommonSolvingData &commonData, const Vector3<float> &impulseVector)
    {
        body1->setLinearVelocity(body1->getLinearVelocity() - (impulseVector * body1->getInvMass() * body1->getLinearFactor()));
        body1->setAngularVelocity(body1->getAngularVelocity() - (commonData.invInertia1 * commonData.r1.crossProduct(impulseVector * body1->getLinearFactor()) * body1->getAngularFactor()));

        body2->setLinearVelocity(body2->getLinearVelocity() + (impulseVector * body2->getInvMass() * body2->getLinearFactor()));
        body2->setAngularVelocity(body2->getAngularVelocity() + (commonData.invInertia2 * commonData.r2.crossProduct(impulseVector * body2->getLinearFactor()) * body2->getAngularFactor()));
    }

    /**
     * @return Relative velocity at the contact point
     */
    Vector3<float> ConstraintSolverManager::computeRelativeVelocity(const CommonSolvingData &commonData) const
    {
        const Vector3<float> vp1 = commonData.body1->getLinearVelocity() + commonData.body1->getAngularVelocity().crossProduct(commonData.r1);
        const Vector3<float> vp2 = commonData.body2->getLinearVelocity() + commonData.body2->getAngularVelocity().crossProduct(commonData.r2);

        return vp2 - vp1;
    }

    /**
     * @return Tangent vector in direction of sliding
     */
    Vector3<float> ConstraintSolverManager::computeTangent(const CommonSolvingData &commonData, const Vector3<float> &normal) const
    {
        Vector3<float> relativeVelocity = computeRelativeVelocity(commonData);
        Vector3<float> tangentVelocity = relativeVelocity - (relativeVelocity.dotProduct(normal)) * normal;

        float tangentVelocityLength = tangentVelocity.length();
        if(tangentVelocityLength==0.0)
        {
            return Vector3<float>(0.0, 0.0, 0.0);
        }

        return tangentVelocity / tangentVelocityLength;
    }

    void ConstraintSolverManager::logCommonData(const std::string &message, const CommonSolvingData &commonData) const
    {
        std::stringstream logStream;
        logStream.precision(std::numeric_limits<float>::max_digits10);

        logStream << message << std::endl;
        logStream << "Common data:" << std::endl;
        logStream << " - Body 1 inverse mass: " << commonData.body1->getInvMass() << std::endl;
        logStream << " - Body 2 inverse mass: " << commonData.body2->getInvMass() << std::endl;
        logStream << " - Inverse inertia 1: " << std::endl << commonData.invInertia1 << std::endl;
        logStream << " - Inverse inertia 2: " << std::endl << commonData.invInertia2 << std::endl;
        logStream << " - R1: " << commonData.r1 << std::endl;
        logStream << " - R2: " << commonData.r2 << std::endl;
        logStream << " - Contact normal: " << commonData.contactNormal << std::endl;
        logStream << " - Contact tangent: " << commonData.contactTangent;

        Logger::logger().logError(logStream.str());
    }
}
