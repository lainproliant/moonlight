/*
 * geometry.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday August 18, 2024
 */

#ifndef __MOONLIGHT_GEOMETRY_H
#define __MOONLIGHT_GEOMETRY_H

#include <cfloat>
#include <cmath>
#include <numeric>
#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

#ifndef MOONLIGHT_GEO_PRECISION
#define MOONLIGHT_GEO_PRECISION 3
#endif

namespace moonlight {
namespace geo {

// ------------------------------------------------------------------
template<class T = float>
class Rect;

template<class T = float>
class Polygon;

// ------------------------------------------------------------------
enum class Pos2 {
    ON = 0,
    TOP = 1,
    RIGHT = 2,
    BOTTOM = 3,
    LEFT = 4,
    OFF = 5
};

enum class Winding {
    NONE = 0,
    CW = 1,
    CCW = -1
};

// ------------------------------------------------------------------
inline bool eps_eq(float a, float b, float eps = FLT_EPSILON) {
    return std::fabs(a - b) < eps;
}

inline bool eps_eq(double a, double b, double eps = DBL_EPSILON) {
    return std::abs(a - b) < eps;
}

// ------------------------------------------------------------------
template<class T>
inline bool equal(const T& a, const T& b) {
    return a == b;
}

template<>
inline bool equal(const float& a, const float& b) {
    return eps_eq(a, b);
}

template<>
inline bool equal(const double& a, const double& b) {
    return eps_eq(a, b);
}

// ------------------------------------------------------------------
template<class T = float>
class V2 {
 public:
     typedef T Contents;

     V2() : x(0), y(0) { }

     V2(const T& vx, const T& vy) : x(vx), y(vy) { }

     template<class R>
     V2<R> as() const {
         return V2<R>(
             static_cast<R>(x),
             static_cast<R>(y)
         );
     }

     template<>
     V2<int> as() const {
         return V2<int>(
             static_cast<int>(x < 0 ? x - 0.5 : x + 0.5),
             static_cast<int>(y < 0 ? y - 0.5 : y + 0.5)
         );
     }

     T angle_between(const V2<T>& other) {
         return std::acos(dot_product(other) / (
             magnitude() * other.magnitude()
         ));
     }

     T cross_product(const V2<T>& vB) {
         auto nA = normalize();
         auto nB = vB.normalize();

         return nA.x * nB.y - nA.y * nB.x;
     }

     T dot_product(const V2<T>& vB) const {
         return std::inner_product({x, y}, 2, {vB.x, vB.y}, 0);
     }

     bool intersects(const V2<T>& other) const {
         return intersects_at(other).has_value();
     }

     std::optional<V2<T>> intersects_at(const V2<T>& other) const {
        T d = (-other.x * y + x * other.y);

        if (! equal(d, 0.0)) {
            T s = (-y       * (x - other.x) + x       * (y - other.y)) / d;
            T t = ( other.x * (y - other.y) + other.y * (x - other.x)) / d;

            if (s >= 0 && s <= 1 && t>= 0 && t <= 1) {
                return V2<T>(
                    x + (t * x),
                    y + (t * y)
                );
            }
        }

        return {};
     }

     T magnitude() const {
         return sqrt(x*x + y*y);
     }

     V2<T> normalize() const {
         return (*this) * (1.0 / magnitude<T>());
     }

     std::string repr() const {
         std::ostringstream sb;
         sb.precision(MOONLIGHT_GEO_PRECISION);

         sb
         << "("
         << x << ", " << y
         << ")";

         return sb.str();
     }

     V2<int> round() const {
         return V2<int>(std::round(x), std::round(y));
     }

     bool operator==(const V2<T>& rhs) const {
         return equal(x, rhs.x) && equal(y, rhs.y);
     }

     bool operator!=(const V2<T>& rhs) const {
         return ! this->operator==(rhs);
     }

     V2<T>& operator+=(const V2<T>& rhs) {
         *this = *this + rhs;
         return *this;
     }

     V2<T>& operator-=(const V2<T>& rhs) {
         *this = *this - rhs;
         return *this;
     }

     V2<T>& operator*=(const T& s) {
         *this = *this * s;
         return *this;
     }

     V2<T>& operator/=(const T& s) {
        *this = *this / s;
        return *this;
     }

