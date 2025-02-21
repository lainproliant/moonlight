/*
 * box2d3.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday September 3, 2024
 */

#ifndef __MOONLIGHT_BOX2D3_H
#define __MOONLIGHT_BOX2D3_H

#include <vector>
#include <box2d/box2d.h>

namespace moonlight {
namespace box2d {

// ------------------------------------------------------------------
class WorldTemplate {
public:
    WorldTemplate() {
        def = b2DefaultWorldDef();
    }

    b2WorldId create_world() const {
        return b2CreateWorld(&def);
    }

    b2WorldDef def;
};

// ------------------------------------------------------------------
class BodyTemplate {
public:
    BodyTemplate() {
        def = b2DefaultBodyDef();
    }

    b2BodyId create_body(b2WorldId world_id) const {
        return b2CreateBody(world_id, &def);
    }

    b2BodyDef def;
};

// ------------------------------------------------------------------
class ShapeTemplate {
public:
    ShapeTemplate() {
        def = b2DefaultShapeDef();
    }

    virtual void assign_to_body(b2BodyId body_id) const = 0;

    b2ShapeDef def;
};

// ------------------------------------------------------------------
struct Group {
    std::vector<b2BodyId> bodies;
    std::vector<b2JointId> joints;

    void destroy() {
        for (auto& joint : joints) {
            b2DestroyJoint(joint);
        }

        for (auto& body : bodies) {
            b2DestroyBody(body);
        }
    }
};

// ------------------------------------------------------------------
class GroupTemplate {
public:
    virtual Group create() const = 0;
};

}
}


#endif /* !__MOONLIGHT_BOX2D3_H */
