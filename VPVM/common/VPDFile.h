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

#ifndef VPDFILE_H
#define VPDFILE_H

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <vpvl2/Common.h>

namespace vpvl2 {
class IBone;
class IEncoding;
class IModel;
}

class VPDFile
{
public:
    struct Bone
    {
        QString name;
        vpvl2::Vector3 position;
        vpvl2::Vector4 rotation;
    };
    typedef QList<Bone *> BoneList;

    /**
      * Type of parsing errors.
      */
    enum Error
    {
        kNoError,
        kInvalidHeaderError,
        kInvalidSignatureError,
        kBoneNameError,
        kPositionError,
        kQuaternionError,
        kEndError,
        kMaxErrors
    };

    VPDFile();
    ~VPDFile();

    bool load(QTextStream &stream);
    void save(QTextStream &stream);
    void makePose(vpvl2::IModel *model);
    VPDFile *clone();

    const BoneList &bones() const { return m_bones; }
    void setBones(const BoneList &value) { m_bones = value; }

private:
    BoneList m_bones;
    Error m_error;

    Q_DISABLE_COPY(VPDFile)
};

typedef QSharedPointer<VPDFile> VPDFilePtr;

#endif // VPDFILE_H
