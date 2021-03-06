#include "Common.h"

#include "vpvl2/pmx/Model.h"
#include "vpvl2/vmd/BoneAnimation.h"
#include "vpvl2/vmd/BoneKeyframe.h"
#include "vpvl2/vmd/CameraAnimation.h"
#include "vpvl2/vmd/CameraKeyframe.h"
#include "vpvl2/vmd/LightAnimation.h"
#include "vpvl2/vmd/LightKeyframe.h"
#include "vpvl2/vmd/MorphAnimation.h"
#include "vpvl2/vmd/MorphKeyframe.h"
#include "vpvl2/vmd/Motion.h"

#include "mock/Bone.h"
#include "mock/Model.h"
#include "mock/Morph.h"

using namespace vpvl2::pmx;

namespace
{

const char *kTestString = "012345678901234";

static void CompareBoneInterpolationMatrix(const QuadWord p[], const vmd::BoneKeyframe &frame)
{
    QuadWord actual, expected = p[0];
    frame.getInterpolationParameter(vmd::BoneKeyframe::kX, actual);
    ASSERT_TRUE(testVector(expected, actual));
    expected = p[1];
    frame.getInterpolationParameter(vmd::BoneKeyframe::kY, actual);
    ASSERT_TRUE(testVector(expected, actual));
    expected = p[2];
    frame.getInterpolationParameter(vmd::BoneKeyframe::kZ, actual);
    ASSERT_TRUE(testVector(expected, actual));
    expected = p[3];
    frame.getInterpolationParameter(vmd::BoneKeyframe::kRotation, actual);
    ASSERT_TRUE(testVector(expected, actual));
}

static void CompareCameraInterpolationMatrix(const QuadWord p[], const vmd::CameraKeyframe &frame)
{
    QuadWord actual, expected = p[0];
    frame.getInterpolationParameter(vmd::CameraKeyframe::kX, actual);
    ASSERT_TRUE(testVector(expected, actual));
    expected = p[1];
    frame.getInterpolationParameter(vmd::CameraKeyframe::kY, actual);
    ASSERT_TRUE(testVector(expected, actual));
    expected = p[2];
    frame.getInterpolationParameter(vmd::CameraKeyframe::kZ, actual);
    ASSERT_TRUE(testVector(expected, actual));
    expected = p[3];
    frame.getInterpolationParameter(vmd::CameraKeyframe::kRotation, actual);
    ASSERT_TRUE(testVector(expected, actual));
    expected = p[4];
    frame.getInterpolationParameter(vmd::CameraKeyframe::kDistance, actual);
    ASSERT_TRUE(testVector(expected, actual));
    expected = p[5];
    frame.getInterpolationParameter(vmd::CameraKeyframe::kFov, actual);
    ASSERT_TRUE(testVector(expected, actual));
}

}

TEST(VMDMotionTest, ParseEmpty)
{
    Encoding encoding;
    Model model(&encoding);
    vmd::Motion motion(&model, &encoding);
    vmd::Motion::DataInfo info;
    // parsing empty should be error
    ASSERT_FALSE(motion.preparse(reinterpret_cast<const uint8_t *>(""), 0, info));
    ASSERT_EQ(vmd::Motion::kInvalidHeaderError, motion.error());
}

TEST(VMDMotionTest, ParseFile)
{
    QFile file("motion.vmd");
    if (file.open(QFile::ReadOnly)) {
        QByteArray bytes = file.readAll();
        const uint8_t *data = reinterpret_cast<const uint8_t *>(bytes.constData());
        size_t size = bytes.size();
        Encoding encoding;
        Model model(&encoding);
        vmd::Motion motion(&model, &encoding);
        vmd::Motion::DataInfo result;
        // valid model motion should be loaded successfully
        ASSERT_TRUE(motion.preparse(data, size, result));
        ASSERT_TRUE(motion.load(data, size));
        ASSERT_EQ(size_t(motion.boneAnimation().countKeyframes()), result.boneKeyframeCount);
        ASSERT_EQ(size_t(motion.cameraAnimation().countKeyframes()), result.cameraKeyframeCount);
        ASSERT_EQ(size_t(motion.morphAnimation().countKeyframes()), result.morphKeyframeCount);
        ASSERT_EQ(vmd::Motion::kNoError, motion.error());
    }
}

