/*
 * finally.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday January 19, 2022
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_FINALLY_H
#define __MOONLIGHT_FINALLY_H

#include <functional>

namespace moonlight {
namespace core {

/**
 * An object to facilitiate finalization when not in the
 * scope of an object.  For example, this object can be used
 * to ensure that a function will be called when a scope
 * is exited, whether it left through the normal flow of
 * execution or via an exception.
 *
 * Note that this will not be called for POSIX signals
 * which interrupt execution.
 *
 * Example Usage:
 *
 * void perform_task(promise<string>& string_promise,
 *                   function<void()> user_defined_task) {
 *
 *    bool success = false;
 *    string failure_reason = "i don't know";
 *
 *    Finalizer finally([&]() {
 *       if (success) {
 *          string_promise.set_value("It's done!");
 *
 *       } else {
 *          string_promise.set_value("It failed, because " +
 *             failure_reason);
 *       }
 *    });
 *
 *    try {
 *       user_defined_task();
 *       success = true;
 *    } catch (const Exception& e) {
 *       failure_reason = e.get_message();
 *    }
 * }
 */
class Finalizer {
public:
	Finalizer(std::function<void()> closure) : closure(closure) { }
	virtual ~Finalizer() {
		closure();
	}

private:
	std::function<void()> closure;
};

}
}


#endif /* !__MOONLIGHT_FINALLY_H */
