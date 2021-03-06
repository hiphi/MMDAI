/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2009-2011  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                2010-2012  hkrn                                    */
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

#include "vpvl2/vpvl2.h"
#include "vpvl2/internal/util.h"

#include "vpvl2/vmd/CameraAnimation.h"
#include "vpvl2/vmd/CameraKeyframe.h"

namespace
{

using namespace vpvl2;
using namespace vpvl2::vmd;

class CameraAnimationKeyFramePredication
{
public:
    bool operator()(const IKeyframe *left, const IKeyframe *right) const {
        return left->timeIndex() < right->timeIndex();
    }
};

}

namespace vpvl2
{
namespace vmd
{

IKeyframe::SmoothPrecision CameraAnimation::weightValue(const CameraKeyframe *keyFrame,
                                                        const IKeyframe::SmoothPrecision &w,
                                                        int at)
{
    const uint16_t index = static_cast<int16_t>(w * CameraKeyframe::kTableSize);
    const IKeyframe::SmoothPrecision *v = keyFrame->interpolationTable()[at];
    return v[index] + (v[index + 1] - v[index]) * (w * CameraKeyframe::kTableSize - index);
}

void CameraAnimation::lerpVector3(const CameraKeyframe *keyFrame,
                                  const Vector3 &from,
                                  const Vector3 &to,
                                  const IKeyframe::SmoothPrecision &w,
                                  int at,
                                  IKeyframe::SmoothPrecision &value)
{
    const IKeyframe::SmoothPrecision &valueFrom = from[at];
    const IKeyframe::SmoothPrecision &valueTo = to[at];
    if (keyFrame->linear()[at]) {
        value = valueFrom * (1 - w) + valueTo * w;
    }
    else {
        const IKeyframe::SmoothPrecision &w2 = weightValue(keyFrame, w, at);
        value = valueFrom * (1 - w2) + valueTo * w2;
    }
}

CameraAnimation::CameraAnimation()
    : BaseAnimation(),
      m_position(0.0f, 0.0f, 0.0f),
      m_angle(0.0f, 0.0f, 0.0f),
      m_distance(0.0f),
      m_fovy(0.0f)
{
}

CameraAnimation::~CameraAnimation()
{
    m_position.setZero();
    m_angle.setZero();
    m_distance = 0.0f;
    m_fovy = 0.0f;
}

void CameraAnimation::read(const uint8_t *data, int size)
{
    if (size > 0) {
        uint8_t *ptr = const_cast<uint8_t *>(data);
        m_keyframes.reserve(size);
        for (int i = 0; i < size; i++) {
            CameraKeyframe *frame = new CameraKeyframe();
            m_keyframes.add(frame);
            frame->read(ptr);
            ptr += frame->estimateSize();
        }
        update();
    }
}

void CameraAnimation::seek(const IKeyframe::TimeIndex &frameAt)
{
    const int nframes = m_keyframes.count();
    CameraKeyframe *lastKeyFrame = reinterpret_cast<CameraKeyframe *>(m_keyframes[nframes - 1]);
    IKeyframe::TimeIndex currentFrame = btMin(frameAt, lastKeyFrame->timeIndex());
    // Find the next frame index bigger than the frame index of last key frame
    int k1 = 0, k2 = 0;
    if (currentFrame >= m_keyframes[m_lastTimeIndex]->timeIndex()) {
        for (int i = m_lastTimeIndex; i < nframes; i++) {
            if (currentFrame <= m_keyframes[i]->timeIndex()) {
                k2 = i;
                break;
            }
        }
    }
    else {
        for (int i = 0; i <= m_lastTimeIndex && i < nframes; i++) {
            if (currentFrame <= m_keyframes[i]->timeIndex()) {
                k2 = i;
                break;
            }
        }
    }

    if (k2 >= nframes)
        k2 = nframes - 1;
    k1 = k2 <= 1 ? 0 : k2 - 1;
    m_lastTimeIndex = k1;

    const CameraKeyframe *keyFrameFrom = this->frameAt(k1), *keyFrameTo = this->frameAt(k2);
    CameraKeyframe *keyFrameForInterpolation = const_cast<CameraKeyframe *>(keyFrameTo);
    float timeIndexFrom = keyFrameFrom->timeIndex(), timeIndexTo = keyFrameTo->timeIndex();
    float distanceFrom = keyFrameFrom->distance(), fovyFrom = keyFrameFrom->fov();
    Vector3 positionFrom = keyFrameFrom->position(), angleFrom = keyFrameFrom->angle();
    float distanceTo = keyFrameTo->distance(), fovyTo = keyFrameTo->fov();
    Vector3 positionTo = keyFrameTo->position(), angleTo = keyFrameTo->angle();
    if (timeIndexFrom != timeIndexTo) {
        if (currentFrame <= timeIndexFrom) {
            m_distance = distanceFrom;
            m_position = positionFrom;
            m_angle = angleFrom;
            m_fovy = fovyFrom;
        }
        else if (currentFrame >= timeIndexTo) {
            m_distance = distanceTo;
            m_position = positionTo;
            m_angle = angleTo;
            m_fovy = fovyTo;
        }
        else if (timeIndexTo - timeIndexFrom <= 1.0f) {
            m_distance = distanceFrom;
            m_position = positionFrom;
            m_angle = angleFrom;
            m_fovy = fovyFrom;
        }
        else {
            const IKeyframe::SmoothPrecision &w = (currentFrame - timeIndexFrom) / (timeIndexTo - timeIndexFrom);
            IKeyframe::SmoothPrecision x = 0, y = 0, z = 0;
            lerpVector3(keyFrameForInterpolation, positionFrom, positionTo, w, 0, x);
            lerpVector3(keyFrameForInterpolation, positionFrom, positionTo, w, 1, y);
            lerpVector3(keyFrameForInterpolation, positionFrom, positionTo, w, 2, z);
            m_position.setValue(x, y, z);
            if (keyFrameForInterpolation->linear()[3]) {
                m_angle = angleFrom.lerp(angleTo, w);
            }
            else {
                const IKeyframe::SmoothPrecision &w2 = weightValue(keyFrameForInterpolation, w, 3);
                m_angle = angleFrom.lerp(angleTo, w2);
            }
            if (keyFrameForInterpolation->linear()[4]) {
                m_distance = internal::lerp(distanceFrom, distanceTo, w);
            }
            else {
                const IKeyframe::SmoothPrecision &w2 = weightValue(keyFrameForInterpolation, w, 4);
                m_distance = internal::lerp(distanceFrom, distanceTo, w2);
            }
            if (keyFrameForInterpolation->linear()[5]) {
                m_fovy = internal::lerp(fovyFrom, fovyTo, w);
            }
            else {
                const IKeyframe::SmoothPrecision &w2 = weightValue(keyFrameForInterpolation, w, 5);
                m_fovy = internal::lerp(fovyFrom, fovyTo, w2);
            }
        }
    }
    else {
        m_distance = distanceFrom;
        m_position = positionFrom;
        m_angle = angleFrom;
        m_fovy = fovyFrom;
    }
    m_previousTimeIndex = m_currentTimeIndex;
    m_currentTimeIndex = frameAt;
}

void CameraAnimation::update()
{
    int nkeyframes = m_keyframes.count();
    if (nkeyframes > 0) {
        m_keyframes.sort(CameraAnimationKeyFramePredication());
        m_maxTimeIndex = m_keyframes[m_keyframes.count() - 1]->timeIndex();
    }
    else {
        m_maxTimeIndex = 0;
    }
}

CameraKeyframe *CameraAnimation::findKeyframe(const IKeyframe::TimeIndex &timeIndex) const
{
    int index = findKeyframeIndex(timeIndex, m_keyframes);
    return index != -1 ? reinterpret_cast<CameraKeyframe *>(m_keyframes[index]) : 0;
}

CameraKeyframe *CameraAnimation::frameAt(int i) const
{
    return i >= 0 && i < m_keyframes.count() ? reinterpret_cast<CameraKeyframe *>(m_keyframes[i]) : 0;
}

}
}
