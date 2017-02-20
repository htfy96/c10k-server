//
// Created by lz on 1/21/17.
//

#ifndef C10K_SERVER_UTILS_HPP
#define C10K_SERVER_UTILS_HPP


#include <cstdio>
#include <cerrno>
#include <exception>
#include <string>
#include <cstring>
#include <cstdlib>
#include <system_error>
#include <utility>
#include <type_traits>

namespace c10k
{
    namespace detail {
        // if a syscall returns <0, throw an std::system_error
        template<typename FT, typename ... Ts>
        int call_must_ok(FT f, const char *msg, Ts &&... ts) {
            int ret = f(std::forward<Ts>(ts)...);
            if (ret < 0) {
                throw std::system_error(std::error_code(errno, std::system_category()));
            }
            return ret;
        }

        // Use make_scope_guard instead
        template<typename FT1, typename FT2>
        struct ScopeGuard
        {
        private:
            bool enabled = true;
            FT1 f_in; FT2 f_exit;
        public:
            ScopeGuard(FT1 f1, FT2 f2):
                    f_in(f1), f_exit(f2)
            {
                f_in();
            }

            ScopeGuard(ScopeGuard && other):
                    enabled(other.enabled),
                    f_in(other.f_in),
                    f_exit(other.f_exit)
            {
                other.enabled = false;
            }

            ScopeGuard &operator= (ScopeGuard &&) = delete; // TODO

            ScopeGuard(const ScopeGuard &) = delete;
            ScopeGuard &operator= (const ScopeGuard &) = delete;


            ~ScopeGuard()
            {
                if (enabled)
                    f_exit();
            }
        };

        // make_scope_guard(enter_fun, exit_fun)
        template<typename FT1, typename FT2>
        ScopeGuard<FT1, FT2> make_scope_guard(FT1 f1, FT2 f2)
        {
            return ScopeGuard<FT1, FT2>(f1, f2);
        };

        template<typename FT>
        auto make_exit_guard(FT f_at_exit) {
            return make_scope_guard([]() {}, f_at_exit);
        }

        // Adapted from http://stackoverflow.com/a/16824239/4140668
#define C10K_GEN_HAS_MEMBER(ClassName, MemberName) \
        template<typename, typename T> \
        struct ClassName { \
            static_assert( \
                    std::integral_constant<T, false>::value, \
                    "Second template parameter needs to be of function type."); \
        }; \
        template<typename C, typename Ret, typename... Args> \
        struct ClassName<C, Ret(Args...)> { \
        private: \
            template<typename T> \
            static constexpr auto check(T*) \
            -> typename \
            std::is_same< \
                    decltype( std::declval<T>().MemberName( std::declval<Args>()... ) ), \
                    Ret    \
            >::type;  \
            template<typename> \
            static constexpr std::false_type check(...); \
            typedef decltype(check<C>(0)) type; \
        public: \
            static constexpr bool value = type::value; \
        };


    }

}

#endif //C10K_SERVER_UTILS_HPP
