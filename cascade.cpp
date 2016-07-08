struct test_point
{
    float x;
    float y;
    float w;
    test_point() : w(1.0f) {}
    test_point(float a, float b) : x(a), y(b), w(1.0f) {}
    test_point(const float2& p, float v) : x(p.x), y(p.y), w(v) {}
};

struct test_segment
{
    test_point a;
    test_point b;
};

struct test_box
{
    float x0, x1;
    float y0, y1;

    test_box() :
        x0(FLT_MAX), x1(-FLT_MAX),
        y0(FLT_MAX), y1(-FLT_MAX)
    {}

    test_box(float l, float r, float b, float t) :
        x0(l), x1(r),
        y0(b), y1(t)
    {}

    void grow(float s)
    {
        x0 -= s;
        x1 += s;
        y0 -= s;
        y1 += s;
    }

    void add(const test_point &p)
    {
        if (p.x > x1) x1 = p.x;
        if (p.x < x0) x0 = p.x;
        if (p.y > y1) y1 = p.y;
        if (p.y < y0) y0 = p.y;
    }

    void add_box(const test_box &b)
    {
        if (!b.empty())
        {
            add(test_point(b.x0, b.y0));
            add(test_point(b.x1, b.y1));
        }
    }

    bool empty() const
    {
        return x0 > x1 || y0 > y1;
    }

    float width() const
    {
        return x1 - x0;
    }

    float height() const
    {
        return y1 - y0;
    }
};

struct test_stroke
{
    size_t pi0;
    size_t pi1;
    size_t brush_id;
    test_box bbox;
};

enum transition_type { TZOOM, TPAN, TROTATE };

struct test_transition
{
    transition_type type;
    float a;
    float b;
};

// TODO: refine this
struct test_image
{
    size_t nameidx;
    float tx;
    float ty;
    float xx;
    float xy;
    float yx;
    float yy;

    test_box get_bbox() const
    {
        test_point p00{ tx,           ty };
        test_point p10{ tx + xx,      ty + xy };
        test_point p01{ tx +      yx, ty +      yy };
        test_point p11{ tx + xx + yx, ty + xy + yy };

        test_box bbox;
        bbox.add(p00);
        bbox.add(p10);
        bbox.add(p01);
        bbox.add(p11);

        return bbox;
    }
};

const size_t NPOS = static_cast<size_t>(-1);

struct test_view
{
    test_transition tr;
    size_t si0;
    size_t si1;
    size_t img;
    size_t pin_index;
    test_box bbox;
    test_box imgbbox;
    test_view(const test_transition& t, size_t s0, size_t s1, size_t i = NPOS)
        : tr(t), si0(s0), si1(s1), img(i), pin_index(NPOS)
    {}
};

struct test_data
{
    const test_point *points;
    const test_stroke *strokes;
    const test_view *views;
    size_t nviews;
};

struct test_visible
{
    enum vis_type {IMAGE, STROKE};
    vis_type ty;
    size_t obj_id;
    size_t tform_id;
};

bool liang_barsky(float L, float R, float B, float T,
                  float x0, float y0, float x1, float y1)
{
    float p[] = {x0 - x1, x1 - x0, y0 - y1, y1 - y0};
    float q[] = {x0 - L,   R - x0, y0 - B,   T - y0};

    float t0 = 0.0f;
    float t1 = 1.0f;

    for (size_t i = 0; i < 4; ++i)
    {
        if (p[i] == 0.0f)
        {
            if (q[i] < 0.0f) return false;
        }
        else
        {
            float t = q[i] / p[i];

            if (p[i] < 0.0f)
            {
                if (t > t0) t0 = t;
            }
            else
            {
                if (t < t1) t1 = t;
            }

            if (t0 > t1) return false;
        }
    }
    return true;
}

bool intersects(const test_box &viewport,
                const test_point &p0,
                const test_point &p1)
{
    return liang_barsky(viewport.x0, viewport.x1, viewport.y0, viewport.y1,
                        p0.x, p0.y, p1.x, p1.y);
}

bool overlaps(const test_box &b1, const test_box &b2)
{
    if (b1.empty() || b2.empty()) return false;
    if (b1.y0 > b2.y1) return false;
    if (b2.y0 > b1.y1) return false;
    if (b1.x0 > b2.x1) return false;
    if (b2.x0 > b1.x1) return false;
    return true;
}

