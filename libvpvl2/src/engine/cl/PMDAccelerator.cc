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

#include "vpvl2/cl/PMDAccelerator.h"
#include "vpvl/PMDModel.h"

#define SOFTWARE_BONE_TRANSFORM

namespace vpvl2
{
namespace cl
{

using namespace vpvl;

static const char kProgramCompileFlags[] = "-cl-fast-relaxed-math -DMAC -DGUID_ARG";

PMDAccelerator::PMDAccelerator(Context *context)
    : m_contextRef(context),
      m_program(0),
      m_performSkinningKernel(0),
      m_verticesBuffer(0),
      m_edgeOffsetBuffer(0),
      m_boneMatricesBuffer(0),
      m_boneWeightsBuffer(0),
      m_boneIndicesBuffer(0),
      m_localWGSizeForPerformSkinning(0),
      m_boneTransform(0),
      m_isBufferAllocated(false)
{
}

PMDAccelerator::~PMDAccelerator()
{
    clReleaseProgram(m_program);
    m_program = 0;
    clReleaseKernel(m_performSkinningKernel);
    m_performSkinningKernel = 0;
    delete[] m_boneTransform;
    m_boneTransform = 0;
    clReleaseMemObject(m_verticesBuffer);
    m_verticesBuffer = 0;
    clReleaseMemObject(m_edgeOffsetBuffer);
    m_edgeOffsetBuffer = 0;
    clReleaseMemObject(m_boneMatricesBuffer);
    m_boneMatricesBuffer = 0;
    clReleaseMemObject(m_boneIndicesBuffer);
    m_boneIndicesBuffer = 0;
    clReleaseMemObject(m_boneWeightsBuffer);
    m_boneWeightsBuffer = 0;
    m_localWGSizeForPerformSkinning = 0;
    m_isBufferAllocated = false;
    m_contextRef = 0;
}

bool PMDAccelerator::isAvailable() const
{
    return m_contextRef->isAvailable() && m_program;
}

bool PMDAccelerator::createKernelProgram()
{
    cl_int err;
    const IString *source = m_contextRef->renderDelegate()->loadKernelSource(IRenderDelegate::kModelSkinningKernel, 0);
    const char *sourceText = reinterpret_cast<const char *>(source->toByteArray());
    const size_t sourceSize = source->length();
    clReleaseProgram(m_program);
    cl_context context = m_contextRef->computeContext();
    m_program = clCreateProgramWithSource(context, 1, &sourceText, &sourceSize, &err);
    delete source;
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed creating an OpenCL program: %d", err);
        return false;
    }
    cl_device_id device = m_contextRef->hostDevice();
    err = clBuildProgram(m_program, 1, &device, kProgramCompileFlags, 0, 0);
    if (err != CL_SUCCESS) {
        size_t buildLogSize;
        clGetProgramBuildInfo(m_program, device, CL_PROGRAM_BUILD_LOG, 0, 0, &buildLogSize);
        cl_char *buildLog = new cl_char[buildLogSize + 1];
        clGetProgramBuildInfo(m_program, device, CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, 0);
        buildLog[buildLogSize] = 0;
        log0(0, IRenderDelegate::kLogWarning, "Failed building a program: %s", buildLog);
        delete[] buildLog;
        return false;
    }
    clReleaseKernel(m_performSkinningKernel);
    m_performSkinningKernel = clCreateKernel(m_program, "performSkinning", &err);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed creating a kernel (performSkinning): %d", err);
        return false;
    }
    return true;
}

