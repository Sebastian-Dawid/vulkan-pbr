#pragma once

#include <glm/glm.hpp>

enum camera_movement_t
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

struct camera_t
{
    private:
        void update_vectors();
    public:
        glm::vec3 position;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 world_up;

        float yaw;
        float pitch;

        float speed;
        float sensitivity;
        float fov_angle;

        glm::mat4 calculate_view_matrix();
        void move(camera_movement_t direction, float delta_time);
        void turn(float yaw, float pitch, bool constrain = true);
        void zoom(float offset);
        camera_t(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);
};