TEST(VMDMotionTest, ParseCamera)
{
    QFile file("camera.vmd");
    if (file.open(QFile::ReadOnly)) {
        QByteArray bytes = file.readAll();
        const uint8_t *data = reinterpret_cast<const uint8_t *>(bytes.constData());
        size_t size = bytes.size();
        Encoding encoding;
        Model model(&encoding);
        vmd::Motion motion(&model, &encoding);
        vmd::Motion::DataInfo result;
        // valid camera motion should be loaded successfully
        ASSERT_TRUE(motion.preparse(data, size, result));
        ASSERT_TRUE(motion.load(data, size));
        ASSERT_EQ(size_t(motion.boneAnimation().countKeyframes()), result.boneKeyframeCount);
        ASSERT_EQ(size_t(motion.cameraAnimation().countKeyframes()), result.cameraKeyframeCount);
        ASSERT_EQ(size_t(motion.morphAnimation().countKeyframes()), result.morphKeyframeCount);
        ASSERT_EQ(vmd::Motion::kNoError, motion.error());
    }
}

TEST(VMDMotionTest, SaveBoneKeyframe)
{
    Encoding encoding;
    CString str(kTestString);
    vmd::BoneKeyframe frame(&encoding), newFrame(&encoding);
    Vector3 pos(1, 2, 3);
    Quaternion rot(4, 5, 6, 7);
    // initialize the bone frame to be copied
    frame.setTimeIndex(42);
    frame.setName(&str);
    frame.setPosition(pos);
    frame.setRotation(rot);
    QuadWord px(8, 9, 10, 11),
            py(12, 13, 14, 15),
            pz(16, 17, 18, 19),
            pr(20, 21, 22, 23);
    QuadWord p[] = { px, py, pz, pr };
    frame.setInterpolationParameter(vmd::BoneKeyframe::kX, px);
    frame.setInterpolationParameter(vmd::BoneKeyframe::kY, py);
    frame.setInterpolationParameter(vmd::BoneKeyframe::kZ, pz);
    frame.setInterpolationParameter(vmd::BoneKeyframe::kRotation, pr);
    // write a bone frame to data and read it
    uint8_t data[vmd::BoneKeyframe::strideSize()];
    frame.write(data);
    newFrame.read(data);
    // compare read bone frame
    ASSERT_TRUE(newFrame.name()->equals(frame.name()));
    ASSERT_EQ(frame.timeIndex(), newFrame.timeIndex());
    ASSERT_TRUE(newFrame.position() == pos);
    ASSERT_TRUE(newFrame.rotation() == rot);
    CompareBoneInterpolationMatrix(p, frame);
    // cloned bone frame shold be copied with deep
    QScopedPointer<IBoneKeyframe> cloned(frame.clone());
    ASSERT_TRUE(cloned->name()->equals(frame.name()));
    ASSERT_EQ(frame.timeIndex(), cloned->timeIndex());
    ASSERT_TRUE(cloned->position() == pos);
    ASSERT_TRUE(cloned->rotation() == rot);
    CompareBoneInterpolationMatrix(p, *static_cast<vmd::BoneKeyframe *>(cloned.data()));
}