void PMDAccelerator::uploadModel(pmd::Model *model, GLuint buffer, void *context)
{
    cl_int err;
    cl_context computeContext = m_contextRef->computeContext();
    clReleaseMemObject(m_verticesBuffer);
    m_verticesBuffer = clCreateFromGLBuffer(computeContext,
                                          CL_MEM_READ_WRITE,
                                          buffer,
                                          &err);
    if (err != CL_SUCCESS) {
        log0(context, IRenderDelegate::kLogWarning, "Failed creating OpenCL vertex buffer: %d", err);
        return;
    }
    const PMDModel *m = model->ptr();
    const int nBoneMatricesAllocs = m->bones().count() << 4;
    const int nBoneMatricesSize = nBoneMatricesAllocs * sizeof(float);
    delete[] m_boneTransform;
    m_boneTransform = new float[nBoneMatricesAllocs];
    const VertexList &vertices = m->vertices();
    const int nvertices = vertices.count();
    Array<int> boneIndices;
    Array<float> boneWeights, edgeOffset;
    boneIndices.resize(nvertices * 2);
    boneWeights.resize(nvertices);
    edgeOffset.resize(nvertices);
    for (int i = 0; i < nvertices; i++) {
        const Vertex *vertex = vertices[i];
        boneIndices[i * 2 + 0] = vertex->bone1();
        boneIndices[i * 2 + 1] = vertex->bone2();
        boneWeights[i] = vertex->weight();
        edgeOffset[i] = vertex->isEdgeEnabled() ? 1 : 0;
    }
    clReleaseMemObject(m_boneMatricesBuffer);
    m_boneMatricesBuffer = clCreateBuffer(computeContext, CL_MEM_READ_WRITE, nBoneMatricesSize, 0, &err);
    if (err != CL_SUCCESS) {
        log0(context, IRenderDelegate::kLogWarning, "Failed creating boneMatricesBuffer %d", err);
        return;
    }
    clReleaseMemObject(m_boneWeightsBuffer);
    m_boneWeightsBuffer = clCreateBuffer(computeContext, CL_MEM_READ_ONLY, nvertices * sizeof(float), 0, &err);
    if (err != CL_SUCCESS) {
        log0(context, IRenderDelegate::kLogWarning, "Failed creating boneWeightsBuffer: %d", err);
        return;
    }
    clReleaseMemObject(m_boneIndicesBuffer);
    m_boneIndicesBuffer = clCreateBuffer(computeContext, CL_MEM_READ_ONLY, nvertices * sizeof(int) * 2, 0, &err);
    if (err != CL_SUCCESS) {
        log0(context, IRenderDelegate::kLogWarning, "Failed creating boneIndicesBuffer: %d", err);
        return;
    }
    clReleaseMemObject(m_edgeOffsetBuffer);
    m_edgeOffsetBuffer = clCreateBuffer(computeContext, CL_MEM_READ_ONLY, nvertices * sizeof(float), 0, &err);
    if (err != CL_SUCCESS) {
        log0(context, IRenderDelegate::kLogWarning, "Failed creating edgeOffsetBuffer: %d", err);
        return;
    }
    cl_device_id device = m_contextRef->hostDevice();
    err = clGetKernelWorkGroupInfo(m_performSkinningKernel,
                                   device,
                                   CL_KERNEL_WORK_GROUP_SIZE,
                                   sizeof(m_localWGSizeForPerformSkinning),
                                   &m_localWGSizeForPerformSkinning,
                                   0);
    if (err != CL_SUCCESS) {
        log0(context, IRenderDelegate::kLogWarning, "Failed getting kernel work group information (CL_KERNEL_WORK_GROUP_SIZE): %d", err);
        return;
    }
    cl_command_queue queue = m_contextRef->commandQueue();
    err = clEnqueueWriteBuffer(queue, m_boneIndicesBuffer, CL_TRUE, 0, nvertices * sizeof(int) * 2, &boneIndices[0], 0, 0, 0);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed enqueue a command to write BoneIndicesBuffer: %d", err);
        return;
    }
    err = clEnqueueWriteBuffer(queue, m_boneWeightsBuffer, CL_TRUE, 0, nvertices * sizeof(float), &boneWeights[0], 0, 0, 0);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed enqueue a command to write boneWeightsBuffer: %d", err);
        return;
    }
    err = clEnqueueWriteBuffer(queue, m_edgeOffsetBuffer, CL_TRUE, 0, nvertices * sizeof(float), &edgeOffset[0], 0, 0, 0);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed enqueue a command to write boneWeightsBuffer: %d", err);
        return;
    }
    m_isBufferAllocated = true;
}

