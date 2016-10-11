#include <cmath> // fabs, sqrt
#include <cfloat> // FLT_MAX FLT_MIN

struct float2
{
    float x, y;
};

inline float si_absf(float x)
{
    return ::fabsf(x);
}

inline float si_sqrtf(float x)
{
    return ::sqrtf(x);
}

inline float si_sinf(float x)
{
    return ::sinf(x);
}

inline float si_cosf(float x)
{
    return ::cosf(x);
}

inline float si_atan2(float y, float x)
{
    return ::atan2f(y, x);
}

inline float si_powf(float x, float p)
{
    return ::powf(x, p);
}

inline float lerp(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}

inline float invlerp(float a, float b, float x)
{
    return (x - a) / (b - a);
}

inline bool iszero(const float2 &v)
{
    bool result;
    result = si_absf(v.x) < 0.0000001f && si_absf(v.y) < 0.0000001f;
    return result;
}

inline float2 operator-(const float2 &v)
{
    float2 result;
    result.x = -v.x;
    result.y = -v.y;
    return result;
}

inline float2 operator+(const float2 &a, const float2 &b)
{
    float2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline float2 operator-(const float2 &a, const float2 &b)
{
    float2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline float2 operator*(float k, const float2 &v)
{
    float2 result;
    result.x = k * v.x;
    result.y = k * v.y;
    return result;
}

inline float2 operator*(const float2 &v, float k)
{
    float2 result;
    result.x = k * v.x;
    result.y = k * v.y;
    return result;
}

inline float2 operator/(const float2 &v, float k)
{
    float2 result;
    result.x = v.x / k;
    result.y = v.y / k;
    return result;
}

inline float len(const float2 &v)
{
    float result = si_sqrtf(v.x * v.x + v.y * v.y);
    return result;
}

inline float dot(const float2 &a, const float2 &b)
{
    float result;
    result = a.x * b.x + a.y * b.y;
    return result;
}

inline float cross(const float2 &a, const float2 &b)
{
    float result;
    result = a.x * b.y - a.y * b.x;
    return result;
}

inline float2 perp(const float2 &v)
{
    float2 result;
    result.x = -v.y;
    result.y =  v.x;
    return result;
}

inline float2 norm(const float2 &v)
{
    return v * (1.0f / len(v));
}

inline float cos(const float2 &a, const float2 &b)
{
    float result;
    result = dot(a, b);
    result /= len(a);
    result /= len(b);
    return result;
}

inline float sin(const float2 &a, const float2 &b)
{
    float result;
    result = cross(a, b);
    result /= len(a);
    result /= len(b);
    return result;
}

inline float2 lerp(const float2& a, const float2& b, float t)
{
    return a * (1.0f - t) + b * t;
}

inline float si_minf(float a, float b)
{
    if (a < b) return a;
    return b;
}

inline float si_maxf(float a, float b)
{
    if (a > b) return a;
    return b;
}


inline double si_mind(double a, double b)
{
    if (a < b) return a;
    return b;
}

inline double si_maxd(double a, double b)
{
    if (a > b) return a;
    return b;
}


inline float si_clampf(float x, float x0, float x1)
{
    return si_minf(si_maxf(x, x0), x1);
}

inline double si_clampd(double x, double x0, double x1)
{
    return si_mind(si_maxd(x, x0), x1);
}

const float si_pi = 3.1415926535f;

struct box2
{
    float2 min, max;

    box2() :
        min{ FLT_MAX,  FLT_MAX },
        max{ -FLT_MAX, -FLT_MAX }
    {}

    box2(float l, float r, float b, float t) :
        min{ l, b },
        max{ r, t }
    {}

    void grow(float s)
    {
        min.x -= s;
        max.x += s;
        min.y -= s;
        max.x += s;
    }

    void add(const float2 &p)
    {
        if (p.x > max.x) max.x = p.x;
        if (p.x < min.x) min.x = p.x;
        if (p.y > max.y) max.y = p.y;
        if (p.y < min.y) min.y = p.y;
    }

    void add_box(const box2 &b)
    {
        if (!b.empty())
        {
            add(b.min);
            add(b.max);
        }
    }

    bool empty() const
    {
        return min.x > max.x || min.y > max.y;
    }

    float width() const
    {
        return max.x - min.x;
    }

    float height() const
    {
        return max.y - min.y;
    }

    float2 get_tl() const
    {
        return{ min.x, max.y };
    }

    float2 get_br() const
    {
        return{ max.x, min.y };
    }
};

// basis with perpendicular y and x axes
// NOTE: y is 90 degrees ccw x
struct basis2s
{
    float2 o;
    float2 x;
    float2 get_y() const { return{ -x.y, x.x }; }
};

struct basis2r
{
    float2 o;
    float2 x;
    float2 y;
};

struct color3f
{
    float r, g, b;
};

struct color4f
{
    float r, g, b, a;
};