TEST(VMDMotionTest, SaveCameraKeyframe)
{
    vmd::CameraKeyframe frame, newFrame;
    Vector3 pos(1, 2, 3), angle(4, 5, 6);
    // initialize the camera frame to be copied
    frame.setTimeIndex(42);
    frame.setPosition(pos);
    frame.setAngle(angle);
    frame.setDistance(7);
    frame.setFov(8);
    QuadWord px(9, 10, 11, 12),
            py(13, 14, 15, 16),
            pz(17, 18, 19, 20),
            pr(21, 22, 23, 24),
            pd(25, 26, 27, 28),
            pf(29, 30, 31, 32);
    QuadWord p[] = { px, py, pz, pr, pd, pf };
    frame.setInterpolationParameter(vmd::CameraKeyframe::kX, px);
    frame.setInterpolationParameter(vmd::CameraKeyframe::kY, py);
    frame.setInterpolationParameter(vmd::CameraKeyframe::kZ, pz);
    frame.setInterpolationParameter(vmd::CameraKeyframe::kRotation, pr);
    frame.setInterpolationParameter(vmd::CameraKeyframe::kDistance, pd);
    frame.setInterpolationParameter(vmd::CameraKeyframe::kFov, pf);
    // write a camera frame to data and read it
    uint8_t data[vmd::CameraKeyframe::strideSize()];
    frame.write(data);
    newFrame.read(data);
    ASSERT_EQ(frame.timeIndex(), newFrame.timeIndex());
    ASSERT_TRUE(newFrame.position() == frame.position());
    // compare read camera frame
    // for radian and degree calculation
    ASSERT_TRUE(qFuzzyCompare(newFrame.angle().x(), frame.angle().x()));
    ASSERT_TRUE(qFuzzyCompare(newFrame.angle().y(), frame.angle().y()));
    ASSERT_TRUE(qFuzzyCompare(newFrame.angle().z(), frame.angle().z()));
    ASSERT_TRUE(newFrame.distance() == frame.distance());
    ASSERT_TRUE(newFrame.fov() == frame.fov());
    CompareCameraInterpolationMatrix(p, frame);
    // cloned camera frame shold be copied with deep
    QScopedPointer<ICameraKeyframe> cloned(frame.clone());
    ASSERT_EQ(frame.timeIndex(), cloned->timeIndex());
    ASSERT_TRUE(cloned->position() == frame.position());
    // for radian and degree calculation
    ASSERT_TRUE(qFuzzyCompare(cloned->angle().x(), frame.angle().x()));
    ASSERT_TRUE(qFuzzyCompare(cloned->angle().y(), frame.angle().y()));
    ASSERT_TRUE(qFuzzyCompare(cloned->angle().z(), frame.angle().z()));
    ASSERT_TRUE(cloned->distance() == frame.distance());
    ASSERT_TRUE(cloned->fov() == frame.fov());
    CompareCameraInterpolationMatrix(p, *static_cast<vmd::CameraKeyframe *>(cloned.data()));
}

TEST(VMDMotionTest, SaveMorphKeyframe)
{
    Encoding encoding;
    CString str(kTestString);
    vmd::MorphKeyframe frame(&encoding), newFrame(&encoding);
    // initialize the morph frame to be copied
    frame.setName(&str);
    frame.setTimeIndex(42);
    frame.setWeight(0.5);
    // write a morph frame to data and read it
    uint8_t data[vmd::MorphKeyframe::strideSize()];
    frame.write(data);
    newFrame.read(data);
    // compare read morph frame
    ASSERT_TRUE(newFrame.name()->equals(frame.name()));
    ASSERT_EQ(frame.timeIndex(), newFrame.timeIndex());
    ASSERT_EQ(frame.weight(), newFrame.weight());
    // cloned morph frame shold be copied with deep
    QScopedPointer<IMorphKeyframe> cloned(frame.clone());
    ASSERT_TRUE(cloned->name()->equals(frame.name()));
    ASSERT_EQ(frame.timeIndex(), cloned->timeIndex());
    ASSERT_EQ(frame.weight(), cloned->weight());
}

TEST(VMDMotionTest, SaveLightKeyframe)
{
    vmd::LightKeyframe frame, newFrame;
    Vector3 color(0.1, 0.2, 0.3), direction(4, 5, 6);
    // initialize the light frame to be copied
    frame.setTimeIndex(42);
    frame.setColor(color);
    frame.setDirection(direction);
    // write a light frame to data and read it
    uint8_t data[vmd::LightKeyframe::strideSize()];
    frame.write(data);
    newFrame.read(data);
    // compare read light frame
    ASSERT_EQ(frame.timeIndex(), newFrame.timeIndex());
    ASSERT_TRUE(newFrame.color() == frame.color());
    ASSERT_TRUE(newFrame.direction() == frame.direction());
    // cloned morph frame shold be copied with deep
    QScopedPointer<ILightKeyframe> cloned(frame.clone());
    ASSERT_EQ(frame.timeIndex(), cloned->timeIndex());
    ASSERT_TRUE(cloned->color() == frame.color());
    ASSERT_TRUE(cloned->direction() == frame.direction());
}

