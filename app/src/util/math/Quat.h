#pragma once
#include <math.h>

#include <type_traits>

#include "src/util/math/Mat4.h"

/**
 * Implementation of a Quaternion.
 */
template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
struct Quat {
    T x;
    T y;
    T z;
    T w;

    /**
     * Initializes an identity rotation.
     */
    Quat() : x(0), y(0), z(0), w(1) {}

    /**
     * Initializes the quaternion with the given values.
     */

    Quat(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    /**
     * Initializes the quaternion with the given values and normalizes it, if wished.
     */
    Quat(T x, T y, T z, T w, bool normalize)
        : x(x), y(y), z(z), w(w)
    {
        if (normalize) {
            T mag = sqrt(x * x + y * y + z * z + w * w);
            if (mag > 0.0f) {
                x = x / mag;
                y = y / mag;
                z = z / mag;
                w = w / mag;
            } else {
                x = 0;
                y = 0;
                z = 0;
                w = 1;
            }
        }
    }

    /**
     * Initializes the quaternion from the given axis and angle.
     */

    Quat(Vec4<T> axis, T angle) {
        T norm = axis.length();
        if (norm < 1E-6f) {
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
            w = 1.0f;
        } else {
            T sin_half_angle = sin(angle / 2.0f);
            x = sin_half_angle * axis[0] / norm;
            y = sin_half_angle * axis[1] / norm;
            z = sin_half_angle * axis[2] / norm;
            w = cos(angle / 2.0f);
        }
    }

    Quat(T pitch, T yaw, T roll){
        Quat qx = Quat(Vec4f(1,0,0), pitch);
        Quat qy = Quat(Vec4f(0,1,0), yaw);
        Quat qz = Quat(Vec4f(0,0,1), roll);

        return qy * qz * qx;
    }

    /**
     * Initializes the quaterion so that it represents a rotation which
     * rotates the from-vector so that it results in the to-vector
     */
    Quat(Vec4<T> from, Vec4<T> to) {
        T fromSqNorm = from.squaredLength();
        T toSqNorm = to.squaredLength();

        // Check if `from` and `to` are valid:
        if ((fromSqNorm < 1E-10f) || (toSqNorm < 1E-10f)) {
            x = y = z = 0.0f;
            w = 1.0f;
        } else {
            Vec4<T> axis = from.cross(to);

            T axisSqNorm = axis.squaredLength();
            T angle = asin(sqrt(axisSqNorm / (fromSqNorm * toSqNorm)));

            if (from.dot(to) < 0.0)
                angle = (M_PI - angle);

            fromAxisAngle(axis, angle);
        }
    }

    /**
     * Creates a quaternion which represents the same rotation as the
     * given rotation matrix.
     */
    Quat(Mat4<T> rotationMatrix){
        T* m = rotationMatrix.data;

        T onePlusTrace = 1.0f + m[0] + m[5] + m[10];

        if (onePlusTrace > 1E-5f) {
            T s = (sqrt(onePlusTrace) * 2.0f);
            x = (m[6] - m[9]) / s;
            y = (m[8] - m[2]) / s;
            z = (m[1] - m[4]) / s;
            w = 0.25f * s;
        } else {
            if ((m[0] > m[5]) & (m[0] > m[10])) {
                T s = (sqrt(1.0f + m[0] - m[5] - m[10]) * 2.0f);
                x = 0.25f * s;
                y = (m[4] + m[1]) / s;
                z = (m[8] + m[2]) / s;
                w = (m[9] - m[6]) / s;
            } else if (m[5] > m[10]) {
                T s = (sqrt(1.0f + m[5] - m[0] - m[10]) * 2.0f);
                x = (m[4] + m[1]) / s;
                y = 0.25f * s;
                z = (m[9] + m[6]) / s;
                w = (m[8] - m[2]) / s;
            } else {
                T s = (sqrt(1.0f + m[10] - m[0] - m[5]) * 2.0f);
                x = (m[8] + m[2]) / s;
                y = (m[9] + m[6]) / s;
                z = 0.25f * s;
                w = (m[4] - m[1]) / s;
            }
        }
        normalize();
    }

    /**
     * Negates all the coefficients of the Quaternion.
     */
    Quat operator-(){
        return Quat(-x,-y,-z,-w);
    }

    /**
     * Dot product of this quaternion with the given quaternion.
     */
    T dot(Quat q) {
        return x * q.x + y * q.y + z * q.z + w * q.w;
    }

    /**
     * Multiplies this quaternion with the given one.
     */
    Quat operator*(Quat q1) {
        T x = w * q1.x + q1.w * x + y * q1.z - z * q1.y;
        T y = w * q1.y + q1.w * y - x * q1.z + z * q1.x;
        T z = w * q1.z + q1.w * z + x * q1.y - y * q1.x;
        T w = w * q1.w - x * q1.x - y * q1.y - z * q1.z;

        return Quat(x,y,z,w);
    }

    /**
     * Rotates the given vector by using this quaternion.
     */
    Vec4<T> operator*(Vec4<T> v) {
        return rotate(v);
    }

    /**
     * Returns the inverse quaternion.
     */
    Quat inverse() {
        Quat tempQuat(*this);
        tempQuat.invert();
        return tempQuat;
    }

    /**
     * Returns the inverse quaternion.
     */
    void invert() {
        T sqNorm = squaredNorm();
        w /= sqNorm;
        x /= -sqNorm;
        y /= -sqNorm;
        z /= -sqNorm;
    }

    /**
     * Normalizes this quaternion.
     */
    T normalize() {
        T norm = sqrt(squaredNorm());
        if (norm > 0.0f) {
            x /= norm;
            y /= norm;
            z /= norm;
            w /= norm;
        } else {
            x = 0.0;
            y = 0.0;
            z = 0.0;
            w = 1.0;
        }
        return norm;
    }

    /**
     * Returns the normalized quaternion.
     */
    Quat<T> normalized(){
        T norm = sqrt(squaredNorm());

        if (norm > 0.0f) {
            return Quat<T>(x /= norm, y /= norm, z /= norm, w /= norm);
        }

        return Quat<T>();
    }

    /**
     * Rotates the given vector v and returns it.
     */
    Vec4<T> rotate(Vec4<T> v) {
        T q00 = 2.0f * x * x;
        T q11 = 2.0f * y * y;
        T q22 = 2.0f * z * z;

        T q01 = 2.0f * x * y;
        T q02 = 2.0f * x * z;
        T q03 = 2.0f * x * w;

        T q12 = 2.0f * y * z;
        T q13 = 2.0f * y * w;

        T q23 = 2.0f * z * w;

        T x = (1.0f - q11 - q22) * v[0] + (q01 - q23) * v[1] + (q02 + q13) * v[2];
        T y = (q01 + q23) * v[0] + (1.0f - q22 - q00) * v[1] + (q12 - q03) * v[2];
        T z = (q02 - q13) * v[0] + (q12 + q03) * v[1] + (1.0f - q11 - q00) * v[2];

        return Vec4<T>(x,y,z,0);
    }

    /**
     * Returns euler angles in a Vec4 vector.
     */
    Vec4<T> eulerAngles() {
        T test = x * y + z * w;
        if (test > 0.499) {
            return Vec4<T>(0, 2 * atan2(x, w), M_PI / 2);
        }

        if (test < -0.499) {
            return Vec4<T>(0, -2 * atan2(x, w), -M_PI / 2);
        }

        T sqx = x * x;
        T sqy = y * y;
        T sqz = z * z;
        T x = atan2(2 * x * w - 2 * y * z, 1 - 2 * sqx - 2 * sqz);
        T y = atan2(2 * y * w - 2 * x * z, 1 - 2 * sqy - 2 * sqz);
        T z = asin(2 * test);
    }

    /**
     * Returns the normalized axis direction of this quaternion.
     */
    Vec4<T> axis(){
        Vec4<T> result;
        result.x = x;
        result.y = y;
        result.z = z;

        T sinus = result.length();
        if (sinus > 1E-8f)
            result /= sinus;

        if (acos(w) > M_PI/2){
            result[0] = -result[0];
            result[1] = -result[1];
            result[2] = -result[2];
        }
    }

    /**
     * Returns the angle (in radians) of this Quaternion.
     */
    T angle() {
        T angle = (2.0f * acos(w));
        return (angle <= M_PI) ? angle : (2.0f * M_PI - angle);
    }


    /**
     * Returns the rotation matrix which represents the same rotation as
     * this quaternion does.
     */
    Mat4<T> rotationMatrix() {
        T q00 = 2.0f * x * x;
        T q11 = 2.0f * y * y;
        T q22 = 2.0f * z * z;

        T q01 = 2.0f * x * y;
        T q02 = 2.0f * x * z;
        T q03 = 2.0f * x * w;

        T q12 = 2.0f * y * z;
        T q13 = 2.0f * y * w;

        T q23 = 2.0f * z * w;

        T m00 = 1.0f - q11 - q22;
        T m01 = q01 - q23;
        T m02 = q02 + q13;

        T m10 = q01 + q23;
        T m11 = 1.0f - q22 - q00;
        T m12 = q12 - q03;

        T m20 = q02 - q13;
        T m21 = q12 + q03;
        T m22 = 1.0f - q11 - q00;

        T m30 = 0.0f;
        T m31 = 0.0f;
        T m32 = 0.0f;

        T m03 = 0.0f;
        T m13 = 0.0f;
        T m23 = 0.0f;
        T m33 = 1.0f;

        return Mat4<T>({
            m00, m10, m20, m30,
            m01, m11, m21, m31,
            m02, m12, m22, m32,
            m03, m13, m23, m33
        });
    }



    /**
     * Spherical linear interpolation from Quaternion a to Quaternion b, where
     * t represents the `time` or `progress` between 0 and 1.
     *
     * If allowFlip is true, always the shortest path is being used (by flipping
     * the source quaternion.
     *
     */
    static Quat slerp(Quat a, Quat b, T t, bool allowFlip = true) {
        // Warning: this method should not normalize the Quaternion
        T cosAngle = a.dot(b);

        T c1, c2;
        // Linear interpolation for close orientations
        if ((1.0 - abs(cosAngle)) < 0.01) {
            c1 = 1.0f - t;
            c2 = t;
        } else {
            // Spherical interpolation
            T angle = acos(abs(cosAngle));
            T sinAngle = sin(angle);
            c1 = (sin(angle * (1.0f - t)) / sinAngle);
            c2 = (sin(angle * t) / sinAngle);
        }

        // Use the shortest path
        if (allowFlip && (cosAngle < 0.0))
            c1 = -c1;

        return Quat(c1 * a.x + c2 * b.x, c1 * a.y + c2 * b.y, c1 * a.z + c2 * b.z, c1 * a.w + c2 * b.w);
    }

    /**
     * Utility function that returns the squared norm of the Quaternion.
     */
    T squaredNorm() {
        return (x * x) + (y * y) + (z * z) + (w * w);
    }

    /**
     * Adds the given Quat to an output stream.
     */
    friend std::ostream& operator<<(std::ostream& os, Quat<T> q){
        os << "Q(" << q.x << ", " << q.y << ", " << q.z << ", " << q.w << ")";
        return os;
    }
};

typedef Quat<float> Quatf;
typedef Quat<double> Quatd;
