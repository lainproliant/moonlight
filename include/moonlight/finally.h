/*
 * ## finally.h -----------------------------------------------------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday January 19, 2022
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ---------------------------------------------------------
 * This header provides `core::Finalizer`, an RAII object to facilitiate
 * finalization when the `Finalizer` object leaves scope.  For example, this
 * object can be used to ensure that a function will be called when a scope is
 * exited, whether it left through the normal flow of execution or via an
 * exception.
 *
 * The `core::Finalizer` object can be provided with any single nullary function
 * to call when scope is exited.  If more complex logic is required, simply
 * provide the `Finalizer` with a nullary closure or function object which
 * implements this behavior via `operator()`.  Note that it will not be called
 * for POSIX signals which interrupt execution.
 *
 * See the example use case below:
 *
 * ```
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
 * ```
 */
#ifndef __MOONLIGHT_FINALLY_H
#define __MOONLIGHT_FINALLY_H

#include <functional>

namespace moonlight {
namespace core {

// ------------------------------------------------------------------
class Finalizer {
 public:
     explicit Finalizer(std::function<void()> closure) : closure(closure) { }
     virtual ~Finalizer() {
         closure();
     }

 private:
     std::function<void()> closure;
};

}  // namespace core
}  // namespace moonlight


#endif /* !__MOONLIGHT_FINALLY_H */