     V2<T> operator+(const V2<T>& rhs) const {
         return V2<T>(x + rhs.x, y + rhs.y);
     }

     V2<T> operator-() const {
         return V2<T>(-x, -y);
     }

     V2<T> operator-(const V2<T>& rhs) const {
         return V2<T>(x - rhs.x, y - rhs.y);
     }

     V2<T> operator*(const T& s) {
         return V2<T>(x * s, y * s);
     }

     V2<T> operator/(const T& s) {
         return V2<T>(x / s, y / s);
     }

     friend std::ostream& operator<<(std::ostream& out, const V2<T>& v) {
         out << "V2<" << type_name<T>() << ">" << v.repr();
         return out;
     }

     T x, y;
};

template<>
inline float V2<float>::magnitude() const {
    return fsqrt(x*x + y*y);
}

// ------------------------------------------------------------------
template<class T = float>
class L2 {
public:
    L2() : a(), b() { }
    L2(const V2<T>& pA, const V2<T>& pB) : a(pA), b(pB) { }
    L2(const T& xA, const T& yA, const T& xB, const T& yB)
    : a(xA, yA), b(xB, yB) { }

    template<class R>
    L2<R> as() const {
        return L2<R>(
            a.template as<R>(),
            b.template as<R>()
        );
    }

    V2<T> as_vec() const {
        return V2<T>(
            b.x - a.x, b.y - a.y
        );
    }

    bool intersects_vec(const L2<T>& other) const {
        return intersects_vec_at(other).has_value();
    }

    std::optional<V2<T>> intersects_vec_at(const V2<T>& vec) const {
        return as_vec().intersects_at(vec);
    }

    Pos2 orient(const V2<T>& point) const {
        auto vA = as_vec();
        auto vB = V2<T>(point.x - vA.x, point.y - vA.y);

        auto result = vA.cross_product(vB);

        if (equal(result, 0.0)) {
            return Pos2::ON;
        }

        return result < 0 ? Pos2::LEFT : Pos2::RIGHT;
    }

    bool intersects_line(const L2<T>& segment) const {
        return (
            orient(segment.a) == Pos2::ON ||
            orient(segment.b) == Pos2::ON ||
            (
                orient_point(segment.a) == Pos2::LEFT ^
                orient_point(segment.b) == Pos2::LEFT
            )
        );
    }

    std::optional<V2<T>> intersects_line_at(const L2<T>& segment) const {
        if (intersects_line(segment)) {
            return intersects_vec_at(segment.a);
        }

        return {};
    }

    T length() const {
        return as_vec().magnitude();
    }

    V2<T> midpoint() const {
        return a + (as_vec().magnitude() / 2);
    }

    std::vector<L2<T>> split(int segments = 2) {

    }

    std::string repr() const {
        std::ostringstream sb;
        sb.precision(MOONLIGHT_GEO_PRECISION);

        sb
        << "{"
        << a.repr() << ", " << b.repr()
        << "}";

        return sb.str();
    }

    bool operator==(const L2<T>& rhs) const {
        return a = rhs.a && b == rhs.b;
    }

    bool operator!=(const L2<T>& rhs) const {
        return ! this->operator==(rhs);
    }

    friend std::ostream& operator<<(std::ostream& out, const L2<T>& line) {
        out << "L2<" << type_name<T>() << line.repr();
        return out;
    }

    V2<T> a, b;
};

// ------------------------------------------------------------------
template<class T = int>
class S2 {
    S2() : w(0), h(0) { }
    S2(const T& width, const T& height) : w(width), h(height) { }

    template<class R>
    S2<R> as() const {
        return S2<R>(
            static_cast<R>(w),
            static_cast<R>(h)
        );
    }

    template<>
    S2<int> as() const {
        return S2<int>(
            static_cast<int>(w < 0 ? w - 0.5 : w + 0.5),
            static_cast<int>(h < 0 ? h - 0.5 : h + 0.5)
        );
    }

    Rect<T> as_rect() const {
        return Rect<T>(V2<T>(), *this);
    }

    std::string repr() const {
        std::ostringstream sb;
        sb.precision(MOONLIGHT_GEO_PRECISION);

        sb << w << "x" << h;

        return sb.str();
    }

    V2<T> vec_w() const {
        return V2<T>(w, 0);
    }

    V2<T> vec_h() const {
        return V2<T>(0, h);
    }

