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

#ifndef VPVL2_INTERNAL_UTIL_H_
#define VPVL2_INTERNAL_UTIL_H_

#include "vpvl2/config.h"
#include "vpvl2/Common.h"
#include "vpvl2/IKeyframe.h"
#include "vpvl2/IString.h"
#include <string.h>

#if defined(WIN32)
#include <windows.h>
#endif

namespace vpvl2
{
namespace internal
{

static const int kCurrentVersion = VPVL2_VERSION;
static const char *const kCurrentVersionString = VPVL2_VERSION_STRING;

static inline float spline1(const IKeyframe::SmoothPrecision &t,
                            const IKeyframe::SmoothPrecision &p1,
                            const IKeyframe::SmoothPrecision &p2)
{
    return ((1 + 3 * p1 - 3 * p2) * t * t * t + (3 * p2 - 6 * p1) * t * t + 3 * p1 * t);
}

static inline float spline2(const IKeyframe::SmoothPrecision &t,
                            const IKeyframe::SmoothPrecision &p1,
                            const IKeyframe::SmoothPrecision &p2)
{
    return ((3 + 9 * p1 - 9 * p2) * t * t + (6 * p2 - 12 * p1) * t + 3 * p1);
}

static inline IKeyframe::SmoothPrecision lerp(const IKeyframe::SmoothPrecision &x,
                                              const IKeyframe::SmoothPrecision &y,
                                              const IKeyframe::SmoothPrecision &t)
{
    return x * (1.0 - t) + y * t;
}

static inline void readBytes(size_t size, uint8_t *&ptr, size_t &rest)
{
    ptr += size;
    rest -= size;
}

static inline bool size8(uint8_t *&ptr, size_t &rest, size_t &size)
{
    assert(ptr);
    if (sizeof(uint8_t) > rest)
        return false;
    size = *reinterpret_cast<uint8_t *>(ptr);
    readBytes(sizeof(uint8_t), ptr, rest);
    return true;
}

static inline bool size16(uint8_t *&ptr, size_t &rest, size_t &size)
{
    assert(ptr);
    if (sizeof(uint16_t) > rest)
        return false;
    size = *reinterpret_cast<uint16_t *>(ptr);
    readBytes(sizeof(uint16_t), ptr, rest);
    return true;
}

static inline bool size32(uint8_t *&ptr, size_t &rest, size_t &size)
{
    assert(ptr);
    if (sizeof(int) > rest)
        return false;
    size = *reinterpret_cast<int *>(ptr);
    readBytes(sizeof(int), ptr, rest);
    return true;
}

static inline bool sizeText(uint8_t *&ptr, size_t &rest, uint8_t *&text, size_t &size)
{
    assert(ptr);
    if (!internal::size32(ptr, rest, size) || size > rest)
        return false;
    text = ptr;
    internal::readBytes(size, ptr, rest);
    return true;
}

static inline bool validateSize(uint8_t *&ptr, size_t stride, size_t size, size_t &rest)
{
    assert(ptr);
    size_t required = stride * size;
    if (required > rest)
        return false;
    internal::readBytes(required, ptr, rest);
    return true;
}

static inline bool validateSize(uint8_t *&ptr, size_t stride, size_t &rest)
{
    return validateSize(ptr, 1, stride, rest);
}

static inline int readSignedIndex(uint8_t *&ptr, size_t size)
{
    int result = 0;
    switch (size) {
    case 1:
        result = *reinterpret_cast<int8_t *>(ptr);
        ptr += sizeof(int8_t);
        break;
    case 2:
        result = *reinterpret_cast<int16_t *>(ptr);
        ptr += sizeof(int16_t);
        break;
    case 4:
        result = *reinterpret_cast<int *>(ptr);
        ptr += sizeof(int);
        break;
    default:
        assert(0);
    }
    return result;
}

static inline int readUnsignedIndex(uint8_t *&ptr, size_t size)
{
    int result = 0;
    switch (size) {
    case 1:
        result = *reinterpret_cast<uint8_t *>(ptr);
        ptr += sizeof(uint8_t);
        break;
    case 2:
        result = *reinterpret_cast<uint16_t *>(ptr);
        ptr += sizeof(uint16_t);
        break;
    case 4:
        result = *reinterpret_cast<int *>(ptr);
        ptr += sizeof(int);
        break;
    default:
        assert(0);
    }
    return result;
}

static void inline setPosition(const float *input, Vector3 &output)
{
#ifdef VPVL2_COORDINATE_OPENGL
    output.setValue(input[0], input[1], -input[2]);
#else
    setPositionRaw(input, output);
#endif
}

static void inline setPositionRaw(const float *input, Vector3 &output)
{
    output.setValue(input[0], input[1], input[2]);
}

static void inline getPosition(const Vector3 &input, float *output)
{
    output[0] = input.x();
    output[1] = input.y();
#ifdef VPVL2_COORDINATE_OPENGL
    output[2] = -input.z();
#else
    output[2] = input.z();
#endif
}

static void inline getPositionRaw(const Vector3 &input, float *output)
{
    output[0] = input.x();
    output[1] = input.y();
    output[2] = input.z();
}

static void inline setRotation(const float *input, Quaternion &output)
{
#ifdef VPVL2_COORDINATE_OPENGL
    output.setValue(input[0], -input[1], -input[2], input[3]);
#else
    output.setValue(input[0], input[1], input[2], input[3]);
#endif
}

static void inline setRotation2(const float *input, Quaternion &output)
{
#ifdef VPVL2_COORDINATE_OPENGL
    output.setValue(-input[0], -input[1], input[2], input[3]);
#else
    output.setValue(input[0], input[1], input[2], input[3]);
#endif
}

static void inline getRotation(const Quaternion &input, float *output)
{
    output[0] = input.x();
#ifdef VPVL2_COORDINATE_OPENGL
    output[1] = -input.y();
    output[2] = -input.z();
#else
    output[1] = input.y();
    output[2] = input.z();
#endif
    output[3] = input.w();
}

static void inline getRotation2(const Quaternion &input, float *output)
{
#ifdef VPVL2_COORDINATE_OPENGL
    output[0] = -input.x();
    output[1] = -input.y();
#else
    output[0] = input.x();
    output[1] = input.y();
#endif
    output[2] = input.z();
    output[3] = input.w();
}

static void inline getColor(const Vector3 &input, float *output)
{
    output[0] = input.x();
    output[1] = input.y();
    output[2] = input.z();
}

static void inline getColor(const Vector4 &input, float *output)
{
    output[0] = input.x();
    output[1] = input.y();
    output[2] = input.z();
    output[3] = input.w();
}

static inline uint8_t *copyBytes(uint8_t *dst, const uint8_t *src, size_t max)
{
    assert(dst != NULL && max > 0);
    uint8_t *ptr = static_cast<uint8_t *>(memcpy(dst, src, max));
    return ptr;
}

static inline void writeBytes(const uint8_t *src, size_t size, uint8_t *&dst)
{
    copyBytes(dst, src, size);
    dst += size;
}

static inline void writeSignedIndex(int value, size_t size, uint8_t *&dst)
{
    switch (size) {
    case 1: {
        int8_t v = value;
        writeBytes(reinterpret_cast<const uint8_t *>(&v), sizeof(v), dst);
        break;
    }
    case 2: {
        int16_t v = value;
        writeBytes(reinterpret_cast<const uint8_t *>(&v), sizeof(v), dst);
        break;
    }
    case 4: {
        int v = value;
        writeBytes(reinterpret_cast<const uint8_t *>(&v), sizeof(v), dst);
        break;
    }
    default:
        assert(0);
    }
}

static inline void writeUnsignedIndex(int value, size_t size, uint8_t *&dst)
{
    switch (size) {
    case 1: {
        uint8_t v = value;
        writeBytes(reinterpret_cast<const uint8_t *>(&v), sizeof(v), dst);
        break;
    }
    case 2: {
        uint16_t v = value;
        writeBytes(reinterpret_cast<const uint8_t *>(&v), sizeof(v), dst);
        break;
    }
    case 4: {
        int v = value;
        writeBytes(reinterpret_cast<const uint8_t *>(&v), sizeof(v), dst);
        break;
    }
    default:
        assert(0);
    }
}

static inline void writeString(const IString *string, uint8_t *&dst)
{
    size_t s = string ? string->length() : 0;
    internal::writeBytes(reinterpret_cast<const uint8_t *>(&s), sizeof(int), dst);
    if (s > 0)
        internal::writeBytes(string->toByteArray(), s, dst);
}

static inline size_t estimateSize(const IString *string)
{
    return sizeof(int) + (string ? string->length() : 0);
}

static inline void setString(const IString *newValue, IString *&value)
{
    if (newValue && newValue != value) {
        delete value;
        value = newValue->clone();
    }
}

static inline void setStringDirect(IString *newValue, IString *&value)
{
    if (newValue && newValue != value) {
        delete value;
        value = newValue;
    }
}

static inline void toggleFlag(int value, bool enable, uint16_t &flags)
{
    if (enable)
        flags |= value;
    else
        flags &= ~value;
}

template<typename T>
static inline void getData(const uint8_t *ptr, T &output)
{
#ifdef VPVL2_BUILD_IOS
    memcpy(&output, ptr, sizeof(output));
#else
    output = *reinterpret_cast<const T *>(ptr);
#endif
}

static inline void buildInterpolationTable(const IKeyframe::SmoothPrecision &x1,
                                           const IKeyframe::SmoothPrecision &x2,
                                           const IKeyframe::SmoothPrecision &y1,
                                           const IKeyframe::SmoothPrecision &y2,
                                           int size,
                                           IKeyframe::SmoothPrecision *&table)
{
    assert(table && size > 0);
    for (int i = 0; i < size; i++) {
        const IKeyframe::SmoothPrecision &in = IKeyframe::SmoothPrecision(i) / size;
        float t = in;
        while (1) {
            const IKeyframe::SmoothPrecision &v = spline1(t, x1, x2) - in;
            if (btFabs(v) < 0.0001f)
                break;
            const IKeyframe::SmoothPrecision &tt = spline2(t, x1, x2);
            if (btFuzzyZero(tt))
                break;
            t -= v / tt;
        }
        table[i] = spline1(t, y1, y2);
    }
    table[size] = 1.0f;
}

static inline void zerofill(void *ptr, size_t size)
{
    assert(ptr && size > 0);
#if defined(WIN32) && !defined(__MINGW32__)
    SecureZeroMemory(ptr, size);
#else
    memset(ptr, 0, size);
#endif
}

}
}

#endif

