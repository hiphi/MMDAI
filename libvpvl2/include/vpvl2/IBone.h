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

#ifndef VPVL2_IBONE_H_
#define VPVL2_IBONE_H_

#include "vpvl2/Common.h"

namespace vpvl2
{

class IString;

/**
 * モデルのボーンをあらわすインターフェースです。
 *
 */
class VPVL2_API IBone
{
public:
    virtual ~IBone() {}

    /**
     * ボーン名を返します。
     *
     * @return IString
     */
    virtual const IString *name() const = 0;

    /**
     * ボーンの ID を返します。
     *
     * ボーン毎にそれぞれ独立し、かつ重複しない値を返します。
     *
     * @return int
     */
    virtual int index() const = 0;

    /**
     * 親ボーンのインスタンスを返します。
     *
     * 「センター」または「全ての親」のように親ボーンが存在しない場合は 0 を返します。
     *
     * @return IBone
     */
    virtual IBone *parentBone() const = 0;

    /**
     * ボーンの接続元のインスタンスを返します。
     *
     * hasInverseKinematics が true の時のみ有効です。false の場合は常に 0 を返します。
     *
     * @return IBone
     * @sa hasInverseKinematics
     */
    virtual IBone *targetBone() const = 0;

    /**
     * ボーンのワールド変換行列を返します。
     *
     * ボーンの位置を求めるにはこれが返す値の Transform::getOrigin によって求めることが出来ます。
     * ボーンの初期位置を求めたい場合は origin を使用してください。
     *
     * @return Transform
     * @sa origin
     */
    virtual const Transform &worldTransform() const = 0;

    /**
     * ボーンの初期位置を返します。
     *
     * 返す値はボーン変形関係なく常に不変です。そのため、ボーン変形によって生じた値を
     * 求めたい場合は worldTransform を使用してください。
     *
     * @return Vector3
     * @sa worldTransform
     */
    virtual const Vector3 &origin() const = 0;

    /**
     * ボーンの接続先の位置を返します。
     *
     * 名前が origin とありますが、返す値は変形すると変化します。
     *
     * @return Vector3
     */
    virtual const Vector3 destinationOrigin() const = 0;

    /**
     * 現在のボーンの移動量を返します。
     *
     * 初期状態は vpvl2::kZeroV3 と同等です。
     *
     * @return Vector3
     * @sa setPosition
     */
    virtual const Vector3 &position() const = 0;

    /**
     * 現在のボーンの回転量を返します。
     *
     * 初期状態は Quaternion::getIdentity() と同等です。
     *
     * @return Quaternion
     * @sa setRotation
     */
    virtual const Quaternion &rotation() const = 0;

    /**
     * リンクするボーンの配列を引数に対して格納します。
     *
     * hasInverseKinematics が true の時のみ有効です。false の場合は何もしません。
     *
     * @param Array<IBone>
     * @sa hasInverseKinematics
     */
    virtual void getLinkedBones(Array<IBone *> &value) const = 0;

    /**
     * ボーンの移動量を設定します。
     *
     * @param Vector3
     * @sa position
     */
    virtual void setPosition(const Vector3 &value) = 0;

    /**
     * ボーンの回転量を設定します。
     *
     * @param Quaternion
     * @sa rotation
     */
    virtual void setRotation(const Quaternion &value) = 0;

    /**
     * ボーンが移動可能かを返します。
     *
     * @return bool
     */
    virtual bool isMovable() const = 0;

    /**
     * ボーンが回転可能かを返します。
     *
     * @return bool
     */
    virtual bool isRotateable() const = 0;

    /**
     * ボーンが表示可能かを返します。
     *
     * @return bool
     */
    virtual bool isVisible() const = 0;

    /**
     * ユーザが操作可能かを返します。
     *
     * @return bool
     */
    virtual bool isInteractive() const = 0;

    /**
     * IK を持っているかを返します。
     *
     * @return bool
     */
    virtual bool hasInverseKinematics() const = 0;

    /**
     * ボーンが固定軸を持っているかを返します。
     *
     * @return bool
     */
    virtual bool hasFixedAxes() const = 0;

    /**
     * ボーンがローカル軸を持っているかを返します。
     *
     * @return bool
     */
    virtual bool hasLocalAxes() const = 0;

    /**
     * ボーンの固定軸を返します。
     *
     * hasFixedAxes() が false の時は kZeroV3 を返します。
     *
     * @param Matrix3x3
     * @sa getLocalAxes
     */
    virtual const Vector3 &fixedAxis() const = 0;

    /**
     * ボーンのローカル軸の行列を返します。
     *
     * 返す行列は X,Y,Z の軸ベクトルで構成されます。
     * hasLocalAxes() が false の時は単位行列を返します。
     *
     * @param Matrix3x3
     * @sa getFixedAxes
     */
    virtual void getLocalAxes(Matrix3x3 &value) const = 0;
};

}

#endif
