/*
 * ## geometry.h: Classes for geometric calculations. ----------------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday August 18, 2024
 *
 * ## WARNING --------------------------------------------------------
 * This library is experimental and not well tested.  More documentation and
 * testing to come.
 */

#ifndef __MOONLIGHT_GEOMETRY_H
#define __MOONLIGHT_GEOMETRY_H

#include <cfloat>
#include <unordered_map>
#include <set>
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
class Vector2d {
 public:
     typedef T Contents;

     Vector2d() : x(0), y(0) { }

     Vector2d(const T& vx, const T& vy) : x(vx), y(vy) { }

     template<class R>
     Vector2d<R> as() const {
         return Vector2d<R>(
             static_cast<R>(x),
             static_cast<R>(y)
         );
     }

     template<>
     Vector2d<int> as() const {
         return Vector2d<int>(
             static_cast<int>(x < 0 ? x - 0.5 : x + 0.5),
             static_cast<int>(y < 0 ? y - 0.5 : y + 0.5)
         );
     }

     T angle_between(const Vector2d<T>& other) {
         return std::acos(dot_product(other) / (
             magnitude() * other.magnitude()
         ));
     }

     T cross_product(const Vector2d<T>& vB) {
         auto nA = normalize();
         auto nB = vB.normalize();

         return nA.x * nB.y - nA.y * nB.x;
     }

     T dot_product(const Vector2d<T>& vB) const {
         return std::inner_product({x, y}, 2, {vB.x, vB.y}, 0);
     }

     bool intersects(const Vector2d<T>& other) const {
         return intersects_at(other).has_value();
     }