void crop(const test_data &data, size_t vi, size_t ti,
          const test_box &viewport,
          std::vector<test_visible> &visibles,
          float negligibledist)
{
    test_view view = data.views[vi];

    if (view.img != NPOS)
    {
        // TODO: why do we check with negligibledist here?
        if (overlaps(viewport, view.imgbbox) &&
            view.imgbbox.width() > negligibledist &&
            view.imgbbox.height() > negligibledist)
        {
            test_visible vis;
            vis.ty = test_visible::IMAGE;
            vis.obj_id = view.img;
            vis.tform_id = ti;
            visibles.push_back(vis);
        }
    }

    // TODO: why do we check with negligibledist here?
    if (overlaps(viewport, view.bbox) &&
        view.bbox.width() > negligibledist &&
        view.bbox.height() > negligibledist)
    {

        for (size_t si = view.si0; si < view.si1; ++si)
        {
            test_stroke stroke = data.strokes[si];
            // TODO: why do we check with negligibledist here?
            if (overlaps(viewport, stroke.bbox) &&
                stroke.bbox.width() > negligibledist &&
                stroke.bbox.height() > negligibledist)
            {
                test_visible vis;
                vis.ty = test_visible::STROKE;
                vis.obj_id = si;
                vis.tform_id = ti;
                visibles.push_back(vis);
            }
        }
    }
}

struct test_transform
{
    float s;
    float tx;
    float ty;
    float a;
};

// coordinates of center (x0, y0) and x axis (xx, xy)
// y axis is (-xy, xx)
struct test_basis
{
    float x0;
    float y0;
    float xx;
    float xy;
};

test_transform transform_from_basis(const test_basis &basis)
{
    test_transform result;
    result.tx = basis.x0;
    result.ty = basis.y0;
    result.s = si_sqrtf(basis.xx * basis.xx + basis.xy * basis.xy);
    result.a = si_atan2(basis.xy, basis.xx);
    return result;
}

test_basis basis_from_transform(const test_transform &transform)
{
    test_basis result;
    result.x0 = transform.tx;
    result.y0 = transform.ty;
    result.xx = transform.s * si_cosf(transform.a);
    result.xy = transform.s * si_sinf(transform.a);
    return result;
}

test_basis default_basis()
{
    return {0.0f, 0.0f, 1.0f, 0.0f};
}

test_transform id_transform()
{
    return {1.0f, 0.0f, 0.0f, 0.0f};
}

test_transform transform_from_transition(const test_transition &transition)
{
    test_transform result = id_transform();

    if (transition.type == TZOOM)
    {
        result.s = transition.b / transition.a;
    }
    else if (transition.type == TPAN)
    {
        result.tx = transition.a;
        result.ty = transition.b;
    }
    else if (transition.type == TROTATE)
    {
        result.a = transition.a;
    }

    return result;
}

test_transition inverse_transition(const test_transition &transition)
{
    test_transition result = transition;

    if (transition.type == TZOOM)
    {
        result.a = transition.b;
        result.b = transition.a;
    }
    else if (transition.type == TPAN)
    {
        result.a = -transition.a;
        result.b = -transition.b;
    }
    else if (transition.type == TROTATE)
    {
        result.a = -transition.a;
    }

    return result;
}

test_point point_in_basis(const test_basis &b,
                          const test_point &p)
{
    float Xx =  b.xx;
    float Xy =  b.xy;
    float Yx = -b.xy;
    float Yy =  b.xx;

    test_point result;
    result.x = p.x * Xx + p.y * Yx + b.x0;
    result.y = p.x * Xy + p.y * Yy + b.y0;
    return result;
}

test_point apply_transform_pt(const test_transform &t,
                              const test_point &p)
{
    test_basis basis = basis_from_transform(t);
    return point_in_basis(basis, p);
}

test_basis basis_in_basis(const test_basis &b0,
                          const test_basis &b1)
{
    test_point b1o = {b1.x0, b1.y0};
    test_point b1x = {b1.x0 + b1.xx, b1.y0 + b1.xy};

    test_point b1o_ = point_in_basis(b0, b1o);
    test_point b1x_ = point_in_basis(b0, b1x);
    
    test_basis result;
    result.x0 = b1o_.x;
    result.y0 = b1o_.y;
    result.xx = b1x_.x - b1o_.x;
    result.xy = b1x_.y - b1o_.y;
    return result;
}

test_box apply_transform_box(const test_transform &t,
                             const test_box &b)
{
    if (b.empty())
        return b;

    test_box result;
    result.add(apply_transform_pt(t, {b.x0, b.y0})); // BL
    result.add(apply_transform_pt(t, {b.x0, b.y1})); // TL
    result.add(apply_transform_pt(t, {b.x1, b.y1})); // TR
    result.add(apply_transform_pt(t, {b.x1, b.y0})); // BR
    return result;
}

float apply_transform_dist(const test_transform &t, float d)
{
    return d * t.s;
}

test_segment apply_transform_seg(const test_transform &t,
                                 const test_segment &s)
{
    return {apply_transform_pt(t, s.a), apply_transform_pt(t, s.b)};
}

