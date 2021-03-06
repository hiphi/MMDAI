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

/* libvpvl2 */
#include <vpvl2/vpvl2.h>
#include <vpvl2/IRenderDelegate.h>

/* internal headers for debug */
#include "vpvl2/pmx/Bone.h"
#include "vpvl2/pmx/Joint.h"
#include "vpvl2/pmx/Label.h"
#include "vpvl2/pmx/Material.h"
#include "vpvl2/pmx/Model.h"
#include "vpvl2/pmx/Morph.h"
#include "vpvl2/pmx/RigidBody.h"
#include "vpvl2/pmx/Vertex.h"
#include "vpvl2/asset/Model.h"
#include "vpvl2/pmd/Model.h"
#include "vpvl2/vmd/Motion.h"

/* ICU */
#include <unicode/unistr.h>

/* SDL */
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>

/* Bullet Physics */
#ifndef VPVL2_NO_BULLET
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#else
VPVL2_DECLARE_HANDLE(btDiscreteDynamicsWorld)
#endif

/* STL */
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>

/* Open Asset Import Library */
#ifdef VPVL2_LINK_ASSIMP
#include <assimp.hpp>
#include <DefaultLogger.h>
#include <Logger.h>
#include <aiPostProcess.h>
#else
BT_DECLARE_HANDLE(aiScene);
#endif