     std::optional<Vector2d<T>> intersects_at(const Vector2d<T>& other) const {
        T d = (-other.x * y + x * other.y);

        if (! equal(d, 0.0)) {
            T s = (-y       * (x - other.x) + x       * (y - other.y)) / d;
            T t = ( other.x * (y - other.y) + other.y * (x - other.x)) / d;

            if (s >= 0 && s <= 1 && t>= 0 && t <= 1) {
                return Vector2d<T>(
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

     Vector2d<T> normalize() const {
         return (*this) * (1.0f / magnitude());
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

     Vector2d<int> round() const {
         return Vector2d<int>(std::round(x), std::round(y));
     }

     bool operator==(const Vector2d<T>& rhs) const {
         return equal(x, rhs.x) && equal(y, rhs.y);
     }

     bool operator!=(const Vector2d<T>& rhs) const {
         return ! this->operator==(rhs);
     }

     Vector2d<T>& operator+=(const Vector2d<T>& rhs) {
         *this = *this + rhs;
         return *this;
     }

     Vector2d<T>& operator-=(const Vector2d<T>& rhs) {
         *this = *this - rhs;
         return *this;
     }

     Vector2d<T>& operator*=(const T& s) {
         *this = *this * s;
         return *this;
     }

     Vector2d<T>& operator/=(const T& s) {
        *this = *this / s;
        return *this;
     }

     Vector2d<T> operator+(const Vector2d<T>& rhs) const {
         return Vector2d<T>(x + rhs.x, y + rhs.y);
     }

     Vector2d<T> operator-() const {
         return Vector2d<T>(-x, -y);
     }

     Vector2d<T> operator-(const Vector2d<T>& rhs) const {
         return Vector2d<T>(x - rhs.x, y - rhs.y);
     }

     Vector2d<T> operator*(const T& s) const {
         return Vector2d<T>(x * s, y * s);
     }

     Vector2d<T> operator/(const T& s) const {
         return Vector2d<T>(x / s, y / s);
     }

     friend std::ostream& operator<<(std::ostream& out, const Vector2d<T>& v) {
         out << "Vector2d<" << type_name<T>() << ">" << v.repr();
         return out;
     }

     T x, y;
};

template<>
inline float Vector2d<float>::magnitude() const {
    return fsqrt(x*x + y*y);
}

// ------------------------------------------------------------------
template<class T = float>
class Line2d {
public:
    Line2d() : a(), b() { }
    Line2d(const Vector2d<T>& pA, const Vector2d<T>& pB) : a(pA), b(pB) { }
    Line2d(const T& xA, const T& yA, const T& xB, const T& yB)
    : a(xA, yA), b(xB, yB) { }

    template<class R>
    Line2d<R> as() const {
        return Line2d<R>(
            a.template as<R>(),
            b.template as<R>()
        );
    }

    Vector2d<T> as_vec() const {
        return Vector2d<T>(
            b.x - a.x, b.y - a.y
        );
    }

    bool intersects_vec(const Line2d<T>& other) const {
        return intersects_vec_at(other).has_value();
    }

    std::optional<Vector2d<T>> intersects_vec_at(const Vector2d<T>& vec) const {
        return as_vec().intersects_at(vec);
    }

    Pos2 orient(const Vector2d<T>& point) const {
        auto vA = as_vec();
        auto vB = Vector2d<T>(point.x - vA.x, point.y - vA.y);

        auto result = vA.cross_product(vB);

        if (equal(result, 0.0)) {
            return Pos2::ON;
        }

        return result < 0 ? Pos2::LEFT : Pos2::RIGHT;
    }

    bool intersects_line(const Line2d<T>& segment) const {
        return (
            orient(segment.a) == Pos2::ON ||
            orient(segment.b) == Pos2::ON ||
            (
                orient_point(segment.a) == Pos2::LEFT ^
                orient_point(segment.b) == Pos2::LEFT
            )
        );
    }

    std::optional<Vector2d<T>> intersects_line_at(const Line2d<T>& segment) const {
        if (intersects_line(segment)) {
            return intersects_vec_at(segment.a);
        }

        return {};
    }

    T length() const {
        return as_vec().magnitude();
    }

    Vector2d<T> midpoint() const {
        return a + as_vec() / 2;
    }

    std::vector<Line2d<T>> split(int segments = 2) {
        std::vector<Vector2d<T>> points;
        auto vec = as_vec() / segments;

        T segment_len = length() / segments;

        for (int x = 0; x < segments; x++) {
            points.push_back(a + vec * x);
        }

        std::vector<Line2d<T>> lines;

        for (int x = 1; x < segments; x++) {
            lines.push_back(Line2d<T>(points[x-1], points[x]));
        }

        return lines;
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

    bool operator==(const Line2d<T>& rhs) const {
        return a = rhs.a && b == rhs.b;
    }

    bool operator!=(const Line2d<T>& rhs) const {
        return ! this->operator==(rhs);
    }

    friend std::ostream& operator<<(std::ostream& out, const Line2d<T>& line) {
        out << "Line2d<" << type_name<T>() << line.repr();
        return out;
    }

    Vector2d<T> a, b;
};

// ------------------------------------------------------------------
template<class T = int>
class Size2d {
public:
    Size2d() : w(0), h(0) { }
    Size2d(const T& width, const T& height) : w(width), h(height) { }

    template<class R>
    Size2d<R> as() const {
        return Size2d<R>(
            static_cast<R>(w),
            static_cast<R>(h)
        );
    }

    template<>
    Size2d<int> as() const {
        return Size2d<int>(
            static_cast<int>(w < 0 ? w - 0.5 : w + 0.5),
            static_cast<int>(h < 0 ? h - 0.5 : h + 0.5)
        );
    }

    Rect<T> as_rect() const {
        return Rect<T>(Vector2d<T>(), *this);
    }

    std::string repr() const {
        std::ostringstream sb;
        sb.precision(MOONLIGHT_GEO_PRECISION);

        sb << w << "x" << h;

        return sb.str();
    }

    Vector2d<T> vec_w() const {
        return Vector2d<T>(w, 0);
    }

    Vector2d<T> vec_h() const {
        return Vector2d<T>(0, h);
    }

    Vector2d<T> vec_wh() const {
        return Vector2d<T>(w, h);
    }

    bool operator==(const Size2d<T>& rhs) const {
        return equal(w, rhs.w) && equal(h, rhs.h);
    }

    bool operator!=(const Size2d<T>& rhs) const {
        return ! this->operator==(rhs);
    }

    Size2d<T>& operator+=(const Size2d<T>& rhs) {
        *this = *this + rhs;
        return *this;
    }

    Size2d<T>& operator-=(const Size2d<T>& rhs) {
        *this = *this - rhs;
        return *this;
    }

    Size2d<T>& operator*=(const T& s) {
        *this = *this * s;
        return *this;
    }

    Size2d<T>& operator/=(const T& s) {
        *this = *this / s;
        return *this;
    }

    Size2d<T> operator+(const Size2d<T>& rhs) const {
        return Size2d<T>(w + rhs.w, h + rhs.h);
    }

    Size2d<T> operator-(const Size2d<T>& rhs) const {
        return Size2d<T>(w - rhs.w, h - rhs.h);
    }

    Size2d<T> operator*(const T& s) {
        return Size2d<T>(w * s, h * s);
    }

    Size2d<T> operator/(const T& s) {
        return Size2d<T>(w / s, h / s);
    }

    friend std::ostream& operator<<(std::ostream& out, const Size2d<T>& sz) {
        out
        << "Size2d<" << type_name<T>() << ">"
        << "(" << sz.repr() <<  ")";
        return out;
    }

    T w, h;
};

// ------------------------------------------------------------------
template<class T>
class Line1d {
public:
    Line1d() : min(0), max(0) { }
    Line1d(const T& minimum, const T& maximum) : min(minimum), max(maximum) { }

    bool overlaps(const Line1d<T>& other) {
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

    friend std::ostream& operator<<(std::ostream& out, const Line1d<T>& proj) {
        out << "Line1d<" << type_name<T>() << ">" << proj.repr();
        return out;
    }

    T min, max;
};

// ------------------------------------------------------------------
template<class T>
class Rect {
 public:
     Rect() : pos(), sz() { }

     Rect(const Size2d<T>& size) : pos(), sz(size) { }
     Rect(const Vector2d<T>& position, const Size2d<T>& size) : pos(position), sz(size) { }
     Rect(const T& width, const T& height) : pos(), sz(width, height) { }
     Rect(const T& x, const T& y, const T& w, const T& h)
     : pos(x, y), sz(w, h) { }

     struct Intersection {
         Pos2 pos;
         Line2d<T> edge;
         Vector2d<T> pt;
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

     static Rect<T> bind_points(const std::vector<Vector2d<T>>& pts) {
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
         std::vector<Vector2d<T>> pts;

         for (auto rect : rects) {
             pts.push_back(rect.pt);
             pts.push_back(rect.pt + rect.sz.vec_wh());
         }

         return bind_points(pts);
     }

     Vector2d<T> relative_point(const Vector2d<T>& pt) const {
         return {
             pt.x - pos.x,
             pt.y - pos.y
         };
     }

     bool contains_point(const Vector2d<T>& pt) const {
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

     Vector2d<T> center() const {
         return pos + Vector2d<T>(sz.w / 2, sz.h / 2);
     }

     std::vector<Vector2d<T>> corners() const {
         return {
             pos,
             pos + sz.vec_w(),
             pos + sz.vec_wh(),
             pos + sz.vec_h()
         };
     }

     std::vector<Line2d<T>> edges() const {
         const auto pts = corners();
         return {
             Line2d<T>(pts[0], pts[1]),
             Line2d<T>(pts[1], pts[2]),
             Line2d<T>(pts[2], pts[3]),
             Line2d<T>(pts[3], pts[0])
         };
     }

     bool intersects_line(const Line2d<T>& line) const {
         if (contains_point(line.a) || contains_point(line.b)) {
             return true;
         }

         return intersects_line_at(line).size() > 0;
     }

     std::vector<Intersection>
     intersects_line_at(const Line2d<T>& line) const {
         std::optional<Vector2d<T>> pt;
         std::vector<std::tuple<Pos2, Line2d<T>, Vector2d<T>>> collisions;
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

     Rect<T> move(const Vector2d<T>& position) const {
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

     Vector2d<T> tile(const Size2d<T>& tile_sz, int tile_id) {
         auto row_len = sz.w / tile_sz.w;
         return pos + Vector2d<T>(
             tile_sz.w * (tile_id % row_len),
             tile_sz.h * (tile_id / row_len)
         );
     }

     Rect<T> translate(const Vector2d<T>& vec) const {
         return move(pos + vec);
     }

     Rect<T> operator+(const Vector2d<T>& vec) const {
         return translate(vec);
     }

     Rect<T> operator-(const Vector2d<T>& vec) const {
         return translate(-vec);
     }

     Rect<T>& operator+=(const Vector2d<T>& vec) {
         *this = *this + vec;
         return *this;
     }

     Rect<T>& operator-=(const Vector2d<T>& vec) {
         *this = *this - vec;
         return *this;
     }

     friend std::ostream& operator<<(std::ostream& out, const Rect<T>& rect) {
         out << "Rect<" << type_name<T>() << ">" << rect.repr();
         return out;
     }

     Vector2d<T> pos;
     Size2d<T> sz;
};

// ------------------------------------------------------------------
template<class T>
class Polygon {
public:
    enum class Winding {
        NONE = 0,
        CW = 1,
        CCW = -1
    };

    Polygon() : pts() { }
    Polygon(const std::vector<Vector2d<T>>& points) : pts(points) { }

    struct Intersection {
        int edge_id;
        Line2d<T> edge;
        Vector2d<T> pt;
    };

    struct Collision {
        bool vertex;
        bool edge;
    };

    bool contains_point(const Vector2d<T>& pt) const {
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

    const std::vector<Line2d<T>> edges() const {
        std::vector<Line2d<T>> edges;

        for (int x = 0; x < pts.size(); x++) {
            if (x < pts.size() - 1) {
                edges.push_back(Line2d<T>(pts[x], pts[x + 1]));
            } else {
                edges.push_back(Line2d<T>(pts[x], pts[0]));
            }
        }

        return edges;
    }

    bool intersects_line(const Line2d<T>& line) const {
        if (contains_point(line.a) || contains_point(line.b)) {
            return true;
        }

        return intersects_line_at(line).size() > 0;
    }

    std::vector<Intersection>
    intersects_line_at(const Line2d<T>& line) const {
        std::optional<Vector2d<T>> pt;
        std::vector<std::tuple<int, Line2d<T>, Vector2d<T>>> collisions;
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
            Vector2d<T> axis = {
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
            Vector2d<T> axis = {
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

    Line1d<T> project_onto_axis(const Vector2d<T>& axis) {
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

    const std::vector<Vector2d<T>> normals() const {
        std::vector<Vector2d<T>> normals;

        for (auto edge : edges()) {
            auto vec = edge.as_vec();
            normals.push_back(Vector2d<T>(vec.y, -vec.x).normalize());
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

    std::vector<Vector2d<T>> pts;
};

// ------------------------------------------------------------------
template<class ID>
class CollisionTree {
public:
    struct Entry {
        ID id;
        Rect<float> aabb;
    };

    struct Intersection {
        ID id;
        Rect<float> aabb;
        Rect<float>::Intersection intersect;
    };

    CollisionTree(const Rect<float>& rect, int level = 0,
                  int max_level = 5, int max_objects = 10)
    : _rect(rect), _level(level), _max_level(max_level), _max_objects(max_objects) { }

    virtual ~CollisionTree() { }

    void insert(const ID& id, const Rect<float>& aabb) {
        insert({id, aabb});
    }

    void insert(const Entry& entry) {
        _entries.insert({entry.id, entry});

        for (auto& quadrant : _quadrants()) {
            if (entry.aabb.intersects_rect(quadrant.rect())) {
                quadrant.insert(entry);
            }
        }
    }

    const Rect<float>& rect() const {
        return _rect;
    }

    const std::vector<CollisionTree<ID>>& quadrants() const {
        return _quadrants;
    }

    bool contains(const ID& id) const {
        return _entries.contains(id);
    }

    std::set<ID> candidate_ids(const ID& id) const {
        if (! contains(id)) {
            return {};
        }

        std::set<ID> results;

        if (_quadrants.empty()) {
            for (auto it = _entries.begin(); it != _entries.end(); it++) {
                results.insert(it->first);
            }

        } else {
            for (const auto& quadrant : _quadrants) {
                for (ID id : quadrant.candidates(id)) {
                    results.insert(id);
                }
            }
        }

        results.erase(id);
        return results;
    }

    std::vector<Entry> candidate_entries(const ID& id) const {
        std::vector<Entry> results;

        for (const auto& candidate_id : candidates(id)) {
            results.push_back(_entries.at(candidate_id));
        }

        return results;
    }

    std::vector<Entry> intersection_entries(const ID& id) const {
        const auto candidates = candidate_entries(id);
        if (candidates.empty()) {
            return {};
        }

        std::vector<Entry> results;
        const auto entry = _entries.at(id);

        for (const auto& candidate : candidates) {
            if (entry.aabb.intersects_rect(candidate.aabb)) {
                results.push_back(candidate);
            }
        }

        return results;
    }

    std::vector<Intersection> intersections(const ID& id) const {
        std::vector<Intersection> results;
        const auto entries = intersection_entries(id);
        if (entries.empty()) {
            return {};
        }

        const auto entry = _entries.at(id);

        for (const auto intersect_entry : entries) {
            const auto rectA = entry.aabb;
            const auto rectB = intersect_entry.aabb;

            if (rectA.contains_point(rectB.center())) {

            } else if (rectB.contains_point(rectA.center())) {

            }

            const auto midline = Line2d<float>{
                intersect_entry.aabb.center(),
                entry.aabb.center()
            };
        }

        return results;
    }

    void clear() {
        _quadrants.clear();
        _entries.clear();
    }

protected:
    void split() {
        auto sz = _rect.sz / 2.0;
        const std::vector<Rect<float>> quadrects = {
            {_rect.pos,                               sz},
            {_rect.pos + Vector2d<float>{0.0, sz.w},  sz},
            {_rect.pos + Vector2d<float>{sz.h, 0.0},  sz},
            {_rect.pos + Vector2d<float>{sz.w, sz.h}, sz}
        };

        _quadrants.clear();

        for (const auto& quadrect : quadrects) {
            _quadrants.push_back(
                CollisionTree<ID>(quadrect, _level + 1, _max_level, _max_objects));
        }

        for (const auto& entry : _entries) {
            for (const auto& quadrant : _quadrants) {
                if (entry.aabb.intersects_rect(quadrant.rect())) {
                    quadrant.insert(entry);
                }
            }
        }
    }

    void collapse() {
        _quadrants.clear();
    }

private:
    int _level, _max_level, _max_objects;
    Rect<float> _rect;
    std::unordered_map<ID, Entry> _entries;
    std::vector<CollisionTree<ID>> _quadrants;
};

}
}


#endif /* !__MOONLIGHT_GEOMETRY_H */