test_transform combine_transforms(const test_transform &t0,
                                  const test_transform &t1)
{
    test_basis b0 = basis_from_transform(t0);
    test_basis b1 = basis_from_transform(t1);

    test_basis combined = basis_in_basis(b0, b1);
    test_transform result = transform_from_basis(combined);

    return result;
}

// get transform which, when applied to points in src_idx view space
// will produce their coordinates in dst_idx view space
test_transform get_relative_transform(const test_data &data,
                                      size_t src_idx, size_t dst_idx)
{
    test_transform accum = id_transform();
    if (dst_idx > src_idx)
    {
        for (size_t vi = dst_idx; vi != src_idx; --vi)
        {
            const auto &view = data.views[vi];

            test_transition inv_tr = inverse_transition(view.tr);
            test_transform transform = transform_from_transition(inv_tr);
            accum = combine_transforms(accum, transform);
        }
    }
    else
    {
        for (size_t vi = dst_idx + 1; vi <= src_idx; ++vi)
        {
            const auto &view = data.views[vi];

            test_transform transform = transform_from_transition(view.tr);
            accum = combine_transforms(accum, transform);
        }
    }
    return accum;
}

// given a pin (a view index) collect list of strokes
// along with needed transforms, that are inside viewport
// which is in pin's view local space
void query(const test_data &data, size_t pin, const test_box &viewport,
           std::vector<test_visible> &visibles,
           std::vector<test_transform> &transforms,
           float negligibledist)
{
    // ps_*** -- pin space
    // ls_*** -- current view's space

    for (size_t vi = 0; vi < data.nviews; ++vi)
    {
        test_transform ls2ps = get_relative_transform(data, vi, pin);
        test_transform ps2ls = get_relative_transform(data, pin, vi);

        test_box ls_box = apply_transform_box(ps2ls, viewport);
        float ls_negligible = apply_transform_dist(ps2ls, negligibledist);
        crop(data, vi, transforms.size(), ls_box, visibles, ls_negligible);
        transforms.push_back(ls2ps);
    }
}

test_box query_bbox(const test_data &data, size_t pin)
{
    test_box ps_accum_box;

    for (size_t vi = 0; vi < data.nviews; ++vi)
    {
        test_transform ls2ps = get_relative_transform(data, vi, pin);

        test_box ps_view_box = apply_transform_box(ls2ps, data.views[vi].bbox);
        ps_accum_box.add_box(ps_view_box);
    }

    return ps_accum_box;
}

float dist_to_seg(const test_point& p,
                  const test_point& p1, const test_point& p2)
{
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    return ::fabsf(dy * p.x - dx * p.y + p2.x * p1.y - p2.y * p1.x)
    / ::sqrtf(dy * dy + dx * dx);
}

void ramer_douglas_peucker(const test_point* from, const test_point* to,
                           std::vector<test_point>& result, float epsilon)
{
    if (from == to)
    {
        result.push_back(*from);
        return;
    }

    if (to - from == 1)
    {
        result.push_back(*from);
        result.push_back(*to);
        return;
    }

    float max_dist = 0.0f;
    const test_point* max_pt = 0;
    for (const test_point* p = from + 1; p != to; ++p)
    {
        // TODO: this may be actually not exactly what we need here
        float d = dist_to_seg(*p, *from, *to);

        if (d > max_dist)
        {
            max_pt = p;
            max_dist = d;
        }
    }

    if (max_dist > epsilon)
    {
        ramer_douglas_peucker(from, max_pt, result, epsilon);
        result.pop_back();
        ramer_douglas_peucker(max_pt, to, result, epsilon);
    }
    else
    {
        result.push_back(*from);
        result.push_back(*to);
    }
}

#include "fitbezier.cpp"

void sample_bezier(const std::vector<bezier4>& pieces,
                   std::vector<test_point>& pts)
{
    for (size_t i = 0; i < pieces.size(); ++i)
    {
        for (float t = 0.0f; t < 0.99f; t += 0.1f)
        {
            pts.push_back(test_point(pieces[i].eval(t), 1.0f));
        }
    }
    if (!pieces.empty())
    {
        pts.push_back(test_point(pieces.back().eval(1.0f), 1.0f));
    }
}

// N > 2
void smooth_stroke_2(const test_point* input, size_t N,
                     std::vector<test_point>& output,
                     float error)
{
    std::vector<bezier4> pieces;
    float2 t0 = {input[1].x - input[0].x,
                 input[1].y - input[0].y};
    t0 = t0 / len(t0);
    float2 t1 = {input[N-2].x - input[N-1].x,
                 input[N-2].y - input[N-1].y};
    t1 = t1 / len(t1);
    fit_bezier(input, 0, N, t0, t1, pieces, error);
    sample_bezier(pieces, output);
}