/* GLM */
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {

using namespace vpvl2;

const char *kDefaultEncoding = "utf8";

class String : public IString {
public:
    String(const UnicodeString &value)
        : m_value(value),
          m_bytes(0)
    {
        size_t size = value.length(), length = value.extract(0, size, 0, kDefaultEncoding);
        m_bytes = new char[length + 1];
        value.extract(0, size, reinterpret_cast<char *>(m_bytes), kDefaultEncoding);
        m_bytes[length] = 0;
    }
    ~String() {
        delete[] m_bytes;
    }

    bool startsWith(const IString *value) const {
        return m_value.startsWith(static_cast<const String *>(value)->value());
    }
    bool contains(const IString *value) const {
        return m_value.indexOf(static_cast<const String *>(value)->value()) != -1;
    }
    bool endsWith(const IString *value) const {
        return m_value.endsWith(static_cast<const String *>(value)->value());
    }
    IString *clone() const {
        return new String(m_value);
    }
    const HashString toHashString() const {
        return HashString(m_bytes);
    }
    bool equals(const IString *value) const {
        return m_value == static_cast<const String *>(value)->value();
    }
    const UnicodeString &value() const {
        return m_value;
    }
    const uint8_t *toByteArray() const {
        return reinterpret_cast<const uint8_t *>(m_bytes);
    }
    size_t length() const {
        return m_value.length();
    }

private:
    const UnicodeString m_value;
    char *m_bytes;
};

class Encoding : public IEncoding {
public:
    Encoding()
    {
    }
    ~Encoding() {
    }

    const IString *stringConstant(ConstantType value) const {
        switch (value) {
        case kLeft: {
            static const String s("左");
            return &s;
        }
        case kRight: {
            static const String s("右");
            return &s;
        }
        case kFinger: {
            static const String s("指");
            return &s;
        }
        case kElbow: {
            static const String s("ひじ");
            return &s;
        }
        case kArm: {
            static const String s("腕");
            return &s;
        }
        case kWrist: {
            static const String s("手首");
            return &s;
        }
        case kCenter: {
            static const String s("センター");
            return &s;
        }
        default: {
            static const String s("");
            return &s;
        }
        }
    }
    IString *toString(const uint8_t *value, size_t size, IString::Codec codec) const {
        IString *s = 0;
        const char *str = reinterpret_cast<const char *>(value);
        switch (codec) {
        case IString::kShiftJIS:
            s = new String(UnicodeString(str, size, "shift_jis"));
            break;
        case IString::kUTF8:
            s = new String(UnicodeString(str, size, "utf-8"));
            break;
        case IString::kUTF16:
            s = new String(UnicodeString(str, size, "utf-16le"));
            break;
        default:
            break;
        }
        return s;
    }
    IString *toString(const uint8_t *value, IString::Codec codec, size_t maxlen) const {
        size_t size = strlen(reinterpret_cast<const char *>(value));
        return toString(value, btMin(maxlen, size), codec);
    }
    uint8_t *toByteArray(const IString *value, IString::Codec codec) const {
        if (value) {
            const String *s = static_cast<const String *>(value);
            const UnicodeString &src = s->value();
            const char *codecTo = 0;
            switch (codec) {
            case IString::kShiftJIS:
                codecTo = "shift_jis";
                break;
            case IString::kUTF8:
                codecTo = "utf-8";
                break;
            case IString::kUTF16:
                codecTo = "utf-16le";
                break;
            default:
                break;
            }
            size_t size = s->length(), newStringLength = src.extract(0, size, 0, codecTo);
            uint8_t *data = new uint8_t[newStringLength + 1];
            src.extract(0, size, reinterpret_cast<char *>(data), codecTo);
            data[newStringLength] = 0;
            return data;
        }
        return 0;
    }
    void disposeByteArray(uint8_t *value) const {
        delete[] value;
    }
};

static const std::string UIUnicodeStringToStdString(const UnicodeString &value)
{
    size_t size = value.length(), length = value.extract(0, size, 0, kDefaultEncoding);
    std::vector<char> bytes(length + 1);
    value.extract(0, size, reinterpret_cast<char *>(&bytes[0]), kDefaultEncoding);
    return std::string(bytes.begin(), bytes.end() - 1);
}

static bool UIUnicodeStringToBool(const UnicodeString &value)
{
    const std::string &v = UIUnicodeStringToStdString(value);
    return v == "true" || v == "1" || v == "y" || v == "yes";
}

static int UIUnicodeStringToInt(const UnicodeString &value, int def = 0)
{
    int v = int(strtol(UIUnicodeStringToStdString(value).c_str(), 0, 10));
    return v != 0 ? v : def;
}

static int UIUnicodeStringToFloat(const UnicodeString &value, float def = 0.0)
{
    float v = strtof(UIUnicodeStringToStdString(value).c_str(), 0);
    return v != 0 ? v : def;
}

static bool UILoadFile(const UnicodeString &path, std::string &bytes)
{
    bytes.clear();
    FILE *fp = ::fopen(UIUnicodeStringToStdString(path).c_str(), "rb");
    bool ret = false;
    if (fp) {
        ::fseek(fp, 0, SEEK_END);
        size_t size = ::ftell(fp);
        ::fseek(fp, 0, SEEK_SET);
        std::vector<char> data(size);
        ::fread(&data[0], size, 1, fp);
        bytes.assign(data.begin(), data.end());
        ret = true;
    }
    ::fclose(fp);
    return ret;
}

static const uint8_t *UICastData(const std::string &data)
{
    return reinterpret_cast<const uint8_t *>(data.c_str());
}

typedef std::map<UnicodeString, UnicodeString> UIStringMap;

class Delegate : public IRenderDelegate {
public:
    struct TextureCache {
        TextureCache() {}
        TextureCache(int w, int h, GLuint i)
            : width(w),
              height(h),
              id(i)
        {
        }
        int width;
        int height;
        GLuint id;
    };
    typedef std::map<UnicodeString, TextureCache> TextureCacheMap;
    struct PrivateContext {
        TextureCacheMap textureCache;
    };

    Delegate(Scene *sceneRef, UIStringMap *configRef)
        : m_sceneRef(sceneRef),
          m_configRef(configRef),
          m_lightWorldMatrix(1),
          m_lightViewMatrix(1),
          m_lightProjectionMatrix(1),
          m_cameraWorldMatrix(1),
          m_cameraViewMatrix(1),
          m_cameraProjectionMatrix(1),
          m_colorSwapSurface(0)
    {
        m_colorSwapSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 0, 0, 32,
                                                  0x000000ff,
                                                  0x0000ff00,
                                                  0x00ff0000,
                                                  0xff000000);
    }
    ~Delegate()
    {
        m_sceneRef = 0;
        m_configRef = 0;
        SDL_FreeSurface(m_colorSwapSurface);
    }

    void allocateContext(const IModel * /* model */, void *&context) {
        PrivateContext *ctx = new PrivateContext();
        context = ctx;
    }
    void releaseContext(const IModel * /* model */, void *&context) {
        delete static_cast<PrivateContext *>(context);
        context = 0;
    }
    bool uploadTexture(const IString *name, const IString *dir, int flags, Texture &texture, void *context) {
        bool mipmap = flags & IRenderDelegate::kGenerateTextureMipmap, ok = false;
        bool ret = false;
        if (flags & IRenderDelegate::kTexture2D) {
            const UnicodeString &path = createPath(dir, name);
            std::cerr << "texture: " << UIUnicodeStringToStdString(path) << std::endl;
            ret = uploadTextureInternal(path, texture, false, false, mipmap, ok, context);
        }
        else if (flags & IRenderDelegate::kToonTexture) {
            if (dir) {
                const UnicodeString &path = createPath(dir, name);
                std::cerr << "toon: " << UIUnicodeStringToStdString(path) << std::endl;
                ret = uploadTextureInternal(path, texture, true, false, mipmap, ok, context);
            }
            if (!ok) {
                String s((*m_configRef)["dir.system.toon"]);
                const UnicodeString &path = createPath(&s, name);
                std::cerr << "system: " << UIUnicodeStringToStdString(path) << std::endl;
                ret = uploadTextureInternal(path, texture, true, true, mipmap, ok, context);
            }
        }
        return ret;
    }
    void getMatrix(float value[], const IModel *model, int flags) const {
        glm::mat4x4 m(1);
        if (flags & IRenderDelegate::kShadowMatrix) {
            if (flags & IRenderDelegate::kProjectionMatrix)
                m *= m_cameraProjectionMatrix;
            if (flags & IRenderDelegate::kViewMatrix)
                m *= m_cameraViewMatrix;
            if (flags & IRenderDelegate::kWorldMatrix) {
                glm::mat4x4 shadowMatrix(1);
                /*
                static const Vector3 plane(0.0f, 1.0f, 0.0f);
                const ILight *light = m_sceneRef->light();
                const Vector3 &direction = light->direction();
                const Scalar dot = plane.dot(-direction);
                for (int i = 0; i < 4; i++) {
                    int offset = i * 4;
                    for (int j = 0; j < 4; j++) {
                        int index = offset + j;
                        shadowMatrix[index] = plane[i] * direction[j];
                        if (i == j)
                            shadowMatrix[index] += dot;
                    }
                }
                */
                m *= shadowMatrix;
                m *= m_cameraWorldMatrix;
                m = glm::scale(m, glm::vec3(model->scaleFactor()));
            }
        }
        else if (flags & IRenderDelegate::kCameraMatrix) {
            if (flags & IRenderDelegate::kProjectionMatrix)
                m *= m_cameraProjectionMatrix;
            if (flags & IRenderDelegate::kViewMatrix)
                m *= m_cameraViewMatrix;
            if (flags & IRenderDelegate::kWorldMatrix) {
                const IBone *bone = model->parentBone();
                Transform transform;
                transform.setOrigin(model->position());
                transform.setRotation(model->rotation());
                Scalar matrix[16];
                transform.getOpenGLMatrix(matrix);
                m *= glm::make_mat4x4(matrix);
                if (bone) {
                    transform = bone->worldTransform();
                    transform.getOpenGLMatrix(matrix);
                    m *= glm::make_mat4x4(matrix);
                }
                m *= m_cameraWorldMatrix;
                m = glm::scale(m, glm::vec3(model->scaleFactor()));
            }
        }
        else if (flags & IRenderDelegate::kLightMatrix) {
            if (flags & IRenderDelegate::kWorldMatrix) {
                m *= m_lightWorldMatrix;
                m = glm::scale(m, glm::vec3(model->scaleFactor()));
            }
            if (flags & IRenderDelegate::kProjectionMatrix)
                m *= m_lightProjectionMatrix;
            if (flags & IRenderDelegate::kViewMatrix)
                m *= m_lightViewMatrix;
        }
        if (flags & IRenderDelegate::kInverseMatrix)
            m = glm::inverse(m);
        if (flags & IRenderDelegate::kTransposeMatrix)
            m = glm::transpose(m);
        memcpy(value, glm::value_ptr(m), sizeof(float) * 16);
    }
    void log(void * /* context */, LogLevel /* level */, const char *format, va_list ap) {
        char buf[1024];
        vsnprintf(buf, sizeof(buf), format, ap);
        std::cerr << buf << std::endl;
    }
    IString *loadShaderSource(ShaderType type, const IModel *model, const IString * /* dir */, void * /* context */) {
        std::string file;
        switch (model->type()) {
        case IModel::kAsset:
            file += "asset/";
            break;
        case IModel::kPMD:
            file += "pmd/";
            break;
        case IModel::kPMX:
            file += "pmx/";
            break;
        default:
            break;
        }
        switch (type) {
        case kEdgeVertexShader:
            file += "edge.vsh";
            break;
        case kEdgeFragmentShader:
            file += "edge.fsh";
            break;
        case kModelVertexShader:
            file += "model.vsh";
            break;
        case kModelFragmentShader:
            file += "model.fsh";
            break;
        case kShadowVertexShader:
            file += "shadow.vsh";
            break;
        case kShadowFragmentShader:
            file += "shadow.fsh";
            break;
        case kZPlotVertexShader:
            file += "zplot.vsh";
            break;
        case kZPlotFragmentShader:
            file += "zplot.fsh";
            break;
        case kEdgeWithSkinningVertexShader:
            file += "skinning/edge.vsh";
            break;
        case kModelWithSkinningVertexShader:
            file += "skinning/model.vsh";
            break;
        case kShadowWithSkinningVertexShader:
            file += "skinning/shadow.vsh";
            break;
        case kZPlotWithSkinningVertexShader:
            file += "skinning/zplot.vsh";
            break;
        case kModelEffectTechniques:
        case kMaxShaderType:
        default:
            break;
        }
        UnicodeString path = (*m_configRef)["dir.system.shaders"];
        path.append("/");
        path.append(UnicodeString::fromUTF8(file));
        std::string bytes;
        if (UILoadFile(path, bytes)) {
            return new(std::nothrow) String(UnicodeString::fromUTF8("#version 120\n" + bytes));
        }
        else {
            return 0;
        }
    }
    IString *loadKernelSource(KernelType type, void * /* context */) {
        std::string file;
        switch (type) {
        case kModelSkinningKernel:
            file += "skinning.cl";
            break;
        default:
            break;
        }
        UnicodeString path = (*m_configRef)["dir.system.kernels"];
        path.append("/");
        path.append(UnicodeString::fromUTF8(file));
        std::string bytes;
        if (UILoadFile(path, bytes)) {
            return new(std::nothrow) String(UnicodeString::fromUTF8(bytes));
        }
        else {
            return 0;
        }
    }
    IString *toUnicode(const uint8_t *str) const {
        const char *s = reinterpret_cast<const char *>(str);
        return new(std::nothrow) String(UnicodeString(s, strlen(s), "shift_jis"));
    }

    IString *loadShaderSource(ShaderType /* type */, const IString * /* path */) {
        return 0;
    }
    void getToonColor(const IString * /* name */, const IString * /* dir */, Color & /* value */, void * /* context */) {
    }
    void getViewport(Vector3 & /* value */) const {
    }
    void getMousePosition(Vector4 & /* value */, MousePositionType /* type */) const {
    }
    void getTime(float & /* value */, bool /* sync */) const {
    }
    void getElapsed(float & /* value */, bool /* sync */) const {
    }
    void uploadAnimatedTexture(float /* offset */, float /* speed */, float /* seek */, void * /* texture */) {
    }
    IModel *findModel(const IString * /* name */) const {
        return 0;
    }
    IModel *effectOwner(const IEffect * /* effect */) const {
        return 0;
    }
    void setRenderColorTargets(const void * /* targets */, const int /* ntargets */) {
    }
    void bindRenderColorTarget(void * /* texture */, size_t /* width */, size_t /* height */, int /* index */, bool /* enableAA */) {
    }
    void releaseRenderColorTarget(void * /* texture */, size_t /* width */, size_t /* height */, int /* index */, bool /* enableAA */) {
    }
    void bindRenderDepthStencilTarget(void * /* texture */, void * /* depth */, void * /* stencil */, size_t /* width */, size_t /* height */, bool /* enableAA */) {
    }
    void releaseRenderDepthStencilTarget(void * /* texture */, void * /* depth */, void * /* stencil */, size_t /* width */, size_t /* height */, bool /* enableAA */) {
    }

    void setCameraMatrix(const glm::mat4x4 &world, const glm::mat4x4 &view, const glm::mat4x4 &projection) {
        m_cameraWorldMatrix = world;
        m_cameraViewMatrix = view;
        m_cameraProjectionMatrix = projection;
    }

private:
    static const UnicodeString createPath(const IString *dir, const UnicodeString &name) {
        const UnicodeString &d = static_cast<const String *>(dir)->value();
        return d + "/" + name;
    }
    static const UnicodeString createPath(const IString *dir, const IString *name) {
        const UnicodeString &d = static_cast<const String *>(dir)->value();
        const UnicodeString &n = static_cast<const String *>(name)->value();
        return d + "/" + n;
    }
    bool uploadTextureInternal(const UnicodeString &path, Texture &texture, bool isToon,
                               bool /* isSystem */, bool /* mipmap */, bool &ok, void *context) {
        PrivateContext *privateContext = static_cast<PrivateContext *>(context);
        /* テクスチャのキャッシュを検索する */
        if (privateContext) {
            TextureCacheMap &tc = privateContext->textureCache;
            if (tc.find(path) != tc.end()) {
                setTextureID(tc[path], isToon, texture);
                ok = true;
                return true;
            }
        }
        std::string bytes;
        if (!UILoadFile(path, bytes)) {
            ok = false;
            return true;
        }
        SDL_Surface *surface = 0;
        SDL_RWops *source = SDL_RWFromConstMem(bytes.data(), bytes.length());
        const UnicodeString &lowerPath = path.tempSubString().toLower();
        if (lowerPath.endsWith(".sph") || lowerPath.endsWith(".spa")) {
            char extension[] = "BMP";
            surface = IMG_LoadTyped_RW(source, 0, extension);
        }
        else if (lowerPath.endsWith(".tga")) {
            char extension[] = "TGA";
            surface = IMG_LoadTyped_RW(source, 0, extension);
        }
        else if (lowerPath.endsWith(".png") && IMG_isPNG(source)) {
            char extension[] = "PNG";
            surface = IMG_LoadTyped_RW(source, 0, extension);
        }
        else if (lowerPath.endsWith(".bmp") && IMG_isBMP(source)) {
            char extension[] = "BMP";
            surface = IMG_LoadTyped_RW(source, 0, extension);
        }
        else if (lowerPath.endsWith(".jpg") && IMG_isJPG(source)) {
            char extension[] = "JPG";
            surface = IMG_LoadTyped_RW(source, 0, extension);
        }
        SDL_FreeRW(source);
        if (!surface) {
            ok = false;
            return true;
        }
        surface = SDL_ConvertSurface(surface, m_colorSwapSurface->format, SDL_SWSURFACE);
        GLuint textureID = 0, internal = GL_RGBA8, format = GL_RGBA;
        size_t width = surface->w, height = surface->h;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
        SDL_FreeSurface(surface);
        glBindTexture(GL_TEXTURE_2D, 0);
        TextureCache cache(width, height, textureID);
        setTextureID(cache, isToon, texture);
        addTextureCache(path, cache, privateContext);
        ok = textureID != 0;
        return ok;
    }
    void setTextureID(const TextureCache &cache, bool isToon, Texture &output) {
        output.width = cache.width;
        output.height = cache.height;
        *const_cast<GLuint *>(static_cast<const GLuint *>(output.object)) = cache.id;
        if (!isToon) {
            GLuint textureID = *static_cast<const GLuint *>(output.object);
            glTexParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }
    void addTextureCache(const UnicodeString &path, const TextureCache &texture, PrivateContext *context) {
        if (context)
            context->textureCache.insert(std::make_pair(path, texture));
    }

    Scene *m_sceneRef;
    UIStringMap *m_configRef;
    glm::mat4x4 m_lightWorldMatrix;
    glm::mat4x4 m_lightViewMatrix;
    glm::mat4x4 m_lightProjectionMatrix;
    glm::mat4x4 m_cameraWorldMatrix;
    glm::mat4x4 m_cameraViewMatrix;
    glm::mat4x4 m_cameraProjectionMatrix;
    SDL_Surface *m_colorSwapSurface;
};

struct UIContext
{
    UIContext(Scene *scene, UIStringMap *config, Delegate *delegate)
        : sceneRef(scene),
          configRef(config),
      #if SDL_MAJOR_VERSION == 2
          windowRef(0),
      #endif
          delegateRef(delegate),
          width(640),
          height(480),
          restarted(SDL_GetTicks()),
          current(restarted),
          currentFPS(0),
          active(true)
    {
    }
    void updateFPS() {
        current = SDL_GetTicks();
        if (current - restarted > 1000) {
#if SDL_MAJOR_VERSION == 2
            snprintf(title, sizeof(title), "libvpvl2 with SDL2 (FPS:%d)", currentFPS);
            SDL_SetWindowTitle(windowRef, title);
#elif SDL_MAJOR_VERSION == 1
            snprintf(title, sizeof(title), "libvpvl2 with SDL (FPS:%d)", currentFPS);
            SDL_WM_SetCaption(title, 0);
#endif
            restarted = current;
            currentFPS = 0;
        }
        currentFPS++;
    }

    const Scene *sceneRef;
    const UIStringMap *configRef;
#if SDL_MAJOR_VERSION == 2
    SDL_Window *windowRef;
#endif
    Delegate *delegateRef;
    size_t width;
    size_t height;
    Uint32 restarted;
    Uint32 current;
    int currentFPS;
    char title[32];
    bool active;
};

static void UIHandleKeyEvent(const SDL_KeyboardEvent &event, UIContext &context)
{
#if SDL_MAJOR_VERSION == 2
    const SDL_Keysym &keysym = event.keysym;
#elif SDL_MAJOR_VERSION == 1
    const SDL_keysym &keysym = event.keysym;
#endif
    const int degree = 15;
    ICamera *camera = context.sceneRef->camera();
    switch (keysym.sym) {
    case SDLK_RIGHT:
        camera->setAngle(camera->angle() + Vector3(0, degree, 0));
        break;
    case SDLK_LEFT:
        camera->setAngle(camera->angle() + Vector3(0, -degree, 0));
        break;
    case SDLK_UP:
        camera->setAngle(camera->angle() + Vector3(degree, 0, 0));
        break;
    case SDLK_DOWN:
        camera->setAngle(camera->angle() + Vector3(-degree, 0, 0));
        break;
    case SDLK_ESCAPE:
        context.active = false;
        break;
    default:
        break;
    }
}

static void UIHandleMouseMotion(const SDL_MouseMotionEvent &event, UIContext &context)
{
    if (event.state == SDL_PRESSED) {
        ICamera *camera = context.sceneRef->camera();
        const Scalar &factor = 0.5;
        camera->setAngle(camera->angle() + Vector3(event.yrel * factor, event.xrel * factor, 0));
    }
}

#if SDL_MAJOR_VERSION == 2
static void UIHandleMouseWheel(const SDL_MouseWheelEvent &event, UIContext &context)
{
    ICamera *camera = context.sceneRef->camera();
    const Scalar &factor = 1.0;
    camera->setDistance(camera->distance() + event.y * factor);
}
#endif

static void UIUpdateCamera(UIContext &context)
{
    const ICamera *camera = context.sceneRef->camera();
    Scalar matrix[16];
    camera->modelViewTransform().getOpenGLMatrix(matrix);
    const float &aspect = context.width / float(context.height);
    const glm::mat4x4 world, &view = glm::make_mat4x4(matrix),
            &projection = glm::perspective(camera->fov(), aspect, camera->znear(), camera->zfar());
    context.delegateRef->setCameraMatrix(world, view, projection);
}

static void UIDrawScreen(const UIContext &context)
{
    glViewport(0, 0, context.width, context.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    const Array<IRenderEngine *> &engines = context.sceneRef->renderEngines();
    const int nengines = engines.count();
    for (int i = 0; i < nengines; i++) {
        IRenderEngine *engine = engines[i];
        engine->preparePostProcess();
    }
    for (int i = 0; i < nengines; i++) {
        IRenderEngine *engine = engines[i];
        engine->performPreProcess();
    }
    for (int i = 0; i < nengines; i++) {
        IRenderEngine *engine = engines[i];
        engine->renderModel();
        engine->renderEdge();
        engine->renderShadow();
    }
    for (int i = 0; i < nengines; i++) {
        IRenderEngine *engine = engines[i];
        engine->performPostProcess();
    }
#if SDL_MAJOR_VERSION == 2
    SDL_GL_SwapWindow(context.windowRef);
#elif SDL_MAJOR_VERSION == 1
    SDL_GL_SwapBuffers();
#endif
}

static void UIProceedEvents(UIContext &context)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_MOUSEMOTION:
            UIHandleMouseMotion(event.motion, context);
            break;
        case SDL_KEYDOWN:
            UIHandleKeyEvent(event.key, context);
            break;
#if SDL_MAJOR_VERSION == 2
        case SDL_MOUSEWHEEL:
            UIHandleMouseWheel(event.wheel, context);
            break;
#elif SDL_MAJOR_VERSION == 1
        case SDL_VIDEORESIZE:
            context.width = event.resize.w;
            context.height = event.resize.h;
            break;
#endif
        case SDL_QUIT:
            context.active = false;
            break;
        default:
            break;
        }
    }
    UIUpdateCamera(context);
}

} /* namespace anonymous */

