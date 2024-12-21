#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

inline const float YAW = -90.00f;
inline const float PITCH = -37.68f;
inline const float SPEED = 5.0f;
inline const float SENSITIVITY = 0.1f;
inline const float ZOOM = 45.0f;

struct Camera {
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 SavedPosition; // To store the position when enabling free camera
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // Rotation
    float Yaw;
    float Pitch;

    float savedYaw;
    float savedPitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    bool LockYAxis = true;

    // Constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH, bool lockYAxis = true) 
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), LockYAxis(lockYAxis) 
    {
        Position = position;
        SavedPosition = position; // Initialize saved position
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Processes input received from any keyboard-like input system
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;

        glm::vec3 movementFront = Front;
        // Lock movement direction to the XZ plane if Y-axis is locked
        if (LockYAxis) {
            movementFront.y = 0.0f;
            movementFront = glm::normalize(movementFront);
        }

        if (direction == FORWARD)
            Position += movementFront * velocity;
        if (direction == BACKWARD)
            Position -= movementFront * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;

        if (!LockYAxis) {
            if (direction == UP)
                Position += Up * velocity;
            if (direction == DOWN)
                Position -= Up * velocity;
        }
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

    void enableFreeCamera() {
        if(LockYAxis) {
            SavedPosition = Position;
            savedYaw = Yaw;
            savedPitch = Pitch;
        } 
        LockYAxis = false;
    }

    void disableFreeCamera() {
        Position = SavedPosition;
        Yaw = savedYaw;
        Pitch = savedPitch; 
        LockYAxis = true;
    }

private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        // Also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