float2 hermite(const float2& p0, const float2& m0,
               const float2& p1, const float2& m1,
               float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    float h10 = t3 - 2.0f * t2 + t;
    float h01 = -2.0f * t3 + 3.0f * t2;
    float h11 = t3 - t2;
    return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
}

float2 tangent2(const float2& p0, float t0,
                const float2& p1, float t1,
                const float2& p2, float t2)
{
    return 0.5f * ((p2 - p1) / (t2 - t1) + (p1 - p0) / (t1 - t0));
}

float2 tangent(const float2& p0, float t0,
               const float2& p1, float t1)
{
    return (p1 - p0) / (t1 - t0);
}

// N > 2
void smooth_stroke_1(const test_point* input, size_t N,
                     std::vector<test_point>& output)
{
    {
        float2 p0 = {input[0].x, input[0].y};
        float2 p1 = {input[1].x, input[1].y};
        float2 p2 = {input[2].x, input[2].y};
        float t0 = 0.0f;//input[0].t;
        float t1 = 1.0f;//input[1].t;
        float t2 = 2.0f;//input[2].t;
        float2 m0 = tangent(p0, t0, p1, t1);
        float2 m1 = tangent2(p0, t0, p1, t1, p2, t2);

        float w0 = input[0].w;
        float w1 = input[1].w;

        output.push_back(input[0]);
        output.push_back(test_point(hermite(p0, m0, p1, m1, 0.25f),
                                    lerp(w0, w1, 0.25f)));
        output.push_back(test_point(hermite(p0, m0, p1, m1, 0.50f),
                                    lerp(w0, w1, 0.50f)));
        output.push_back(test_point(hermite(p0, m0, p1, m1, 0.75f),
                                    lerp(w0, w1, 0.75f)));
    }

	// TODO: better tangents
    for (size_t i = 1; i <= N-3; ++i)
    {
        float2 p0 = {input[i-1].x, input[i-1].y};
        float2 p1 = {input[i+0].x, input[i+0].y};
        float2 p2 = {input[i+1].x, input[i+1].y};
        float2 p3 = {input[i+2].x, input[i+2].y};
        float t0 = i-1.0f;//input[i-1].w;
        float t1 = i+0.0f;//input[i+0].w;
        float t2 = i+1.0f;//input[i+1].w;
        float t3 = i+2.0f;//input[i+2].w;
        float2 m1 = tangent2(p0, t0, p1, t1, p2, t2);
        float2 m2 = tangent2(p1, t1, p2, t2, p3, t3);

        float w1 = input[i+0].w;
        float w2 = input[i+1].w;

        output.push_back(input[i]);
        output.push_back(test_point(hermite(p1, m1, p2, m2, 0.25f),
                                    lerp(w1, w2, 0.25f)));
        output.push_back(test_point(hermite(p1, m1, p2, m2, 0.50f),
                                    lerp(w1, w2, 0.50f)));
        output.push_back(test_point(hermite(p1, m1, p2, m2, 0.75f),
                                    lerp(w1, w2, 0.75f)));
    }

    {
        float2 p0 = {input[N-3].x, input[N-3].y};
        float2 p1 = {input[N-2].x, input[N-2].y};
        float2 p2 = {input[N-1].x, input[N-1].y};
        float t0 = N-3.0f;//input[N-3].t;
        float t1 = N-2.0f;//input[N-2].t;
        float t2 = N-1.0f;//input[N-1].t;
        float2 m1 = tangent2(p0, t0, p1, t1, p2, t2);
        float2 m2 = tangent(p1, t1, p2, t2);

        float w1 = input[N-2].w;
        float w2 = input[N-1].w;

        output.push_back(input[N-2]);
        output.push_back(test_point(hermite(p1, m1, p2, m2, 0.25f),
                                    lerp(w1, w2, 0.25f)));
        output.push_back(test_point(hermite(p1, m1, p2, m2, 0.50f),
                                    lerp(w1, w2, 0.50f)));
        output.push_back(test_point(hermite(p1, m1, p2, m2, 0.75f),
                                    lerp(w1, w2, 0.75f)));
        output.push_back(input[N-1]);
    }
}

void smooth_stroke(const test_point* input, size_t N,
                   std::vector<test_point>& output,
                   float error, bool ng)
{
    if (N < 3)
    {
        for (size_t i = 0; i < N; ++i)
        {
            output.push_back(input[i]);
        }
        return;
    }

    if (ng)
    {
        smooth_stroke_2(input, N, output, error);
    }
    else
    {
        smooth_stroke_1(input, N, output);
    }
}

#include "tests.cpp"
