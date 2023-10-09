#include "camera.h"
#include "glm/ext/matrix_transform.hpp"

camera_t::camera_t(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
{
    this->position = position;
    this->world_up = up;
    this->yaw = yaw;
    this->pitch = pitch;
    this->fov_angle = 45.0f;
    this->speed = 2.5f;
    this->sensitivity = 0.1f;
    this->front = glm::vec3(0.0f, 0.0f, -1.0f);
    update_vectors();
}

glm::mat4 camera_t::calculate_view_matrix()
{
    return glm::lookAt(this->position, this->position + this->front, this->up);
}

void camera_t::move(camera_movement_t direction, float delta_time)
{
    float velocity = this->speed * delta_time;
    switch (direction)
    {
        case FORWARD:
            this->position += this->front * velocity;
            break;
        case BACKWARD:
            this->position -= this->front * velocity;
            break;
        case LEFT:
            this->position -= this->right * velocity;
            break;
        case RIGHT:
            this->position += this->right * velocity;
            break;
    }
}

void camera_t::turn(float xoffset, float yoffset, bool constrain)
{
    xoffset *= this->sensitivity;
    yoffset *= this->sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (constrain)
    {
        this->pitch = (this->pitch >  89.0f) ?  89.0f : this->pitch;
        this->pitch = (this->pitch < -89.0f) ? -89.0f : this->pitch;
    }
    update_vectors();
}

void camera_t::zoom(float offset)
{
    this->fov_angle -= offset;
    this->fov_angle = (this->fov_angle <  1.0f) ?  1.0f : this->fov_angle;
    this->fov_angle = (this->fov_angle > 45.0f) ? 45.0f : this->fov_angle;
}

void camera_t::update_vectors()
{
    this->front = glm::normalize(glm::vec3(
            std::cos(glm::radians(this->yaw)) * std::cos(glm::radians(this->pitch)),
            std::sin(glm::radians(this->pitch)),
            std::sin(glm::radians(this->yaw)) * std::cos(glm::radians(this->pitch))
            ));
    this->right = glm::normalize(glm::cross(this->front, this->world_up));
    this->up    = glm::normalize(glm::cross(this->right, this->front));
}
