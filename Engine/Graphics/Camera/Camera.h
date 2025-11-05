/**
 * @file Camera.h
 * @brief Provides utility functions for managing camera behavior and transformations.
 *
 * This file contains a collection of inline functions for manipulating camera components,
 * including movement, orientation, and projection. It supports both orbiting and walking
 * camera types and integrates with shader programs for rendering.
 *
 * @author Zhang TongYan
 * @date Sept, 2025
 */
#ifndef CAMERA_H
#define CAMERA_H

#include "ShaderProgram.h"

namespace Camera {
    //Returns the position in front of the camera, where it is looking.
    inline glm::vec3 GetFowardLookPosition(const CameraComponent& cam, float dist)
    {
        glm::vec3 look_vector = cam.target - cam.position;
        //potential edge case where camera look direction may be 0. 
        if (look_vector.length() <= 0.01) return cam.position;
        dist /= look_vector.length();
        return cam.position + dist * look_vector;
    }
    inline void Move_Cam_and_Target(CameraComponent& cam, glm::vec3 offset) {

        LOGD("Camera Target: %f, %f, %f", cam.target.x, cam.target.y, cam.target.z);
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

        glm::vec3 front = glm::normalize(cam.target - cam.position);      // view direction
        glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
        glm::vec3 up = glm::normalize(glm::cross(right, front));

        // Combine local-axis moves into one world-space translation
        glm::vec3 t = right * offset.x + up * offset.y + front * offset.z;

        // Apply translation; choose whether to pan (move target too) or not
        cam.position += t;
        cam.target += t;
    }

    // Sets the camera type and updates the target if it's orbiting
    inline void setType(CameraComponent& cam, CameraComponent::CameraType cameraType)
    {
        cam.camType = cameraType;
        if (cameraType == CameraComponent::CameraType::ORBITING)
            cam.target = { 0.0f, 0.0f, 0.0f }; // Orbiting cameras always target the origin
    }

    inline void set_Cam_Pos(CameraComponent& cam, glm::vec3 pos) {
        cam.position = pos;
    }

    inline void set_Target_Pos(CameraComponent& cam, glm::vec3 pos) {
        cam.target = pos;
    }

    // Updates the shader program with camera position
    inline void setUniforms(CameraComponent& cam, std::shared_ptr<ShaderProgram> pProgram)
    {
        pProgram->setUniform("camera.position", cam.position); // Pass camera position to the shader
    }
    // Returns the view matrix for the camera using the "look at" method
    inline glm::mat4 getLookAt(CameraComponent& cam, glm::vec3 center = { 0.0f, 0.0f, 0.0f }, glm::vec3 up = { 0.0f, 1.0f, 0.0f })
    {
        UNREFERENCED_PARAMETER(center);
        cam.view_vector = glm::lookAt(cam.position, cam.target, up);
        return cam.view_vector;
    }

    // Returns the perspective projection matrix based on the field of view and aspect ratio
    inline glm::mat4 getPerspective(CameraComponent& cam, float aspect = 1)
    {
        cam.proj_matrix = glm::perspective(glm::radians(cam.fieldOfView), aspect, cam.nearPlane, cam.farPlane);
        return cam.proj_matrix;
    }

