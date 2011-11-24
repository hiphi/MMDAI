const int kBoneIndices = 128;
uniform mat4 boneMatrices[kBoneIndices];
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 biasMatrix;
uniform mat3 normalMatrix;
uniform vec4 lightColor;
uniform vec4 materialAmbient;
uniform vec4 materialDiffuse;
uniform bool isMainSphereMap;
uniform bool isSubSphereMap;
attribute vec4 inPosition;
attribute vec3 inNormal;
attribute vec2 inTexCoord;
attribute vec2 inToonTexCoord;
attribute float inFirstBoneIndex;
attribute float inSecondBoneIndex;
attribute float inBoneWeight;
varying vec4 outColor;
varying vec4 outShadowTexCoord;
varying vec2 outMainTexCoord;
varying vec2 outSubTexCoord;
varying vec2 outToonTexCoord;
const float kTwo = 2.0;
const float kOne = 1.0;
const float kHalf = 0.5;
const vec4 kOne4 = vec4(kOne, kOne, kOne, kOne);

vec4 doSkinning(vec4 position, mat4 matrix1, mat4 matrix2) {
    return inBoneWeight * (matrix1 * position) + (1.0 - inBoneWeight) * (matrix2 * position);
}

vec2 makeSphereMap(vec3 position, vec3 normal) {
    vec3 R = reflect(position, normal);
    float M = kTwo * sqrt(R.x * R.x + R.y * R.y + (R.z + kOne) * (R.z + kOne));
    return vec2(R.x / M + kHalf, R.y / M + kHalf);
}

void main() {
    mat4 matrix1 = boneMatrices[int(inFirstBoneIndex)];
    mat4 matrix2 = boneMatrices[int(inSecondBoneIndex)];
    vec4 position = modelViewMatrix * doSkinning(inPosition, matrix1, matrix2);
    vec3 view = normalize(position.xyz);
    vec3 normal = normalize(normalMatrix * doSkinning(vec4(inNormal, 0.0), matrix1, matrix2).xyz);
    vec4 color = min(materialAmbient + lightColor * materialDiffuse, kOne4);
    vec4 outPosition = projectionMatrix * position;
    outColor = color;
    outShadowTexCoord = biasMatrix * outPosition;
    outMainTexCoord = isMainSphereMap ? makeSphereMap(view, normal) : inTexCoord;
    outSubTexCoord = isSubSphereMap ? makeSphereMap(view, normal) : inTexCoord;
    outToonTexCoord = inToonTexCoord;
    gl_Position = outPosition;
}

