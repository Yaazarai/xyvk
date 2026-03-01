#pragma once
#ifndef __XYVK_DISPOSABLE
#define __XYVK_DISPOSABLE
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
		#ifndef DISPOSABLE_BOOL_DEFAULT
			#define DISPOSABLE_BOOL_DEFAULT true
		#endif

		class xyvk_disposable {
		protected:
			std::atomic_bool disposed = false;

		public:
			xyvk_invoker<bool> onDispose;

			void Dispose() {
				if (disposed) return;
				onDispose.invoke(DISPOSABLE_BOOL_DEFAULT);
				disposed = true;
			}

			bool IsDisposed() { return disposed; }
		};
	}

#endif