int main(int /* argc */, char ** /* argv[] */)
{
    atexit(SDL_Quit);
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init(SDL_INIT_VIDEO) failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    atexit(IMG_Quit);
    if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) < 0) {
        std::cerr << "SDL_Init(SDL_INIT_VIDEO) failed: " << SDL_GetError() << std::endl;
        return -1;
    }
#if SDL_MAJOR_VERSION == 1
    SDL_WM_SetCaption("libvpvl2 with SDL", 0);
    const SDL_VideoInfo *info = SDL_GetVideoInfo();
    if (!info) {
        std::cerr << "SDL_GetVideoInfo() failed: " << SDL_GetError() << std::endl;
        return -1;
    }
#endif

    std::ifstream stream("config.ini");
    std::string line;
    UIStringMap config;
    UnicodeString k, v;
    while (stream && std::getline(stream, line)) {
        if (line.empty() || line.find_first_of("#;") != std::string::npos)
            continue;
        std::istringstream ss(line);
        std::string key, value;
        std::getline(ss, key, '=');
        std::getline(ss, value);
        k.setTo(UnicodeString::fromUTF8(key));
        v.setTo(UnicodeString::fromUTF8(value));
        config[k.trim()] = v.trim();
    }

    size_t width = UIUnicodeStringToInt("window.width", 640),
            height = UIUnicodeStringToInt("window.height", 480);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#if SDL_MAJOR_VERSION == 2
    SDL_Window *window = SDL_CreateWindow("libvpvl2 with SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          width, height, SDL_WINDOW_OPENGL);
    if (!window) {
        std::cerr << "SDL_CreateWindow(title, x, y, width, height, SDL_OPENGL) failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    SDL_GLContext contextGL = SDL_GL_CreateContext(window);
    if (!contextGL) {
        std::cerr << "SDL_GL_CreateContext(window) failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    SDL_DisableScreenSaver();
#elif SDL_MAJOR_VERSION == 1
    SDL_Surface *surface = SDL_SetVideoMode(width, height, info->vfmt->BitsPerPixel, SDL_OPENGL);
    if (!surface) {
        std::cerr << "SDL_SetVideoMode(width, height, bpp, SDL_OPENGL) failed: " << SDL_GetError() << std::endl;
        return -1;
    }
#endif
    std::cerr << "GL_VERSION:                " << glGetString(GL_VERSION) << std::endl;
    std::cerr << "GL_VENDOR:                 " << glGetString(GL_VENDOR) << std::endl;
    std::cerr << "GL_RENDERER:               " << glGetString(GL_RENDERER) << std::endl;

    int value = 0;
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &value);
    std::cerr << "SDL_GL_RED_SIZE:           " << value << std::endl;
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &value);
    std::cerr << "SDL_GL_GREEN_SIZE:         " << value << std::endl;
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &value);
    std::cerr << "SDL_GL_BLUE_SIZE:          " << value << std::endl;
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &value);
    std::cerr << "SDL_GL_DEPTH_SIZE:         " << value << std::endl;
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &value);
    std::cerr << "SDL_GL_STENCIL_SIZE:       " << value << std::endl;
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value);
    std::cerr << "SDL_GL_DOUBLEBUFFER:       " << value << std::endl;
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &value);
    std::cerr << "SDL_GL_MULTISAMPLEBUFFERS: " << value << std::endl;
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &value);
    std::cerr << "SDL_GL_MULTISAMPLESAMPLES: " << value << std::endl;
    SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &value);
    std::cerr << "SDL_GL_ACCELERATED_VISUAL: " << value << std::endl;

    Encoding encoding;
    Factory factory(&encoding);
    Scene scene;
    Delegate delegate(&scene, &config);
    bool ok = false;
    const UnicodeString &motionPath = config["dir.motion"] + "/" + config["file.motion"];
    if (UIUnicodeStringToBool(config["enable.opencl"])) {
        scene.setAccelerationType(Scene::kOpenCLAccelerationType1);
    }

    std::string data;
    int nmodels = UIUnicodeStringToInt(config["models/size"]);
    for (int i = 0; i < nmodels; i++) {
        std::ostringstream stream;
        stream << "models/" << (i + 1);
        const UnicodeString &prefix = UnicodeString::fromUTF8(stream.str()),
                &modelPath = config[prefix + "/path"];
        int indexOf = modelPath.lastIndexOf("/");
        String dir(modelPath.tempSubString(0, indexOf));
        if (UILoadFile(modelPath, data)) {
            int flags = 0;
            IModel *model = factory.createModel(UICastData(data), data.size(), ok);
            IRenderEngine *engine = scene.createRenderEngine(&delegate, model, flags);
            model->setEdgeWidth(UIUnicodeStringToFloat(config[prefix + "/edge.width"]));
            if (engine->upload(&dir)) {
                scene.addModel(model, engine);
                if (UILoadFile(motionPath, data)) {
                    IMotion *motion = factory.createMotion(UICastData(data), data.size(), model, ok);
                    scene.addMotion(motion);
                }
            }
        }
    }
    int nassets = UIUnicodeStringToInt(config["assets/size"]);
    for (int i = 0; i < nassets; i++) {
        std::ostringstream stream;
        stream << "assets/" << (i + 1);
        const UnicodeString &prefix = UnicodeString::fromUTF8(stream.str()),
                &assetPath = config[prefix + "/path"];
        if (UILoadFile(assetPath, data)) {
            int indexOf = assetPath.lastIndexOf("/");
            String dir(assetPath.tempSubString(0, indexOf));
            IModel *asset = factory.createModel(UICastData(data), data.size(), ok);
            IRenderEngine *engine = scene.createRenderEngine(&delegate, asset, 0);
            if (engine->upload(&dir)) {
                scene.addModel(asset, engine);
            }
        }
    }

    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glClearColor(0, 0, 1, 0);

    UIContext context(&scene, &config, &delegate);
#if SDL_MAJOR_VERSION == 2
    context.windowRef = window;
#endif
    while (context.active) {
        UIProceedEvents(context);
        UIDrawScreen(context);
        scene.update(Scene::kUpdateAll);
        context.updateFPS();
    }
#if SDL_MAJOR_VERSION == 2
    SDL_EnableScreenSaver();
    SDL_GL_DeleteContext(contextGL);
    SDL_DestroyWindow(window);
#elif SDL_MAJOR_VERSION == 1
    SDL_FreeSurface(surface);
#endif

    return 0;
}