TEST(VMDMotionTest, SaveMotion)
{
    QFile file("motion.vmd");
    if (file.open(QFile::ReadOnly)) {
        QByteArray bytes = file.readAll();
        const uint8_t *data = reinterpret_cast<const uint8_t *>(bytes.constData());
        size_t size = bytes.size();
        Encoding encoding;
        Model model(&encoding);
        vmd::Motion motion(&model, &encoding);
        motion.load(data, size);
        size_t newSize = motion.estimateSize();
        QScopedArrayPointer<uint8_t> newData(new uint8_t[newSize]);
        motion.save(newData.data());
        // just compare written size
        ASSERT_EQ(size, newSize);
    }
}

TEST(VMDMotionTest, CloneMotion)
{
    QFile file("motion.vmd");
    if (file.open(QFile::ReadOnly)) {
        QByteArray bytes = file.readAll();
        const uint8_t *data = reinterpret_cast<const uint8_t *>(bytes.constData());
        size_t size = bytes.size();
        Encoding encoding;
        Model model(&encoding);
        vmd::Motion motion(&model, &encoding);
        motion.load(data, size);
        QByteArray bytes2(motion.estimateSize(), 0);
        motion.save(reinterpret_cast<uint8_t *>(bytes2.data()));
        QScopedPointer<IMotion> motion2(motion.clone());
        QByteArray bytes3(motion2->estimateSize(), 0);
        motion2->save(reinterpret_cast<uint8_t *>(bytes3.data()));
        ASSERT_STREQ(bytes2.constData(), bytes3.constData());
    }
}

TEST(VMDMotionTest, ParseBoneKeyframe)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.writeRawData(kTestString, vmd::BoneKeyframe::kNameSize);
    stream << quint32(1)                   // frame index
           << 2.0f << 3.0f << 4.0f         // position
           << 5.0f << 6.0f << 7.0f << 8.0f // rotation
              ;
    for (int i = 0; i < vmd::BoneKeyframe::kTableSize; i++)
        stream << quint8(0);
    ASSERT_EQ(vmd::BoneKeyframe::strideSize(), size_t(bytes.size()));
    Encoding encoding;
    vmd::BoneKeyframe frame(&encoding);
    CString str(kTestString);
    frame.read(reinterpret_cast<const uint8_t *>(bytes.constData()));
    ASSERT_TRUE(frame.name()->equals(&str));
    ASSERT_EQ(IKeyframe::TimeIndex(1.0), frame.timeIndex());
#ifdef VPVL2_COORDINATE_OPENGL
    ASSERT_TRUE(frame.position() == Vector3(2.0f, 3.0f, -4.0f));
    ASSERT_TRUE(frame.rotation() == Quaternion(-5.0f, -6.0f, 7.0f, 8.0f));
#else
    ASSERT_TRUE(frame.position() == Vector3(2.0f, 3.0f, 4.0f));
    ASSERT_TRUE(frame.rotation() == Quaternion(5.0f, 6.0f, 7.0f, 8.0f));
#endif
}

TEST(VMDMotionTest, ParseCameraKeyframe)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint32(1)           // frame index
           << 1.0f                 // distance
           << 2.0f << 3.0f << 4.0f // position
           << 5.0f << 6.0f << 7.0f // angle
              ;
    for (int i = 0; i < vmd::CameraKeyframe::kTableSize; i++)
        stream << quint8(0);
    stream << quint32(8)           // view angle (fovy)
           << quint8(1)            // no perspective
              ;
    ASSERT_EQ(vmd::CameraKeyframe::strideSize(), size_t(bytes.size()));
    vmd::CameraKeyframe frame;
    frame.read(reinterpret_cast<const uint8_t *>(bytes.constData()));
    ASSERT_EQ(IKeyframe::TimeIndex(1.0), frame.timeIndex());
