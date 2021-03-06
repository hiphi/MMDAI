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

#include "vpvl2/IBoneKeyframe.h"
#include "vpvl2/vmd/BoneAnimation.h"
#include "vpvl2/vmd/BoneKeyframe.h"

namespace
{

using namespace vpvl2;
using namespace vpvl2::vmd;

class BoneAnimationKeyframePredication
{
public:
    bool operator()(const IBoneKeyframe *left, const IBoneKeyframe *right) const {
        return left->timeIndex() < right->timeIndex();
    }
};

}

namespace vpvl2
{
namespace vmd
{

struct BoneAnimation::InternalBoneKeyFrameList {
    IBone *bone;
    Array<BoneKeyframe *> keyframes;
    Vector3 position;
    Quaternion rotation;
    int lastIndex;

    bool isNull() const {
        if (keyframes.count() == 1) {
            const IBoneKeyframe *keyFrame = keyframes[0];
            return keyFrame->position() == kZeroV3 &&
                    keyFrame->rotation() == Quaternion::getIdentity();
        }
        return false;
    }
};

IKeyframe::SmoothPrecision BoneAnimation::weightValue(const BoneKeyframe *keyFrame,
                                                      const IKeyframe::SmoothPrecision &w,
                                                      int at)
{
    const uint16_t index = static_cast<int16_t>(w * BoneKeyframe::kTableSize);
    const IKeyframe::SmoothPrecision *v = keyFrame->interpolationTable()[at];
    return v[index] + (v[index + 1] - v[index]) * (w * BoneKeyframe::kTableSize - index);
}

void BoneAnimation::lerpVector3(const BoneKeyframe *keyFrame,
                                const Vector3 &from,
                                const Vector3 &to,
                                const IKeyframe::SmoothPrecision &w,
                                int at,
                                IKeyframe::SmoothPrecision &value)
{
    const IKeyframe::SmoothPrecision &valueFrom = from[at];
    const IKeyframe::SmoothPrecision &valueTo = to[at];
    if (keyFrame->linear()[at]) {
        value = internal::lerp(valueFrom, valueTo, w);
    }
    else {
        const IKeyframe::SmoothPrecision &w2 = weightValue(keyFrame, w, at);
        value = internal::lerp(valueFrom, valueTo, w2);
    }
}

BoneAnimation::BoneAnimation(IEncoding *encoding)
    : BaseAnimation(),
      m_encodingRef(encoding),
      m_modelRef(0),
      m_enableNullFrame(false)
{
}

BoneAnimation::~BoneAnimation()
{
    m_name2keyframes.releaseAll();
    m_modelRef = 0;
}

void BoneAnimation::read(const uint8_t *data, int size)
{
    uint8_t *ptr = const_cast<uint8_t *>(data);
    m_keyframes.reserve(size);
    for (int i = 0; i < size; i++) {
        BoneKeyframe *frame = new BoneKeyframe(m_encodingRef);
        m_keyframes.add(frame);
        frame->read(ptr);
        ptr += frame->estimateSize();
    }
}

void BoneAnimation::seek(const IKeyframe::TimeIndex &frameAt)
{
    if (!m_modelRef)
        return;
    const int nnodes = m_name2keyframes.count();
    for (int i = 0; i < nnodes; i++) {
        InternalBoneKeyFrameList *keyframes = *m_name2keyframes.value(i);
        if (m_enableNullFrame && keyframes->isNull())
            continue;
        calculateFrames(frameAt, keyframes);
        IBone *bone = keyframes->bone;
        bone->setPosition(keyframes->position);
        bone->setRotation(keyframes->rotation);
    }
    m_previousTimeIndex = m_currentTimeIndex;
    m_currentTimeIndex = frameAt;
}

void BoneAnimation::setParentModel(IModel *model)
{
    buildInternalKeyFrameList(model);
    m_modelRef = model;
}

BoneKeyframe *BoneAnimation::frameAt(int i) const
{
    return i >= 0 && i < m_keyframes.count() ? reinterpret_cast<BoneKeyframe *>(m_keyframes[i]) : 0;
}

BoneKeyframe *BoneAnimation::findKeyframe(const IKeyframe::TimeIndex &timeIndex, const IString *name) const
{
    if (!name)
        return 0;
    const HashString &key = name->toHashString();
    InternalBoneKeyFrameList *const *ptr = m_name2keyframes.find(key);
    if (ptr) {
        const InternalBoneKeyFrameList *node = *ptr;
        const Array<BoneKeyframe *> &frames = node->keyframes;
        int index = findKeyframeIndex(timeIndex, frames);
        return index != -1 ? frames[index] : 0;
    }
    return 0;
}

void BoneAnimation::buildInternalKeyFrameList(IModel *model)
{
    if (!model)
        return;
    const int nframes = m_keyframes.count();
    m_name2keyframes.releaseAll();
    // Build internal node to find by name, not frame index
    for (int i = 0; i < nframes; i++) {
        BoneKeyframe *frame = reinterpret_cast<BoneKeyframe *>(m_keyframes.at(i));
        const IString *name = frame->name();
        const HashString &key = name->toHashString();
        InternalBoneKeyFrameList **ptr = m_name2keyframes[key], *node;
        if (ptr) {
            node = *ptr;
            node->keyframes.add(frame);
        }
        else {
            IBone *bone = model->findBone(name);
            if (bone) {
                node = new InternalBoneKeyFrameList();
                node->keyframes.add(frame);
                node->bone = bone;
                node->lastIndex = 0;
                node->position.setZero();
                node->rotation.setValue(0.0f, 0.0f, 0.0f, 1.0f);
                m_name2keyframes.insert(key, node);
            }
        }
    }
    // Sort frames from each internal nodes by frame index ascend
    const int nnodes = m_name2keyframes.count();
    for (int i = 0; i < nnodes; i++) {
        InternalBoneKeyFrameList *node = *m_name2keyframes.value(i);
        Array<BoneKeyframe *> &frames = node->keyframes;
        frames.sort(BoneAnimationKeyframePredication());
        btSetMax(m_maxTimeIndex, frames[frames.count() - 1]->timeIndex());
    }
}

void BoneAnimation::calculateFrames(const IKeyframe::TimeIndex &frameAt, InternalBoneKeyFrameList *keyFrames)
{
    Array<BoneKeyframe *> &keyframes = keyFrames->keyframes;
    const int nframes = keyframes.count();
    IBoneKeyframe *lastKeyFrame = keyframes[nframes - 1];
    const IKeyframe::TimeIndex &currentFrame = btMin(frameAt, lastKeyFrame->timeIndex());
    // Find the next frame index bigger than the frame index of last key frame
    int k1 = 0, k2 = 0, lastIndex = keyFrames->lastIndex;
    if (currentFrame >= keyframes[lastIndex]->timeIndex()) {
        for (int i = lastIndex; i < nframes; i++) {
            if (currentFrame <= keyframes[i]->timeIndex()) {
                k2 = i;
                break;
            }
        }
    }
    else {
        for (int i = 0; i <= lastIndex && i < nframes; i++) {
            if (currentFrame <= keyframes[i]->timeIndex()) {
                k2 = i;
                break;
            }
        }
    }

    if (k2 >= nframes)
        k2 = nframes - 1;
    k1 = k2 <= 1 ? 0 : k2 - 1;
    keyFrames->lastIndex = k1;

    const BoneKeyframe *keyFrameFrom = keyframes.at(k1),
            *keyFrameTo = keyframes.at(k2);
    const IKeyframe::TimeIndex &timeIndexFrom = keyFrameFrom->timeIndex(), timeIndexTo = keyFrameTo->timeIndex();
    BoneKeyframe *keyFrameForInterpolation = const_cast<BoneKeyframe *>(keyFrameTo);
    const Vector3 &positionFrom = keyFrameFrom->position();
    const Quaternion &rotationFrom = keyFrameFrom->rotation();
    const Vector3 &positionTo = keyFrameTo->position();
    const Quaternion &rotationTo = keyFrameTo->rotation();

    if (timeIndexFrom != timeIndexTo) {
        if (currentFrame <= timeIndexFrom) {
            keyFrames->position = positionFrom;
            keyFrames->rotation = rotationFrom;
        }
        else if (currentFrame >= timeIndexTo) {
            keyFrames->position = positionTo;
            keyFrames->rotation = rotationTo;
        }
        else {
            const IKeyframe::SmoothPrecision &w = (currentFrame - timeIndexFrom) / (timeIndexTo - timeIndexFrom);
            IKeyframe::SmoothPrecision x = 0, y = 0, z = 0;
            lerpVector3(keyFrameForInterpolation, positionFrom, positionTo, w, 0, x);
            lerpVector3(keyFrameForInterpolation, positionFrom, positionTo, w, 1, y);
            lerpVector3(keyFrameForInterpolation, positionFrom, positionTo, w, 2, z);
            keyFrames->position.setValue(x, y, z);
            if (keyFrameForInterpolation->linear()[3]) {
                keyFrames->rotation = rotationFrom.slerp(rotationTo, w);
            }
            else {
                const IKeyframe::SmoothPrecision &w2 = weightValue(keyFrameForInterpolation, w, 3);
                keyFrames->rotation = rotationFrom.slerp(rotationTo, w2);
            }
        }
    }
    else {
        keyFrames->position = positionFrom;
        keyFrames->rotation = rotationFrom;
    }
}

void BoneAnimation::reset()
{
    BaseAnimation::reset();
    const int nnodes = m_name2keyframes.count();
    for (int i = 0; i < nnodes; i++) {
        InternalBoneKeyFrameList *keyframes = *m_name2keyframes.value(i);
        keyframes->lastIndex = 0;
    }
}

}
}
