/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Camera
*/

#include "rt/scene/Camera.hpp"
#include <iostream>
#include <cmath>

Camera::Camera(const Vec3 &position, const Vec3 &lookAt, const Vec3 &up,
               double fov, int width, int height)
    : position(position), lookAt(lookAt), up(up), fov(fov),
      width(width), height(height), 
      aperture(0.0), focusDistance(1.0), useDOF(false)
{    
    double aspectRatio = static_cast<double>(width) / height;
    double theta = fov * M_PI / 180.0;
    double h = std::tan(theta / 2.0);
    double viewportHeight = 2.0 * h;
    double viewportWidth = aspectRatio * viewportHeight;

    w = (position - lookAt).normalized();
    u = up.cross(w).normalized();
    v = w.cross(u);

    horizontal = u * viewportWidth;
    vertical = v * viewportHeight;
    lowerLeft = position - horizontal / 2.0 - vertical / 2.0 - w;
}

Ray Camera::generateRay(int x, int y) const
{
    double u = static_cast<double>(x) / (width - 1);
    double v = static_cast<double>(y) / (height - 1);
    
    Vec3 target = lowerLeft + horizontal * u + vertical * v;
    Vec3 direction = (target - position).normalized();
    
    return Ray(position, direction);
}

Ray Camera::generateRay(double x, double y) const
{
    double u = x / (width - 1);
    double v = y / (height - 1);
    
    Vec3 target = lowerLeft + horizontal * u + vertical * v;
    Vec3 direction = (target - position).normalized();
    
    return Ray(position, direction);
}

Ray Camera::generateRay(double x, double y, std::mt19937 &rng) const
{
    double u = x / (width - 1);
    double v = y / (height - 1);
    
    Vec3 target = lowerLeft + horizontal * u + vertical * v;
    Vec3 rayDirection = target - position;
    
    if (!useDOF || aperture <= 0.0) {
        return Ray(position, rayDirection.normalized());
    }
    
    double rayLength = rayDirection.length();
    Vec3 rayDir = rayDirection / rayLength;
    
    Vec3 focusPoint = position + rayDir * focusDistance;
    
    Vec3 rd = randomInUnitDisk(rng) * (aperture / 2.0);
    Vec3 offset = this->u * rd.x + this->v * rd.y;
    Vec3 origin = position + offset;
    
    Vec3 direction = (focusPoint - origin).normalized();
    
    return Ray(origin, direction);
}

void Camera::setDepthOfField(double aperture, double focusDistance)
{
    this->aperture = aperture;
    this->focusDistance = focusDistance;
    this->useDOF = (aperture > 0.0);
}

Vec3 Camera::randomInUnitDisk(std::mt19937 &rng) const
{
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    
    while (true) {
        double angle = dis(rng) * 2.0 * M_PI;
        double radius = std::sqrt(dis(rng));
        
        double x = radius * std::cos(angle);
        double y = radius * std::sin(angle);
        
        return Vec3(x, y, 0);
    }
}

void Camera::updateViewMatrices()
{
    double aspectRatio = static_cast<double>(width) / height;
    double theta = fov * M_PI / 180.0;
    double h = std::tan(theta / 2.0);
    double viewportHeight = 2.0 * h;
    double viewportWidth = aspectRatio * viewportHeight;

    w = (position - lookAt).normalized();
    u = up.cross(w).normalized();
    v = w.cross(u);

    horizontal = u * viewportWidth;
    vertical = v * viewportHeight;
    lowerLeft = position - horizontal / 2.0 - vertical / 2.0 - w;
}

void Camera::moveForward(double distance)
{
    Vec3 forward = (lookAt - position).normalized();
    position += forward * distance;
    lookAt += forward * distance;
    updateViewMatrices();
}

void Camera::moveBackward(double distance)
{
    Vec3 forward = (lookAt - position).normalized();
    position -= forward * distance;
    lookAt -= forward * distance;
    updateViewMatrices();
}

void Camera::moveLeft(double distance)
{
    Vec3 right = u;
    position -= right * distance;
    lookAt -= right * distance;
    updateViewMatrices();
}

void Camera::moveRight(double distance)
{
    Vec3 right = u;
    position += right * distance;
    lookAt += right * distance;
    updateViewMatrices();
}

void Camera::moveUp(double distance)
{
    Vec3 upVec = v;
    position += upVec * distance;
    lookAt += upVec * distance;
    updateViewMatrices();
}

void Camera::moveDown(double distance)
{
    Vec3 upVec = v;
    position -= upVec * distance;
    lookAt -= upVec * distance;
    updateViewMatrices();
}

void Camera::rotateYaw(double angle)
{
    double radians = angle * M_PI / 180.0;
    Vec3 direction = lookAt - position;
    double distance = direction.length();
    
    double cosAngle = std::cos(radians);
    double sinAngle = std::sin(radians);
    
    Vec3 dirNorm = direction.normalized();
    Vec3 newDir = dirNorm * cosAngle + v.cross(dirNorm) * sinAngle + v * (v.dot(dirNorm)) * (1.0 - cosAngle);
    
    lookAt = position + newDir * distance;
    printf("%f, %f, %f\n", lookAt.x, lookAt.y, lookAt.z);
    updateViewMatrices();
}

void Camera::rotatePitch(double angle)
{
    double radians = angle * M_PI / 180.0;
    Vec3 direction = lookAt - position;
    double distance = direction.length();
    
    double cosAngle = std::cos(radians);
    double sinAngle = std::sin(radians);
    
    Vec3 dirNorm = direction.normalized();
    Vec3 newDir = dirNorm * cosAngle + u.cross(dirNorm) * sinAngle + u * (u.dot(dirNorm)) * (1.0 - cosAngle);
    
    lookAt = position + newDir * distance;
    updateViewMatrices();
}