#ifdef VPVL2_COORDINATE_OPENGL
    ASSERT_EQ(-1.0f, frame.distance());
    ASSERT_TRUE(frame.position() == Vector3(2.0f, 3.0f, -4.0f));
    ASSERT_TRUE(frame.angle() == Vector3(-degree(5.0f), -degree(6.0f), degree(7.0f)));
#else
    ASSERT_EQ(1.0f, frame.distance());
    ASSERT_TRUE(frame.position() == Vector3(2.0f, 3.0f, 4.0f));
    ASSERT_TRUE(frame.angle() == Vector3(degree(5.0f), degree(6.0f), degree(7.0f)));
#endif
    ASSERT_EQ(8.0f, frame.fov());
    // TODO: perspective flag
}

TEST(VMDMotionTest, ParseMorphKeyframe)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.writeRawData(kTestString, vmd::MorphKeyframe::kNameSize);
    stream << quint32(1) // frame index
           << 0.5f       // weight
              ;
    ASSERT_EQ(vmd::MorphKeyframe::strideSize(), size_t(bytes.size()));
    Encoding encoding;
    vmd::MorphKeyframe frame(&encoding);
    CString str(kTestString);
    frame.read(reinterpret_cast<const uint8_t *>(bytes.constData()));
    ASSERT_TRUE(frame.name()->equals(&str));
    ASSERT_EQ(IKeyframe::TimeIndex(1.0), frame.timeIndex());
    ASSERT_EQ(IMorph::WeightPrecision(0.5), frame.weight());
}

TEST(VMDMotionTest, ParseLightKeyframe)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint32(1)           // frame index
           << 0.2f << 0.3f << 0.4f // color
           << 0.5f << 0.6f << 0.7f // direction
              ;
    ASSERT_EQ(vmd::LightKeyframe::strideSize(), size_t(bytes.size()));
    vmd::LightKeyframe frame;
    frame.read(reinterpret_cast<const uint8_t *>(bytes.constData()));
    ASSERT_EQ(IKeyframe::TimeIndex(1.0), frame.timeIndex());
    ASSERT_TRUE(frame.color() == Vector3(0.2f, 0.3f, 0.4f));
#ifdef VPVL2_COORDINATE_OPENGL
    ASSERT_TRUE(frame.direction() == Vector3(0.5f, 0.6f, -0.7f));
#else
    ASSERT_TRUE(frame.direction() == Vector3(0.5f, 0.6f, 0.7f));
#endif
}

TEST(VMDMotionTest, BoneInterpolation)
{
    Encoding encoding;
    vmd::BoneKeyframe frame(&encoding);
    QuadWord n;
    frame.getInterpolationParameter(vmd::BoneKeyframe::kX, n);
    ASSERT_TRUE(testVector(QuadWord(0.0f, 0.0f, 0.0f, 0.0f), n));
    QuadWord px(8, 9, 10, 11),
            py(12, 13, 14, 15),
            pz(16, 17, 18, 19),
            pr(20, 21, 22, 23);
    QuadWord p[] = { px, py, pz, pr };
    frame.setInterpolationParameter(vmd::BoneKeyframe::kX, px);
    frame.setInterpolationParameter(vmd::BoneKeyframe::kY, py);
    frame.setInterpolationParameter(vmd::BoneKeyframe::kZ, pz);
    frame.setInterpolationParameter(vmd::BoneKeyframe::kRotation, pr);
    CompareBoneInterpolationMatrix(p, frame);
}