    // Handles cursor movement to adjust camera orientation
    inline void onCursor(CameraComponent& cam, double xoffset, double yoffset)
    {
        const float PI05 = glm::pi<float>() / 2.0f; // Half of pi for clamping

        if (cam.camType == CameraComponent::CameraType::ORBITING)
        {
            // Calculate spherical coordinates for orbiting movement
            const float r = glm::sqrt(cam.position.x * cam.position.x +
                cam.position.y * cam.position.y + cam.position.z * cam.position.z);
            float alpha = glm::asin(cam.position.y / r); // Vertical angle
            float betta = std::atan2f(cam.position.x, cam.position.z); // Horizontal angle

            // Adjust angles based on cursor offset
            /*if (yoffset < 0.0)
                alpha += -0.02f;
            else if (yoffset > 0.0)
                alpha += 0.02f;

            if (xoffset < 0.0)
                betta += 0.05f;
            else if (xoffset > 0.0)
                betta += -0.05f;*/

            const float yaw_sens = 0.005f;
            const float pitch_sens = 0.003f;
            betta -= (float)xoffset * yaw_sens;    // flip to "-xoffset" if you prefer the opposite
            alpha -= (float)-yoffset * pitch_sens;  // minus makes up = up

            // Clamp vertical angle
            alpha = glm::clamp(alpha, -PI05 + 0.01f, PI05 - 0.01f);

            // Update position based on spherical coordinates
            cam.position.x = r * glm::cos(alpha) * glm::sin(betta);
            cam.position.y = r * glm::sin(alpha);
            cam.position.z = r * glm::cos(alpha) * glm::cos(betta);
        }
        else if (cam.camType == CameraComponent::CameraType::WALKING)
        {
            // Calculate spherical coordinates for walking movement
            const float r = glm::sqrt(
                (cam.target.x - cam.position.x) * (cam.target.x - cam.position.x) +
                (cam.target.y - cam.position.y) * (cam.target.y - cam.position.y) +
                (cam.target.z - cam.position.z) * (cam.target.z - cam.position.z));
            float alpha = glm::asin((cam.target.y - cam.position.y) / r);
            float betta = std::atan2f((cam.target.x - cam.position.x), (cam.target.z - cam.position.z));

            // Adjust angles based on cursor offset
            /*if (yoffset < 0.0)
                alpha += -0.02f;
            else if (yoffset > 0.0)
                alpha += 0.02f;

            if (xoffset < 0.0)
                betta += -0.05f;
            else if (xoffset > 0.0)
                betta += 0.05f;*/

            const float yaw_sens = 0.005f;
            const float pitch_sens = 0.003f;
            betta -= (float)xoffset * yaw_sens;    // flip to "-xoffset" if you prefer the opposite
            alpha += (float)-yoffset * pitch_sens;  // minus makes up = up

            // Clamp vertical angle
            alpha = glm::clamp(alpha, -PI05 + 0.01f, PI05 - 0.01f);

            // Update target based on spherical coordinates
            cam.target.x = cam.position.x + r * glm::cos(alpha) * glm::sin(betta);
            cam.target.y = cam.position.y + r * glm::sin(alpha);
            cam.target.z = cam.position.z + r * glm::cos(alpha) * glm::cos(betta);
        }
        //UNREFERENCED_PARAMETER(pProgram);
        // Update shader program with the new camera settings
        //setUniforms(pProgram);
    }

    // Handles scroll input to adjust zoom or camera position
    inline void onScroll(CameraComponent& cam, double xoffset, double yoffset)
    {
        UNREFERENCED_PARAMETER(xoffset);
        if (cam.camType == CameraComponent::CameraType::ORBITING)
        {
            // Calculate the distance from the origin and adjust it based on scroll input
            float r = glm::sqrt(cam.position.x * cam.position.x + cam.position.y * cam.position.y + cam.position.z * cam.position.z);
            const float alpha = glm::asin(cam.position.y / r);
            const float betta = std::atan2f(cam.position.x, cam.position.z);

            r += yoffset > 0.0f ? -1.0f : 1.0f; // Zoom in or out
            if (r < 1.0f) r = 1.0f; // Clamp minimum distance

            // Update position based on new distance
            cam.position.x = r * glm::cos(alpha) * glm::sin(betta);
            cam.position.y = r * glm::sin(alpha);
            cam.position.z = r * glm::cos(alpha) * glm::cos(betta);
        }
        else if (cam.camType == CameraComponent::CameraType::WALKING)
        {
            // Adjust position and target based on scroll input for walking
            const glm::vec3 velocity = glm::normalize(yoffset > 0.0f ? cam.target - cam.position : cam.position - cam.target);
            cam.position += velocity * glm::vec3(1.0f, 0.0f, 1.0f);
            cam.target += velocity * glm::vec3(1.0f, 0.0f, 1.0f);
        }
        //UNREFERENCED_PARAMETER(pProgram);
        // Update shader program with the new camera settings
        //setUniforms(pProgram);
    }
}

#endif // CAMERA_H