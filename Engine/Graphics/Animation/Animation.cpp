#include "pch.hpp"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Animation.hpp"
#include <glm/gtx/string_cast.hpp>
#include <stdio.h>

namespace EngineFramework
{
    // Small helper function
    static inline int clampInt(int v, int low, int high) {
        return std::max(low, std::min(v, high));
    }

    //static void LogMat4(const glm::mat4& m)
    //{
    //    LOGI("[%.6f %.6f %.6f %.6f]", m[0][0], m[1][0], m[2][0], m[3][0]);
    //    LOGI("[%.6f %.6f %.6f %.6f]", m[0][1], m[1][1], m[2][1], m[3][1]);
    //    LOGI("[%.6f %.6f %.6f %.6f]", m[0][2], m[1][2], m[2][2], m[3][2]);
    //    LOGI("[%.6f %.6f %.6f %.6f]", m[0][3], m[1][3], m[2][3], m[3][3]);
    //}

    static keyFrame Blend(const keyFrame& a, const keyFrame& b, float t)
    {
        keyFrame r;
        r.scale = glm::mix(a.scale, b.scale, t);
        r.rotation = glm::normalize(glm::slerp(a.rotation, b.rotation, t));
        r.position = glm::mix(a.position, b.position, t);

        // Pre/post/rotationOrder are constant per bone
        r.preRotDeg = a.preRotDeg;
        r.postRotDeg = a.postRotDeg;
        r.rotOrder = a.rotOrder;

        return r;
    }

    void AnimationPlayer::update(float dt) {
        // If Animation exists
        if (!package) return;
        // Else if invalid index is given
        if (currentAnimIndex < 0 || currentAnimIndex >= (int)package->animations.size()) return;

        // Run the animation
        const Animation& anim = package->animations[currentAnimIndex];

        // Validate if the clip has ended
        if (anim.duration <= 0.0f) return;

        time += dt;

        // Ensure proper looping
        while (time >= anim.duration) time -= anim.duration;
        while (time < 0.0f) time += anim.duration;
    }

    void AnimationPlayer::logBoneTransforms(const Skeleton& skeleton, const std::vector<glm::mat4>& globals) const
    {
        int root = -1, hip = -1, knee = -1, ankle = -1;
        for (int i = 0; i < skeleton.bones.size(); ++i) {
            if (skeleton.bones[i].name == "Root") root = i;
            if (skeleton.bones[i].name == "Hip") hip = i;
            if (skeleton.bones[i].name == "Knee") knee = i;
            if (skeleton.bones[i].name == "Ankle") ankle = i;
        }

        if (root != -1) printf("Root global: %s\n", glm::to_string(globals[root]).c_str());
        if (hip != -1) printf("Hip global: %s\n", glm::to_string(globals[hip]).c_str());
        if (knee != -1) printf("Knee global: %s\n", glm::to_string(globals[knee]).c_str());
        if (ankle != -1) printf("Ankle global: %s\n", glm::to_string(globals[ankle]).c_str());
    }

    void AnimationPlayer::computeMatrices(std::vector<glm::mat4>& finalMatrices, const glm::mat4& globalRootTransform, const Skeleton& skeleton) const {
        if (!package || package->animations.empty())
            return;

        if (currentAnimIndex < 0 ||
            currentAnimIndex >= static_cast<int>(package->animations.size()))
        {
            // LOGE is not defined, so I will comment it out
            // LOGE("computeMatrices: currentAnimIndex %d out of range (clips=%zu)",
            //     currentAnimIndex, package->animations.size());
            return;
        }

        const Animation& anim = package->animations[currentAnimIndex];

        const int boneCount = skeleton.count();
        const int clipBones = static_cast<int>(anim.boneKeyframes.size());
        const int numFrames = anim.numFrames();

        if (boneCount == 0 || numFrames == 0)
            return;

        finalMatrices.resize(boneCount);

        // --- frame indices + blend ---
        float frameTime = time * anim.fps;
        frameTime = fmodf(frameTime, static_cast<float>(numFrames));

        int   frame0 = static_cast<int>(floorf(frameTime));
        int   frame1 = (frame0 + 1) % numFrames;
        float blend = frameTime - static_cast<float>(frame0);

        // --- local (bone-space) transforms ---
        std::vector<glm::mat4> locals(boneCount);
        for (int i = 0; i < boneCount; ++i)
        {
            if (i < clipBones && !anim.boneKeyframes[i].frames.empty())
            {
                const BoneKeyframes& kf = anim.boneKeyframes[i];
                const int kfCount = static_cast<int>(kf.frames.size());

                const keyFrame& a = kf.frames[frame0 % kfCount];
                const keyFrame& b = kf.frames[frame1 % kfCount];

                keyFrame blended = Blend(a, b, blend);

				locals[i] = blended.toMatrix();

            //    if (skeleton.bones[i].parentIndex != -1) {
            //        // Decompose neutralPose
            //        glm::vec3 s_bind, t_bind;
            //        glm::quat r_bind;
            //        glm::vec3 skew;
            //        glm::vec4 persp;
            //        glm::decompose(skeleton.bones[i].neutralPose, s_bind, r_bind, t_bind, skew, persp);

            //        // Decompose animated transform
            //        glm::vec3 s_anim, t_anim;
            //        glm::quat r_anim;
            //        glm::decompose(blended.toMatrix(), s_anim, r_anim, t_anim, skew, persp);

            //        // Combine
            //        glm::vec3 t_final = t_bind + t_anim;
            //        glm::quat r_final = r_bind * r_anim;
            //        glm::vec3 s_final = s_bind * s_anim;

            //        // Recompose
            //        glm::mat4 T = glm::translate(glm::mat4(1.0f), t_final);
            //        glm::mat4 R = glm::mat4_cast(r_final);
            //        glm::mat4 S = glm::scale(glm::mat4(1.0f), s_final);
            //        locals[i] = T * R * S;

            //    } else {
            //        locals[i] = blended.toMatrix();
            //    }
            }
            else
            {
                locals[i] = glm::mat4(1.0f);
            }
        }

        // --- global (model-space) transforms, with recursion ---
        std::vector<glm::mat4> globals(boneCount);
        std::vector<bool>      built(boneCount, false);

        std::function<void(int)> buildGlobal = [&](int idx)
            {
                if (built[idx]) return;

                int parent = skeleton.bones[idx].parentIndex;
                if (parent < 0 || parent >= boneCount || parent == idx)
                {
                    globals[idx] = locals[idx];
                }
                else
                {
                    buildGlobal(parent);
                    globals[idx] = globals[parent] * locals[idx];
                }
                built[idx] = true;
            };

        for (int i = 0; i < boneCount; ++i)
            buildGlobal(i);

        // logBoneTransforms(skeleton, globals); // Commenting out for now to avoid spamming the console

        // --- final skinning matrices ---
        for (int i = 0; i < boneCount; ++i)
        {
            finalMatrices[i] = globalRootTransform * globals[i] * skeleton.bones[i].invBind;
        }
    }
}