TEST(VMDMotionTest, CameraInterpolation)
{
    vmd::CameraKeyframe frame;
    QuadWord n;
    frame.getInterpolationParameter(vmd::CameraKeyframe::kX, n);
    ASSERT_TRUE(testVector(QuadWord(0.0f, 0.0f, 0.0f, 0.0f), n));
    QuadWord px(9, 10, 11, 12),
            py(13, 14, 15, 16),
            pz(17, 18, 19, 20),
            pr(21, 22, 23, 24),
            pd(25, 26, 27, 28),
            pf(29, 30, 31, 32);
    QuadWord p[] = { px, py, pz, pr, pd, pf };
    frame.setInterpolationParameter(vmd::CameraKeyframe::kX, px);
    frame.setInterpolationParameter(vmd::CameraKeyframe::kY, py);
    frame.setInterpolationParameter(vmd::CameraKeyframe::kZ, pz);
    frame.setInterpolationParameter(vmd::CameraKeyframe::kRotation, pr);
    frame.setInterpolationParameter(vmd::CameraKeyframe::kDistance, pd);
    frame.setInterpolationParameter(vmd::CameraKeyframe::kFov, pf);
    CompareCameraInterpolationMatrix(p, frame);
}

TEST(VMDMotionTest, AddAndRemoveBoneKeyframes)
{
    Encoding encoding;
    CString name("bone");
    MockIModel model;
    MockIBone bone;
    vmd::Motion motion(&model, &encoding);
    ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kBone));
    // mock bone
    EXPECT_CALL(model, findBone(_)).Times(AtLeast(1)).WillRepeatedly(Return(&bone));
    QScopedPointer<IBoneKeyframe> keyframePtr(new vmd::BoneKeyframe(&encoding));
    keyframePtr->setTimeIndex(42);
    keyframePtr->setName(&name);
    {
        // The frame that the layer index is not zero should not be added
        QScopedPointer<IBoneKeyframe> frame42(new vmd::BoneKeyframe(&encoding));
        frame42->setLayerIndex(42);
        motion.addKeyframe(frame42.data());
        motion.update(IKeyframe::kBone);
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kBone));
    }
    {
        // add a bone keyframe (don't forget updating motion!)
        motion.addKeyframe(keyframePtr.data());
        IKeyframe *keyframe = keyframePtr.take();
        motion.update(IKeyframe::kBone);
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kBone));
        // boudary check of findBoneKeyframeAt
        ASSERT_EQ(static_cast<IBoneKeyframe *>(0), motion.findBoneKeyframeAt(-1));
        ASSERT_EQ(keyframe, motion.findBoneKeyframeAt(0));
        ASSERT_EQ(static_cast<IBoneKeyframe *>(0), motion.findBoneKeyframeAt(1));
        // layer index 0 must be used
        ASSERT_EQ(1, motion.countLayers(&name, IKeyframe::kBone));
        ASSERT_EQ(0, motion.findMorphKeyframe(42, &name, 1));
        // find a bone keyframe with timeIndex and name
        ASSERT_EQ(keyframe, motion.findBoneKeyframe(42, &name, 0));
    }
    keyframePtr.reset(new vmd::BoneKeyframe(&encoding));
    keyframePtr->setTimeIndex(42);
    keyframePtr->setName(&name);
    {
        // replaced bone frame should be one keyframe (don't forget updating motion!)
        motion.replaceKeyframe(keyframePtr.data());
        IKeyframe *keyframeToDelete = keyframePtr.take();
        motion.update(IKeyframe::kBone);
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kBone));
        // no longer be find previous bone keyframe
        ASSERT_EQ(keyframeToDelete, motion.findBoneKeyframe(42, &name, 0));
        // delete bone keyframe and set it null (don't forget updating motion!)
        motion.deleteKeyframe(keyframeToDelete);
        motion.update(IKeyframe::kBone);
        // bone keyframes should be empty
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kBone));
        ASSERT_EQ(static_cast<IBoneKeyframe *>(0), motion.findBoneKeyframe(42, &name, 0));
        ASSERT_EQ(static_cast<IKeyframe *>(0), keyframeToDelete);
    }
}