    V2<T> vec_wh() const {
        return V2<T>(w, h);
    }

    bool operator==(const S2<T>& rhs) const {
        return equal(w, rhs.w) && equal(h, rhs.h);
    }

    bool operator!=(const S2<T>& rhs) const {
        return ! this->operator==(rhs);
    }

    S2<T>& operator+=(const S2<T>& rhs) {
        *this = *this + rhs;
        return *this;
    }

    S2<T>& operator-=(const S2<T>& rhs) {
        *this = *this - rhs;
        return *this;
    }

    S2<T>& operator*=(const T& s) {
        *this = *this * s;
        return *this;
    }

    S2<T>& operator/=(const T& s) {
        *this = *this / s;
        return *this;
    }

    S2<T> operator+(const S2<T>& rhs) const {
        return S2<T>(w + rhs.w, h + rhs.h);
    }

    S2<T> operator-(const S2<T>& rhs) const {
        return S2<T>(w - rhs.w, h - rhs.h);
    }

    S2<T> operator*(const T& s) {
        return S2<T>(w * s, h * s);
    }

    S2<T> operator/(const T& s) {
        return S2<T>(w / s, h / s);
    }

    friend std::ostream& operator<<(std::ostream& out, const S2<T>& sz) {
        out
        << "S2<" << type_name<T>() << ">"
        << "(" << sz.repr() <<  ")";
        return out;
    }

    T w, h;
};

// ------------------------------------------------------------------
template<class T>
class P1 {
public:
    P1() : min(0), max(0) { }
    P1(const T& minimum, const T& maximum) : min(minimum), max(maximum) { }

    bool overlaps_proj(const P1<T>& other) {
        return max >= other.max && other.max >= min;
    }

    std::string repr() const {
        std::ostringstream sb;
        sb.precision(MOONLIGHT_GEO_PRECISION);

        sb
        << "("
        << min << " - " << max
        << ")";

        return sb.str();
    }

    friend std::ostream& operator<<(std::ostream& out, const P1<T>& proj) {
        out << "P1<" << type_name<T>() << ">" << proj.repr();
        return out;
    }

    T min, max;
};

// ------------------------------------------------------------------
template<class T>
class Rect {
 public:
     Rect() : pos(), sz() { }

     Rect(const S2<T>& size) : pos(), sz(size) { }
     Rect(const V2<T>& position, const S2<T>& size) : pos(position), sz(size) { }
     Rect(const T& width, const T& height) : pos(), sz(width, height) { }
     Rect(const T& x, const T& y, const T& w, const T& h)
     : pos(x, y), sz(w, h) { }

     struct Intersection {
         Pos2 pos;
         L2<T> edge;
         V2<T> pt;
     };

     template<class R>
     Rect<R> as() const {
         return Rect(
             pos.template as<R>(),
             sz.template as<R>()
         );
     }

     Polygon<T> as_polygon() const {
         return Polygon<T>(corners());
     }

     static Rect<T> bind_points(const std::vector<V2<T>>& pts) {
         T xmin = 0, ymin = 0, xmax = 0, ymax = 0;

         for (auto pt : pts) {
            if (pt.x < xmin) {
               xmin = pt.x;
            } else if (pt.x > xmax) {
               xmax = pt.x;
            }

            if (pt.y < ymin) {
               ymin = pt.y;
            } else if (pt.y > ymax) {
               ymax = pt.y;
            }
         }

         return Rect<T>(xmin, ymin, xmax - xmin, ymax - ymin);
     }

     static Rect<T> bind_rects(const std::vector<Rect<T>> rects) {
         std::vector<V2<T>> pts;

         for (auto rect : rects) {
             pts.push_back(rect.pt);
             pts.push_back(rect.pt + rect.sz.vec_wh());
         }

         return bind_points(pts);
     }

     bool contains_point(const V2<T>& pt) const {
         return (
             pt.x > pos.x &&
             pt.x <= pos.x + sz.w &&
             pt.y > pos.y &&
             pt.y <= pos.y + sz.height
         );
     }

     bool contains_rect(const Rect<T>& other) const {
         return (
             other.pos.x >= pos.x &&
             other.pos.y >= pos.y &&
             other.sz.width + (other.pos.x - pos.x) <= sz.width &&
             other.sz.height + (other.pos.y - pos.y) <= sz.height
         );
     }

