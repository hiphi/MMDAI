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

#include "vpvl2/vpvl2.h"
#include "vpvl2/internal/util.h"

#include "vpvl2/mvd/MorphKeyframe.h"
#include "vpvl2/mvd/MorphSection.h"
#include "vpvl2/mvd/NameListSection.h"

namespace vpvl2
{
namespace mvd
{

#pragma pack(push, 1)

struct MorphSecionHeader {
    int key;
    int sizeOfKeyframe;
    int countOfKeyframes;
    int reserved;
};

#pragma pack(pop)

class MorphSection::PrivateContext : public BaseSectionContext {
public:
    IMorph *morphRef;
    IMorph::WeightPrecision weight;
    PrivateContext()
        : morphRef(0),
          weight(0)
    {
    }
    ~PrivateContext() {
        morphRef = 0;
        weight = 0;
    }
    void seek(const IKeyframe::TimeIndex &timeIndex) {
        if (morphRef) {
            int fromIndex, toIndex;
            IKeyframe::TimeIndex currentTimeIndex;
            findKeyframeIndices(timeIndex, currentTimeIndex, fromIndex, toIndex);
            const MorphKeyframe *keyframeFrom = reinterpret_cast<const MorphKeyframe *>(keyframes->at(fromIndex)),
                    *keyframeTo = reinterpret_cast<const MorphKeyframe *>(keyframes->at(toIndex));
            const IKeyframe::TimeIndex &timeIndexFrom = keyframeFrom->timeIndex(), &timeIndexTo = keyframeTo->timeIndex();
            const IMorph::WeightPrecision &weightFrom = keyframeFrom->weight(), &weightTo = keyframeTo->weight();
            if (timeIndexFrom != timeIndexTo) {
                if (currentTimeIndex <= timeIndexFrom) {
                    weight = weightFrom;
                }
                else if (currentTimeIndex >= timeIndexTo) {
                    weight = weightTo;
                }
                else {
                    const IKeyframe::SmoothPrecision &w = calculateWeight(currentTimeIndex, timeIndexFrom, timeIndexTo);;
                    const Motion::InterpolationTable &tableForWeight = keyframeTo->tableForWeight();
                    if (tableForWeight.linear) {
                        weight = internal::lerp(weightFrom, weightTo, w);
                    }
                    else {
                        const IKeyframe::SmoothPrecision &weight2 = calculateInterpolatedWeight(tableForWeight, w);
                        weight = internal::lerp(weightFrom, weightTo, weight2);
                    }
                }
            }
            else {
                weight = weightFrom;
            }
            morphRef->setWeight(weight);
        }
    }
};

MorphSection::MorphSection(IModel *model, NameListSection *nameListSectionRef)
    : BaseSection(nameListSectionRef),
      m_modelRef(model),
      m_keyframePtr(0),
      m_contextPtr(0)
{
}

MorphSection::~MorphSection()
{
    release();
}

bool MorphSection::preparse(uint8_t *&ptr, size_t &rest, Motion::DataInfo &info)
{
    MorphSecionHeader header;
    if (!internal::validateSize(ptr, sizeof(header), rest)) {
        return false;
    }
    internal::getData(ptr - sizeof(header), header);
    if (!internal::validateSize(ptr, header.reserved, rest)) {
        return false;
    }
    const int nkeyframes = header.countOfKeyframes;
    const size_t reserved = header.sizeOfKeyframe - MorphKeyframe::size();
    for (int i = 0; i < nkeyframes; i++) {
        if (!MorphKeyframe::preparse(ptr, rest, reserved, info)) {
            return false;
        }
    }
    return true;
}

void MorphSection::release()
{
    BaseSection::release();
    delete m_contextPtr;
    m_contextPtr = 0;
    delete m_keyframePtr;
    m_keyframePtr = 0;
    m_allKeyframeRefs.clear();
    m_context2names.clear();
    m_name2contexts.releaseAll();
}

void MorphSection::read(const uint8_t *data)
{
    uint8_t *ptr = const_cast<uint8_t *>(data);
    MorphSecionHeader header;
    internal::getData(ptr, header);
    const size_t sizeOfKeyframe = header.sizeOfKeyframe;
    const int nkeyframes = header.countOfKeyframes;
    delete m_keyframeListPtr;
    m_keyframeListPtr = new BaseSectionContext::KeyframeCollection();
    m_keyframeListPtr->reserve(nkeyframes);
    ptr += sizeof(header) + header.reserved;
    for (int i = 0; i < nkeyframes; i++) {
        m_keyframePtr = new MorphKeyframe(m_nameListSectionRef);
        m_keyframePtr->read(ptr);
        m_keyframeListPtr->add(m_keyframePtr);
        m_allKeyframeRefs.add(m_keyframePtr);
        btSetMax(m_maxTimeIndex, m_keyframePtr->timeIndex());
        ptr += sizeOfKeyframe;
    }
    m_keyframeListPtr->sort(KeyframeTimeIndexPredication());
    delete m_contextPtr;
    m_contextPtr = new PrivateContext();
    m_contextPtr->morphRef = m_modelRef ? m_modelRef->findMorph(m_nameListSectionRef->value(header.key)) : 0;
    m_contextPtr->keyframes = m_keyframeListPtr;
    m_name2contexts.insert(header.key, m_contextPtr);
    m_context2names.insert(m_contextPtr, header.key);
    m_keyframeListPtr = 0;
    m_keyframePtr = 0;
    m_contextPtr = 0;
}

void MorphSection::seek(const IKeyframe::TimeIndex &timeIndex)
{
    if (m_modelRef) {
        m_modelRef->resetVertices();
        const int ncontexts = m_name2contexts.count();
        for (int i = 0; i < ncontexts; i++) {
            PrivateContext **context = const_cast<PrivateContext **>(m_name2contexts.value(i));
            (*context)->seek(timeIndex);
        }
    }
    saveCurrentTimeIndex(timeIndex);
}

void MorphSection::setParentModel(IModel *model)
{
    m_modelRef = model;
    if (model) {
        const int ncontexts = m_name2contexts.count();
        for (int i = 0; i < ncontexts; i++) {
            PrivateContext **context = const_cast<PrivateContext **>(m_name2contexts.value(i));
            PrivateContext *contextRef = *context;
            const int *key = m_context2names.find(contextRef);
            if (key) {
                IMorph *morph = model->findMorph(m_nameListSectionRef->value(*key));
                contextRef->morphRef = morph;
            }
            else {
                contextRef->morphRef = 0;
            }
        }
    }
}

void MorphSection::write(uint8_t *data) const
{
    const int ncontexts = m_name2contexts.count();
    for (int i = 0; i < ncontexts; i++) {
        const PrivateContext *const *context = m_name2contexts.value(i);
        const PrivateContext *contextRef = *context;
        const IMorph *morph = contextRef->morphRef;
        if (morph) {
            const PrivateContext::KeyframeCollection *keyframes = contextRef->keyframes;
            const int nkeyframes = keyframes->count();
            Motion::SectionTag tag;
            tag.type = Motion::kMorphSection;
            tag.minor = 0;
            internal::writeBytes(reinterpret_cast<const uint8_t *>(&tag), sizeof(tag), data);
            MorphSecionHeader header;
            header.countOfKeyframes = nkeyframes;
            header.key = m_nameListSectionRef->key(morph->name());
            header.reserved = 0;
            header.sizeOfKeyframe = MorphKeyframe::size();
            internal::writeBytes(reinterpret_cast<const uint8_t *>(&header) ,sizeof(header), data);
            for (int i = 0 ; i < nkeyframes; i++) {
                const IKeyframe *keyframe = keyframes->at(i);
                keyframe->write(data);
                data += keyframe->estimateSize();
            }
        }
    }
}

size_t MorphSection::estimateSize() const
{
    size_t size = 0;
    const int ncontexts = m_name2contexts.count();
    for (int i = 0; i < ncontexts; i++) {
        const PrivateContext *const *context = m_name2contexts.value(i);
        const PrivateContext *contextPtr = *context;
        if (contextPtr->morphRef) {
            const PrivateContext::KeyframeCollection *keyframes = contextPtr->keyframes;
            const int nkeyframes = keyframes->count();
            size += sizeof(Motion::SectionTag);
            size += sizeof(MorphSecionHeader);
            for (int i = 0 ; i < nkeyframes; i++) {
                const IKeyframe *keyframe = keyframes->at(i);
                size += keyframe->estimateSize();
            }
        }
    }
    return size;
}

size_t MorphSection::countKeyframes() const
{
    return m_allKeyframeRefs.count();
}

void MorphSection::addKeyframe(IKeyframe *keyframe)
{
    int key = m_nameListSectionRef->key(keyframe->name());
    PrivateContext *const *context = m_name2contexts.find(key);
    if (context) {
        PrivateContext *contextPtr = *context;
        addKeyframe0(keyframe, contextPtr->keyframes);
    }
    else {
        PrivateContext *contextPtr = m_contextPtr = new PrivateContext();
        contextPtr->morphRef = m_modelRef->findMorph(keyframe->name());
        BaseSectionContext::KeyframeCollection *kc = contextPtr->keyframes = new BaseSectionContext::KeyframeCollection();
        addKeyframe0(keyframe, kc);
        m_name2contexts.insert(key, contextPtr);
        m_context2names.insert(contextPtr, key);
        m_contextPtr = 0;
    }
}

void MorphSection::deleteKeyframe(IKeyframe *&keyframe)
{
    int key = m_nameListSectionRef->key(keyframe->name());
    PrivateContext *const *context = m_name2contexts.find(key);
    if (context) {
        PrivateContext *contextPtr = *context;
        contextPtr->keyframes->remove(keyframe);
        m_allKeyframeRefs.remove(keyframe);
        if (contextPtr->keyframes->count() == 0) {
            m_name2contexts.remove(key);
            m_context2names.remove(contextPtr);
            delete contextPtr;
        }
        delete keyframe;
        keyframe = 0;
    }
}

void MorphSection::getKeyframes(const IKeyframe::TimeIndex & /* timeIndex */,
                                const IKeyframe::LayerIndex & /* layerIndex */,
                                Array<IKeyframe *> & /* keyframes */)
{
}

IMorphKeyframe *MorphSection::findKeyframe(const IKeyframe::TimeIndex &timeIndex,
                                           const IString *name,
                                           const IKeyframe::LayerIndex &layerIndex) const
{
    PrivateContext *const *context = m_name2contexts.find(m_nameListSectionRef->key(name));
    if (context) {
        const PrivateContext::KeyframeCollection *keyframes = (*context)->keyframes;
        const int nkeyframes = keyframes->count();
        for (int i = 0; i < nkeyframes; i++) {
            IMorphKeyframe *keyframe = reinterpret_cast<IMorphKeyframe *>(keyframes->at(i));
            if (keyframe->timeIndex() == timeIndex && keyframe->layerIndex() == layerIndex) {
                return keyframe;
            }
        }
    }
    return 0;
}

IMorphKeyframe *MorphSection::findKeyframeAt(int index) const
{
    if (index >= 0 && index < m_allKeyframeRefs.count()) {
        IMorphKeyframe *keyframe = reinterpret_cast<IMorphKeyframe *>(m_allKeyframeRefs[index]);
        return keyframe;
    }
    return 0;
}

void MorphSection::addKeyframe0(IKeyframe *keyframe, BaseSectionContext::KeyframeCollection *keyframes)
{
    keyframes->add(keyframe);
    m_allKeyframeRefs.add(keyframe);
    btSetMax(m_maxTimeIndex, keyframe->timeIndex());
}


} /* namespace mvd */
} /* namespace vpvl2 */
