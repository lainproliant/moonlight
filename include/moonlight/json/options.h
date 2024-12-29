/*
 * options.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Thursday December 26, 2024
 */

#ifndef __MOONLIGHT_JSON_OPTIONS_H
#define __MOONLIGHT_JSON_OPTIONS_H

namespace moonlight {
namespace json {

struct FormatOptions {
    bool pretty = false;
    bool spacing = true;
    bool sort_keys = false;
    int indent = 4;
};

}
}


#endif /* !__MOONLIGHT_JSON_OPTIONS_H */