TEST(VMDMotionTest, AddAndRemoveCameraKeyframes)
{
    Encoding encoding;
    Model model(&encoding);
    vmd::Motion motion(&model, &encoding);
    ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kCamera));
    QScopedPointer<ICameraKeyframe> keyframePtr(new vmd::CameraKeyframe());
    keyframePtr->setTimeIndex(42);
    keyframePtr->setDistance(42);
    {
        // The frame that the layer index is not zero should not be added
        QScopedPointer<ICameraKeyframe> frame42(new vmd::CameraKeyframe());
        frame42->setLayerIndex(42);
        motion.addKeyframe(frame42.data());
        motion.update(IKeyframe::kCamera);
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kCamera));
    }
    {
        // add a camera keyframe (don't forget updating motion!)
        motion.addKeyframe(keyframePtr.data());
        IKeyframe *keyframe = keyframePtr.take();
        motion.update(IKeyframe::kCamera);
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kCamera));
        // boudary check of findCameraKeyframeAt
        ASSERT_EQ(static_cast<ICameraKeyframe *>(0), motion.findCameraKeyframeAt(-1));
        ASSERT_EQ(keyframe, motion.findCameraKeyframeAt(0));
        ASSERT_EQ(static_cast<ICameraKeyframe *>(0), motion.findCameraKeyframeAt(1));
        // layer index 0 must be used
        ASSERT_EQ(1, motion.countLayers(0, IKeyframe::kCamera));
        ASSERT_EQ(0, motion.findCameraKeyframe(42, 1));
        // find a camera keyframe with timeIndex
        ASSERT_EQ(keyframe, motion.findCameraKeyframe(42, 0));
    }
    keyframePtr.reset(new vmd::CameraKeyframe());
    keyframePtr->setTimeIndex(42);
    keyframePtr->setDistance(84);
    {
        // replaced camera frame should be one keyframe (don't forget updating motion!)
        motion.replaceKeyframe(keyframePtr.data());
        IKeyframe *keyframeToDelete = keyframePtr.take();
        motion.update(IKeyframe::kCamera);
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kCamera));
        // no longer be find previous camera keyframe
        ASSERT_EQ(84.0f, motion.findCameraKeyframe(42, 0)->distance());
        // delete camera keyframe and set it null (don't forget updating motion!)
        motion.deleteKeyframe(keyframeToDelete);
        motion.update(IKeyframe::kCamera);
        // camera keyframes should be empty
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kCamera));
        ASSERT_EQ(static_cast<ICameraKeyframe *>(0), motion.findCameraKeyframe(42, 0));
        ASSERT_EQ(static_cast<IKeyframe *>(0), keyframeToDelete);
    }
}

TEST(VMDMotionTest, AddAndRemoveLightKeyframes)
{
    Encoding encoding;
    Model model(&encoding);
    vmd::Motion motion(&model, &encoding);
    ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kLight));
    QScopedPointer<ILightKeyframe> keyframePtr(new vmd::LightKeyframe());
    keyframePtr->setTimeIndex(42);
    keyframePtr->setColor(Vector3(1, 0, 0));
    {
        // The frame that the layer index is not zero should not be added
        QScopedPointer<ILightKeyframe> frame42(new vmd::LightKeyframe());
        frame42->setLayerIndex(42);
        motion.addKeyframe(frame42.data());
        motion.update(IKeyframe::kLight);
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kLight));
    }
    {
        // add a light keyframe (don't forget updating motion!)
        motion.addKeyframe(keyframePtr.data());
        IKeyframe *keyframe = keyframePtr.take();
        motion.update(IKeyframe::kLight);
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kLight));
        // boudary check of findLightKeyframeAt
        ASSERT_EQ(static_cast<ILightKeyframe *>(0), motion.findLightKeyframeAt(-1));
        ASSERT_EQ(keyframe, motion.findLightKeyframeAt(0));
        ASSERT_EQ(static_cast<ILightKeyframe *>(0), motion.findLightKeyframeAt(1));
        // layer index 0 must be used
        ASSERT_EQ(1, motion.countLayers(0, IKeyframe::kLight));
        ASSERT_EQ(0, motion.findLightKeyframe(42, 1));
        // find a light keyframe with timeIndex
        ASSERT_EQ(keyframe, motion.findLightKeyframe(42, 0));
    }
    keyframePtr.reset(new vmd::LightKeyframe());
    keyframePtr->setTimeIndex(42);
    keyframePtr->setColor(Vector3(0, 0, 1));
    {
        // replaced light frame should be one keyframe (don't forget updating motion!)
        motion.replaceKeyframe(keyframePtr.data());
        IKeyframe *keyframeToDelete = keyframePtr.take();
        motion.update(IKeyframe::kLight);
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kLight));
        // no longer be find previous light keyframe
        ASSERT_EQ(1.0f, motion.findLightKeyframe(42, 0)->color().z());
        // delete light keyframe and set it null (don't forget updating motion!)
        motion.deleteKeyframe(keyframeToDelete);
        motion.update(IKeyframe::kLight);
        // light keyframes should be empty
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kLight));
        ASSERT_EQ(static_cast<ILightKeyframe *>(0), motion.findLightKeyframe(42, 0));
        ASSERT_EQ(static_cast<IKeyframe *>(0), keyframeToDelete);
    }
}

