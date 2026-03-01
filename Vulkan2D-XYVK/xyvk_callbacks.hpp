/// @brief Source: https://stackoverflow.com/questions/9568150/what-is-a-c-delegate/9568485#9568485
#pragma once
#ifndef __XYVK_CALLBACK
#define __XYVK_CALLBACK
    #include "./xyvk_engine.hpp"
    #include <functional>
    #include <vector>
    #include <mutex>

    namespace XYVK_NAMESPACE {
        template<typename... A>
        class xyvk_callback {
        public:
            size_t hash;
            std::function<void(A...)> bound;

            xyvk_callback(std::function<void(A...)> func) : hash(func.target_type().hash_code()), bound(std::move(func)) {}
            bool compare(const xyvk_callback<A...>& cb) { return hash == cb.hash; }
            constexpr size_t hash_code() const throw() { return hash; }
            xyvk_callback<A...>& invoke(A... args) { bound(static_cast<A&&>(args)...); return (*this); }
        };

        template<typename... A>
        class xyvk_invoker {
        public:
            std::timed_mutex safety_lock;
            std::vector<xyvk_callback<A...>> callbacks;
            
            bool hook(const xyvk_callback<A...> cb) {
                xyvk_timedlock<> tg(safety_lock);
                bool signaled = tg.Signaled();
                if (signaled)
                    callbacks.push_back(cb);
                return signaled;
            }

            bool invoke(A... args) {
                xyvk_timedlock<> tg(safety_lock);
                bool signaled = tg.Signaled();
                if (signaled)
                    for (xyvk_callback<A...> cb : callbacks)
                        cb.invoke(static_cast<A&&>(args)...);
                return signaled;
            }
        };
    }
#endif