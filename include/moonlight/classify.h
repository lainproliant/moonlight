/*
 * classify.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday June 23, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_CLASSIFY_H
#define __MOONLIGHT_CLASSIFY_H

#include <functional>
#include <optional>
#include "moonlight/exceptions.h"

namespace moonlight {

namespace _private {

template <typename RT> struct ClassifierTraits {
    typedef std::optional<RT> Result;
};

template<> struct ClassifierTraits<void> {
    typedef void Result;
};

}

/** -----------------------------------------------------------------
 */
template<class T, class R = void>
class Classifier {
public:
    typedef std::function<bool(const T&)> MatchFunction;
    typedef std::function<R()> NullaryAction;
    typedef std::function<R(const T&)> UnaryAction;

    typedef _private::ClassifierTraits<R> Traits;

    class Action {
    public:
        Action(NullaryAction action) : _nullary(action), _unary({}) { }
        Action(UnaryAction action) : _nullary({}), _unary(action) { }

        typename Traits::Result nullary() const {
            return _nullary.value()();
        }

        typename Traits::Result unary(const T& value) const {
            return _unary.value()(value);
        }

        bool is_unary() const {
            return _unary.has_value();
        }

    private:
        std::optional<NullaryAction> _nullary;
        std::optional<UnaryAction> _unary;
    };

    class Case {
    public:
        template<class ActionImpl>
        Case(MatchFunction match, ActionImpl action) : _match(match), _action(Action(action)) { }

        typename Traits::Result apply(const T& value) const {
            if (_action.is_unary()) {
                return _action.unary(value);
            } else {
                return _action.nullary();
            }
        }

        bool match(const T& value) const {
            return _match(value);
        }

    private:
        MatchFunction _match;
        Action _action;
    };

    class CaseAssigner {
    public:
        CaseAssigner(Classifier& switch_, MatchFunction match) : _switch(switch_), _match(match) { }

        template<class ActionType>
        void operator=(ActionType action) {
            _switch._cases.push_back(Case(_match, action));
        }

    private:
        Classifier& _switch;
        MatchFunction _match;
    };

    template<class... MatchType>
    CaseAssigner operator()(const MatchType&... matches) {
        return CaseAssigner(*this, f_or_join(matches...));
    }

    CaseAssigner otherwise() {
        return CaseAssigner(*this, [](const T& value) { (void)value; return true; });
    }

    std::optional<Case> match(const T& value) const {
        for (auto case_ : _cases) {
            if (case_.match(value)) {
                return case_;
            }
        }
        return {};
    }

    typename Traits::Result apply(const T& value) const {
        auto case_ = match(value);
        if (case_.has_value()) {
            return case_->apply(value);
        } else {
            return typename Traits::Result();
        }
    }

private:
    static MatchFunction f_eq(const T& value) {
        return [=](const T& x) {
            return x == value;
        };
    }

    static MatchFunction f_or(MatchFunction fA, MatchFunction fB) {
        return [=](const T& x) {
            return fA(x) || fB(x);
        };
    }

    MatchFunction f_or_join(MatchFunction f) {
        return f;
    }

    MatchFunction f_or_join(const T& value) {
        return f_eq(value);
    }

    template<class... MatchTypePack>
    MatchFunction f_or_join(MatchFunction f, MatchTypePack... pack) {
        return f_or(f, f_or_join(pack...));
    }

    template<class... MatchTypePack>
    MatchFunction f_or_join(const T& value, MatchTypePack... pack) {
        return f_or(f_eq(value), f_or_join(pack...));
    }

    std::vector<Case> _cases;
};

}


#endif /* !__MOONLIGHT_CLASSIFY_H */
