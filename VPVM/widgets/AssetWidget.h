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

#ifndef ASSETWIDGET_H
#define ASSETWIDGET_H

#include <QtCore/QTextStream>
#include <QtCore/QUuid>
#include <QtGui/QWidget>

namespace vpvl2 {
class IBone;
class IModel;
}

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QPushButton;
class SceneLoader;

class AssetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AssetWidget(QWidget *parent = 0);
    ~AssetWidget();

    vpvl2::IModel *currentAsset() const { return m_currentAsset; }
    vpvl2::IModel *currentModel() const { return m_currentModel; }

public slots:
    void addAsset(vpvl2::IModel *asset);
    void removeAsset(vpvl2::IModel *asset);
    void addModel(vpvl2::IModel *model);
    void removeModel(vpvl2::IModel *model);
    void retranslate();

signals:
    void assetDidSelect(vpvl2::IModel *asset);
    void assetDidRemove(vpvl2::IModel *asset);

private slots:
    void removeAsset();
    void changeCurrentAsset(int index);
    void changeCurrentAsset(vpvl2::IModel *asset);
    void changeCurrentModel(int index);
    void changeParentBone(int index);
    void updatePositionX(double value);
    void updatePositionY(double value);
    void updatePositionZ(double value);
    void updateRotationX(double value);
    void updateRotationY(double value);
    void updateRotationZ(double value);
    void updateScaleFactor(double value);
    void updateOpacity(double value);
    void setAssetProperties(vpvl2::IModel *asset, SceneLoader *loader);

private:
    void setEnable(bool value);
    void updateModelBoneComboBox(vpvl2::IModel *model);
    int modelIndexOf(vpvl2::IModel *model);

    QGroupBox *m_assetGroup;
    QGroupBox *m_assignGroup;
    QGroupBox *m_positionGroup;
    QGroupBox *m_rotationGroup;
    QComboBox *m_assetComboBox;
    QComboBox *m_modelComboBox;
    QComboBox *m_modelBonesComboBox;
    QPushButton *m_removeButton;
    QDoubleSpinBox *m_px;
    QDoubleSpinBox *m_py;
    QDoubleSpinBox *m_pz;
    QDoubleSpinBox *m_rx;
    QDoubleSpinBox *m_ry;
    QDoubleSpinBox *m_rz;
    QDoubleSpinBox *m_scale;
    QDoubleSpinBox *m_opacity;
    QLabel *m_scaleLabel;
    QLabel *m_opacityLabel;
    QList<vpvl2::IModel *> m_assets;
    QList<vpvl2::IModel *> m_models;
    vpvl2::IModel *m_currentAsset;
    vpvl2::IModel *m_currentModel;

    Q_DISABLE_COPY(AssetWidget)
};

#endif // ASSETWIDGET_H