     V2<T> center() const {
         return pos + V2<T>(sz.w / 2, sz.h / 2);
     }

     std::vector<V2<T>> corners() const {
         return {
             pos,
             pos + sz.vec_w(),
             pos + sz.vec_wh(),
             pos + sz.vec_h()
         };
     }

     std::vector<L2<T>> edges() const {
         const auto pts = corners();
         return {
             L2<T>(pts[0], pts[1]),
             L2<T>(pts[1], pts[2]),
             L2<T>(pts[2], pts[3]),
             L2<T>(pts[3], pts[0])
         };
     }

     bool intersects_line(const L2<T>& line) const {
         if (contains_point(line.a) || contains_point(line.b)) {
             return true;
         }

         return intersects_line_at(line).size() > 0;
     }

     std::vector<Intersection>
     intersects_line_at(const L2<T>& line) const {
         std::optional<V2<T>> pt;
         std::vector<std::tuple<Pos2, L2<T>, V2<T>>> collisions;
         const auto lines = edges();
         static const std::vector<Pos2> pos_order = {
             Pos2::TOP, Pos2::RIGHT, Pos2::BOTTOM, Pos2::LEFT
         };

         for (int x = 0; x < 4; x++) {
             pt = lines[x].intersects_line_at(line);
             if (pt.has_value()) {
                 collisions.push_back({
                     pos_order[x],
                     lines[x],
                     pt.value()
                 });
             }

         }

         return collisions;
     }

     bool intersects_rect(const Rect<T>& other) const {
         return !(
             (pos.x > other.pos.x + other.sz.width) ||
             (pos.y > other.pos.y + other.sz.height) ||
             (other.pos.x > pos.x + sz.width) ||
             (other.pos.y > pos.y + sz.height)
         );
     }

     Rect<T> move(const V2<T>& position) const {
         return Rect<T>(position, sz);
     }

     std::vector<Rect<T>> quadsect() const {
         auto qsz = sz / 2;
         auto quadrect = Rect<T>(pos, qsz);
         return {
             quadrect,
             quadrect + qsz.vec_w(),
             quadrect + qsz.vec_wh(),
             quadrect + qsz.vec_h()
         };
     }

     std::string repr() const {
         std::ostringstream sb;

         sb << "{"
         << sz.repr() << ", " << pos.repr()
         << "}";

         return sb.str();
     }

     V2<T> tile(const S2<T>& tile_sz, int tile_id) {
         auto row_len = sz.w / tile_sz.w;
         return pos + V2<T>(
             tile_sz.w * (tile_id % row_len),
             tile_sz.h * (tile_id / row_len)
         );
     }

     Rect<T> translate(const V2<T>& vec) const {
         return move(pos + vec);
     }

     Rect<T> operator+(const V2<T>& vec) const {
         return translate(vec);
     }

     Rect<T> operator-(const V2<T>& vec) const {
         return translate(-vec);
     }

     Rect<T>& operator+=(const V2<T>& vec) {
         *this = *this + vec;
         return *this;
     }

     Rect<T>& operator-=(const V2<T>& vec) {
         *this = *this - vec;
         return *this;
     }

     friend std::ostream& operator<<(std::ostream& out, const Rect<T>& rect) {
         out << "Rect<" << type_name<T>() << ">" << rect.repr();
         return out;
     }

     V2<T> pos;
     S2<T> sz;
};

// ------------------------------------------------------------------
template<class T>
class Polygon {
public:
    Polygon() : pts() { }
    Polygon(const std::vector<V2<T>>& points) : pts(points) { }

    struct Intersection {
        int edge_id;
        L2<T> edge;
        V2<T> pt;
    };

    struct Collision {
        bool vertex;
        bool edge;
    };

    bool contains_point(const V2<T>& pt) const {
        bool inside = false;
        const auto x = pt.x, y = pt.y;

        for (int i = 0, j = pts.size(); i < pts.size(); j = i++) {
            const auto xi = pts[i].x, yi = pts[i].y;
            const auto xj = pts[j].x, yj = pts[j].y;

            const bool intersect = (
                ((yi > y) != (yj > j)) &&
                (x < (xj - xi) * (y - yi) / (yj - yi) + xi)
            );
            if (intersect) inside = !inside;
        }

        return inside;
    }

