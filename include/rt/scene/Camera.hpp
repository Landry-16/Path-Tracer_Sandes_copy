/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** Camera
*/

#ifndef RT_CAMERA_HPP
    #define RT_CAMERA_HPP

    #include "rt/math/Vec3.hpp"
    #include "rt/math/Ray.hpp"
    #include <random>

class Camera {
public:
    Camera(const Vec3 &position, const Vec3 &lookAt, const Vec3 &up, 
           double fov, int width, int height);

    Ray generateRay(int x, int y) const;
    Ray generateRay(double x, double y) const;
    Ray generateRay(double x, double y, std::mt19937 &rng) const;

    void setDepthOfField(double aperture, double focusDistance);
    
    void moveForward(double distance);
    void moveBackward(double distance);
    void moveLeft(double distance);
    void moveRight(double distance);
    void moveUp(double distance);
    void moveDown(double distance);
    
    void rotateYaw(double angle);
    void rotatePitch(double angle);
    

    Vec3 getPosition() const { return position; }
    Vec3 getForward() const { return (w * -1.0); }
    Vec3 getRight() const { return u; }
    Vec3 getUp() const { return v; }

private:
    Vec3 randomInUnitDisk(std::mt19937 &rng) const;
    void updateViewMatrices();

    Vec3 position;
    Vec3 lookAt;
    Vec3 up;
    double fov;
    Vec3 lowerLeft;
    Vec3 horizontal;
    Vec3 vertical;
    Vec3 u, v, w;
    int width;
    int height;
    
    double aperture;
    double focusDistance;
    bool useDOF;
};

#endif // RT_CAMERA_HPP
