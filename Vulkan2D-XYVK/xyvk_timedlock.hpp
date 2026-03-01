#pragma once
#ifndef __XYVK_TIMEDLOCK
#define __XYVK_TIMEDLOCK
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
        template<size_t timeout = 100>
        class _NODISCARD_LOCK xyvk_timedlock {
        public:
            std::atomic_bool signal = false;
            std::timed_mutex& lock;
            
            xyvk_timedlock(const xyvk_timedlock&) = delete;
            xyvk_timedlock& operator=(const xyvk_timedlock&) = delete;
            ~xyvk_timedlock() noexcept { if (signal) lock.unlock(); }

            explicit xyvk_timedlock(std::timed_mutex& lock) : lock(lock) { signal.store(lock.try_lock_for(std::chrono::milliseconds(timeout))); }
            bool Signaled() { return signal.load(); }
        };
    }
#endif