TEST(VMDMotionTest, AddAndRemoveMorphKeyframes)
{
    Encoding encoding;
    CString name("morph");
    MockIModel model;
    MockIMorph morph;
    vmd::Motion motion(&model, &encoding);
    ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kMorph));
    // mock morph
    EXPECT_CALL(model, findMorph(_)).Times(AtLeast(1)).WillRepeatedly(Return(&morph));
    QScopedPointer<IMorphKeyframe> keyframePtr(new vmd::MorphKeyframe(&encoding));
    keyframePtr->setTimeIndex(42);
    keyframePtr->setName(&name);
    {
        // The frame that the layer index is not zero should not be added
        QScopedPointer<IMorphKeyframe> frame42(new vmd::MorphKeyframe(&encoding));
        frame42->setLayerIndex(42);
        motion.addKeyframe(frame42.data());
        motion.update(IKeyframe::kMorph);
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kMorph));
    }
    {
        // add a morph keyframe (don't forget updating motion!)
        motion.addKeyframe(keyframePtr.data());
        IKeyframe *keyframe = keyframePtr.take();
        motion.update(IKeyframe::kMorph);
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kMorph));
        // boudary check of findMorphKeyframeAt
        ASSERT_EQ(static_cast<IMorphKeyframe *>(0), motion.findMorphKeyframeAt(-1));
        ASSERT_EQ(keyframe, motion.findMorphKeyframeAt(0));
        ASSERT_EQ(static_cast<IMorphKeyframe *>(0), motion.findMorphKeyframeAt(1));
        // layer index 0 must be used
        ASSERT_EQ(1, motion.countLayers(&name, IKeyframe::kMorph));
        ASSERT_EQ(0, motion.findMorphKeyframe(42, &name, 1));
        ASSERT_EQ(keyframe, motion.findMorphKeyframe(42, &name, 0));
    }
    keyframePtr.reset(new vmd::MorphKeyframe(&encoding));
    keyframePtr->setTimeIndex(42);
    keyframePtr->setName(&name);
    {
        // replaced morph frame should be one keyframe (don't forget updating motion!)
        motion.replaceKeyframe(keyframePtr.data());
        IKeyframe *keyframeToDelete = keyframePtr.take();
        motion.update(IKeyframe::kMorph);
        ASSERT_EQ(1, motion.countKeyframes(IKeyframe::kMorph));
        // no longer be find previous morph keyframe
        ASSERT_EQ(keyframeToDelete, motion.findMorphKeyframe(42, &name, 0));
        // delete light keyframe and set it null (don't forget updating motion!)
        motion.deleteKeyframe(keyframeToDelete);
        motion.update(IKeyframe::kMorph);
        // morph keyframes should be empty
        ASSERT_EQ(0, motion.countKeyframes(IKeyframe::kMorph));
        ASSERT_EQ(static_cast<IMorphKeyframe *>(0), motion.findMorphKeyframe(42, &name, 0));
        ASSERT_EQ(static_cast<IKeyframe *>(0), keyframeToDelete);
    }
}

TEST(VMDMotionTest, AddAndRemoveNullKeyframe)
{
    /* should happen nothing */
    Encoding encoding;
    MockIModel model;
    IKeyframe *nullKeyframe = 0;
    vmd::Motion motion(&model, &encoding);
    motion.addKeyframe(nullKeyframe);
    motion.replaceKeyframe(nullKeyframe);
    motion.deleteKeyframe(nullKeyframe);
}