void PMDAccelerator::updateModel(pmd::Model *model, const Scene *scene)
{
    if (!m_isBufferAllocated)
        return;
    const PMDModel *m = model->ptr();
    const BoneList &bones = m->bones();
    const int nbones = bones.count();
    Transform transform = Transform::getIdentity();
    for (int i = 0; i < nbones; i++) {
        Bone *bone = bones[i];
        int index = i << 4;
        bone->getSkinTransform(transform);
        transform.getOpenGLMatrix(&m_boneTransform[index]);
    }
    size_t nsize = (nbones * sizeof(float)) << 4;
    cl_int err;
    size_t local, global;
    cl_command_queue queue = m_contextRef->commandQueue();
    err = clEnqueueWriteBuffer(queue, m_boneMatricesBuffer, CL_TRUE, 0, nsize, m_boneTransform, 0, 0, 0);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed enqueue a command to write output matrices buffer: %d", err);
        return;
    }
    /* force flushing OpenGL commands to acquire GL objects by OpenCL */
    glFinish();
    clEnqueueAcquireGLObjects(queue, 1, &m_verticesBuffer, 0, 0, 0);
    int argumentIndex = 0;
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(m_boneMatricesBuffer), &m_boneMatricesBuffer);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting 1st argument of kernel (localMatrices): %d", err);
        return;
    }
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(m_boneWeightsBuffer), &m_boneWeightsBuffer);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting 2nd argument of kernel (boneMatrices): %d", err);
        return;
    }
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(m_boneIndicesBuffer), &m_boneIndicesBuffer);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting 3rd argument of kernel (boneIndices): %d", err);
        return;
    }
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(m_edgeOffsetBuffer), &m_edgeOffsetBuffer);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting %dth argument of kernel (edgeOffset): %d", err);
        return;
    }
    const Vector3 &lightDirection = -m->lightPosition();
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(lightDirection), &lightDirection);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting %dth argument of kernel (lightDirection): %d", argumentIndex, err);
        return;
    }
    const ICamera *camera = scene->camera();
    const Vector3 &cameraPosition = camera->position() + Vector3(0, 0, camera->distance());
    const Scalar &edgeScaleFactor = model->edgeScaleFactor(cameraPosition) * model->edgeWidth();
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(edgeScaleFactor), &edgeScaleFactor);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting %dth argument of kernel (edgeScaleFactor): %d", argumentIndex, err);
        return;
    }
    const int nvertices = m->vertices().count();
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(nvertices), &nvertices);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting %dth argument of kernel (nvertices): %d", argumentIndex, err);
        return;
    }
    size_t strideSize = m->strideSize(PMDModel::kVerticesStride) >> 4;
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(strideSize), &strideSize);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting %dth argument of kernel (strideSize): %d", argumentIndex, err);
        return;
    }
    size_t offsetPosition = m->strideOffset(PMDModel::kVerticesStride) >> 4;
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(offsetPosition), &offsetPosition);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting %dth argument of kernel (offsetPosition): %d", argumentIndex, err);
        return;
    }
    size_t offsetNormal = m->strideOffset(PMDModel::kNormalsStride) >> 4;
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(offsetNormal), &offsetNormal);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting %dth argument of kernel (offsetNormal): %d", argumentIndex, err);
        return;
    }
    size_t offsetToonTexCoord = m->strideOffset(PMDModel::kToonTextureStride) >> 4;
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(offsetToonTexCoord), &offsetToonTexCoord);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting %dth argument of kernel (offsetTexCoord): %d", argumentIndex, err);
        return;
    }
    size_t offsetEdge = m->strideOffset(PMDModel::kEdgeVerticesStride) >> 4;
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(offsetEdge), &offsetEdge);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting %dth argument of kernel (offsetEdge): %d", argumentIndex, err);
        return;
    }
    err = clSetKernelArg(m_performSkinningKernel, argumentIndex++, sizeof(m_verticesBuffer), &m_verticesBuffer);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed setting %dth argument of kernel (vertices): %d", argumentIndex, err);
        return;
    }
    local = m_localWGSizeForPerformSkinning;
    global = local * ((nvertices + (local - 1)) / local);
    err = clEnqueueNDRangeKernel(queue, m_performSkinningKernel, 1, 0, &global, &local, 0, 0, 0);
    if (err != CL_SUCCESS) {
        log0(0, IRenderDelegate::kLogWarning, "Failed enqueue executing kernel: %d", err);
        return;
    }
    clEnqueueReleaseGLObjects(queue, 1, &m_verticesBuffer, 0, 0, 0);
    clFinish(queue);
}

void PMDAccelerator::log0(void *context, IRenderDelegate::LogLevel level, const char *format...)
{
    va_list ap;
    va_start(ap, format);
    m_contextRef->renderDelegate()->log(context, level, format, ap);
    va_end(ap);
}

#undef SOFTWARE_BONE_TRANSFORM

} /* namespace cl */
} /* namespace vpvl2 */
