#pragma once

struct Vec4f {
    float x;
    float y;
    float z;
    float w;

    Vec4f(float x, float y, float z, float w = 1.f)
        : x(x)
        , y(y)
        , z(z)
        , w(w){}

};

