/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2010-2012  hkrn                                    */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAI project team nor the names of     */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#ifndef VPVL2_QT_WORLD_H_
#define VPVL2_QT_WORLD_H_

#include <vpvl2/Common.h>
#include <vpvl2/IModel.h>
#include <vpvl2/Scene.h>

#include <btBulletDynamicsCommon.h>

namespace vpvl2
{
namespace qt
{

const static Vector3 kWorldAabbSize(10000, 10000, 10000);

class World
{
public:
    explicit World()
        : m_dispatcher(&m_config),
          m_world(&m_dispatcher, &m_broadphase, &m_solver, &m_config),
          m_preferredFPS(0)
    {
        setGravity(Vector3(0.0f, -9.8f, 0.0f));
        setPreferredFPS(Scene::defaultFPS());
        // m_world.getSolverInfo().m_numIterations = 10;
    }
    ~World()
    {
    }

    const Vector3 gravity() const { return m_world.getGravity(); }
    void setGravity(const Vector3 &value) { m_world.setGravity(value); }
    void setPreferredFPS(const Scalar &value) { m_preferredFPS = value; }
    void addModel(vpvl2::IModel *value) { value->joinWorld(&m_world); }
    void removeModel(vpvl2::IModel *value) { value->leaveWorld(&m_world); }
    void addRigidBody(btRigidBody *value) { m_world.addRigidBody(value); }
    void removeRigidBody(btRigidBody *value) { m_world.removeRigidBody(value); }

    void stepSimulationDefault(const Scalar &substep = 1) {
        m_world.stepSimulation(1, substep, 1.0 / m_preferredFPS);
    }
    void stepSimulationDelta(const Scalar &delta) {
        const Scalar &step = delta / m_preferredFPS;
        m_world.stepSimulation(step, 1.0 / m_preferredFPS);
    }

private:
    btDefaultCollisionConfiguration m_config;
    btCollisionDispatcher m_dispatcher;
    btDbvtBroadphase m_broadphase;
    btSequentialImpulseConstraintSolver m_solver;
    btDiscreteDynamicsWorld m_world;
    Scalar m_preferredFPS;

    VPVL2_DISABLE_COPY_AND_ASSIGN(World)
};

} /* namespace qt */
} /* namespace vpvl2 */

#endif // WORLD_H