    const std::vector<L2<T>> edges() const {
        std::vector<L2<T>> edges;

        for (int x = 0; x < pts.size(); x++) {
            if (x < pts.size() - 1) {
                edges.push_back(L2<T>(pts[x], pts[x + 1]));
            } else {
                edges.push_back(L2<T>(pts[x], pts[0]));
            }
        }

        return edges;
    }

    bool intersects_line(const L2<T>& line) const {
        if (contains_point(line.a) || contains_point(line.b)) {
            return true;
        }

        return intersects_line_at(line).size() > 0;
    }

    std::vector<Intersection>
    intersects_line_at(const L2<T>& line) const {
        std::optional<V2<T>> pt;
        std::vector<std::tuple<int, L2<T>, V2<T>>> collisions;
        const auto lines = edges();

        for (int x = 0; x < lines.size(); x++) {
            pt = lines[x].intersects_line_at(line);
            if (pt.has_value()) {
                collisions.push_back({
                    x,
                    lines[x],
                    pt.value()
                });
            }
        }

        return collisions;
    }

    Collision collides_with_polygon(const Polygon<T>& other) const {
        bool contains_vertex = false;

        for (const auto& pt : other.pts) {
            if (contains_point(pt)) {
                contains_vertex = true;
                break;
            }
        }

        return {contains_vertex, intersects_polygon_sep_axis(other)};
    }

    std::vector<Intersection> intersects_polygon_at(const Polygon<T>& other) {
        std::vector<Intersection> results;

        for (auto edge : other.edges()) {
            for (auto intersection : intersects_line_at(edge)) {
                results.push_back(intersection);
            }
        }

        return results;
    }

    bool intersects_polygon_sep_axis(const Polygon<T>& other) {
        for (int i = 0, j = pts.size(); i < pts.size(); j = i++) {
            V2<T> axis = {
                -(pts[j].y - pts[i].y),
                pts[j].x - pts[i].x
            };

            auto p1 = project_onto_axis(axis);
            auto p2 = other.project_onto_axis(axis);

            if (! p1.overlaps_proj(p2)) {
                return false;
            }
        }

        for (int i = 0, j = other.pts.size(); i < other.pts.size(); j = i++) {
            V2<T> axis = {
                -(other.pts[j].y - other.pts[i].y),
                other.pts[j].x - other.pts[i].x
            };

            auto p1 = project_onto_axis(axis);
            auto p2 = other.project_onto_axis(axis);

            if (! p1.overlaps_proj(p2)) {
                return false;
            }
        }

        return true;
    }

    P1<T> project_onto_axis(const V2<T>& axis) {
        T min = pts[0].dot_product(axis);
        T max = min;

        for (int x = 1; x < pts.size(); x++) {
            T proj = pts[x].dot_product(axis);
            if (proj < min) {
                min = proj;

            } else if (proj > max) {
                max = proj;
            }
        }

        return {min, max};
    }

    const std::vector<V2<T>> normals() const {
        std::vector<V2<T>> normals;

        for (auto edge : edges()) {
            auto vec = edge.as_vec();
            normals.push_back(V2<T>(vec.y, -vec.x).normalize());
        }

        return normals;
    }

    std::string repr() const {
        std::ostringstream sb;

        sb << "{" << pts.size() << ", ";

        for (int x = 0; x < pts.size(); x++) {
            sb << pts[x].repr();
            if (x < pts.size() - 1) {
                sb << ", ";
            }
        }

        sb << "}";

        return sb.str();
    }

    Winding winding() const {
        for (int x = 0; x < pts.size(); x++) {
            int y = x + 1 == pts.size() ? 0 : x + 1;

            auto cross_product = pts[x].cross_product(pts[y]);

            if (! equal(cross_product, 0.00)) {
                if (cross_product > 0) {
                    return Winding::CW;

                } else if (cross_product < 0) {
                    return Winding::CCW;
                }
            }
        }

        return Winding::NONE;
    }

    friend std::ostream& operator<<(std::ostream& out, const Polygon<T>& polygon) {
        out << "Polygon<" << type_name<T>() << ">" << polygon.repr();
        return out;
    }

    std::vector<V2<T>> pts;
};

}
}


#endif /* !__MOONLIGHT_GEOMETRY_H */
