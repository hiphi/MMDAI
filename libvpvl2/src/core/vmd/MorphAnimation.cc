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

#include "vpvl2/vmd/MorphAnimation.h"
#include "vpvl2/vmd/MorphKeyframe.h"

namespace
{

using namespace vpvl2;
using namespace vpvl2::vmd;

class MorphAnimationKeyframePredication
{
public:
    bool operator()(const BaseKeyframe *left, const BaseKeyframe *right) const {
        return left->timeIndex() < right->timeIndex();
    }
};

}

namespace vpvl2
{
namespace vmd
{

struct InternalMorphKeyFrameList {
    IMorph *morph;
    Array<MorphKeyframe *> keyframes;
    float weight;
    int lastIndex;

    bool isNull() const {
        if (keyframes.count() == 1) {
            const MorphKeyframe *keyFrame = keyframes[0];
            return keyFrame->weight() == 0.0f;
        }
        return false;
    }
};

MorphAnimation::MorphAnimation(IEncoding *encoding)
    : BaseAnimation(),
      m_encodingRef(encoding),
      m_modelRef(0),
      m_enableNullFrame(false)
{
}

MorphAnimation::~MorphAnimation()
{
    m_name2keyframes.releaseAll();
    m_modelRef = 0;
}

void MorphAnimation::read(const uint8_t *data, int size)
{
    uint8_t *ptr = const_cast<uint8_t *>(data);
    m_keyframes.reserve(size);
    for (int i = 0; i < size; i++) {
        MorphKeyframe *frame = new MorphKeyframe(m_encodingRef);
        m_keyframes.add(frame);
        frame->read(ptr);
        ptr += frame->estimateSize();
    }
}

void MorphAnimation::seek(const IKeyframe::TimeIndex &frameAt)
{
    if (!m_modelRef)
        return;
    const int nnodes = m_name2keyframes.count();
    m_modelRef->resetVertices();
    for (int i = 0; i < nnodes; i++) {
        InternalMorphKeyFrameList *frames = *m_name2keyframes.value(i);
        if (m_enableNullFrame && frames->isNull())
            continue;
        calculateFrames(frameAt, frames);
        IMorph *morph = frames->morph;
        morph->setWeight(frames->weight);
    }
    m_previousTimeIndex = m_currentTimeIndex;
    m_currentTimeIndex = frameAt;
}

void MorphAnimation::setParentModel(IModel *model)
{
    buildInternalNodes(model);
    m_modelRef = model;
}

void MorphAnimation::buildInternalNodes(IModel *model)
{
    if (!model)
        return;
    const int nframes = m_keyframes.count();
    m_name2keyframes.releaseAll();
    // Build internal node to find by name, not frame index
    for (int i = 0; i < nframes; i++) {
        MorphKeyframe *frame = reinterpret_cast<MorphKeyframe *>(m_keyframes.at(i));
        const IString *name = frame->name();
        const HashString &key = name->toHashString();
        InternalMorphKeyFrameList **ptr = m_name2keyframes[key], *node;
        if (ptr) {
            node = *ptr;
            node->keyframes.add(frame);
        }
        else {
            IMorph *morph = model->findMorph(name);
            if (morph) {
                node = new InternalMorphKeyFrameList();
                node->keyframes.add(frame);
                node->morph = morph;
                node->lastIndex = 0;
                node->weight = 0.0f;
                m_name2keyframes.insert(key, node);
            }
        }
    }
    // Sort frames from each internal nodes by frame index ascend
    const int nnodes = m_name2keyframes.count();
    for (int i = 0; i < nnodes; i++) {
        InternalMorphKeyFrameList *keyframes = *m_name2keyframes.value(i);
        Array<MorphKeyframe *> &frames = keyframes->keyframes;
        frames.sort(MorphAnimationKeyframePredication());
        btSetMax(m_maxTimeIndex, frames[frames.count() - 1]->timeIndex());
    }
}

void MorphAnimation::reset()
{
    BaseAnimation::reset();
    const int nnodes = m_name2keyframes.count();
    for (int i = 0; i < nnodes; i++) {
        InternalMorphKeyFrameList *node = *m_name2keyframes.value(i);
        node->lastIndex = 0;
    }
}

MorphKeyframe *MorphAnimation::frameAt(int i) const
{
    return i >= 0 && i < m_keyframes.count() ? reinterpret_cast<MorphKeyframe *>(m_keyframes[i]) : 0;
}

MorphKeyframe *MorphAnimation::findKeyframe(const IKeyframe::TimeIndex &timeIndex, const IString *name) const
{
    if (!name)
        return 0;
    const HashString &key = name->toHashString();
    InternalMorphKeyFrameList *const *ptr = m_name2keyframes.find(key);
    if (ptr) {
        const InternalMorphKeyFrameList *node = *ptr;
        const Array<MorphKeyframe *> &frames = node->keyframes;
        int index = findKeyframeIndex(timeIndex, frames);
        return index != -1 ? frames[index] : 0;
    }
    return 0;
}

void MorphAnimation::calculateFrames(const IKeyframe::TimeIndex &frameAt, InternalMorphKeyFrameList *keyFrames)
{
    Array<MorphKeyframe *> &kframes = keyFrames->keyframes;
    const int nframes = kframes.count();
    MorphKeyframe *lastKeyFrame = kframes.at(nframes - 1);
    const IKeyframe::TimeIndex &currentFrame = btMin(frameAt, lastKeyFrame->timeIndex());
    // Find the next frame index bigger than the frame index of last key frame
    int k1 = 0, k2 = 0, lastIndex = keyFrames->lastIndex;
    if (currentFrame >= kframes.at(lastIndex)->timeIndex()) {
        for (int i = lastIndex; i < nframes; i++) {
            if (currentFrame <= kframes.at(i)->timeIndex()) {
                k2 = i;
                break;
            }
        }
    }
    else {
        for (int i = 0; i <= lastIndex && i < nframes; i++) {
            if (currentFrame <= m_keyframes.at(i)->timeIndex()) {
                k2 = i;
                break;
            }
        }
    }

    if (k2 >= nframes)
        k2 = nframes - 1;
    k1 = k2 <= 1 ? 0 : k2 - 1;
    keyFrames->lastIndex = k1;

    const MorphKeyframe *keyFrameFrom = kframes.at(k1), *keyFrameTo = kframes.at(k2);
    const IKeyframe::TimeIndex &timeIndexFrom = keyFrameFrom->timeIndex(), timeIndexTo = keyFrameTo->timeIndex();
    const IMorph::WeightPrecision &weightFrom = keyFrameFrom->weight();
    const IMorph::WeightPrecision &weightTo = keyFrameTo->weight();

    if (timeIndexFrom != timeIndexTo) {
        const IKeyframe::SmoothPrecision &w = (currentFrame - timeIndexFrom) / (timeIndexTo - timeIndexFrom);
        keyFrames->weight = internal::lerp(weightFrom, weightTo, w);
    }
    else {
        keyFrames->weight = weightFrom;
    }
    m_previousTimeIndex = m_currentTimeIndex;
    m_currentTimeIndex = frameAt;
}

}
}
