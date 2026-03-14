#pragma once
#ifndef __XYVK_FRAME
#define __XYVK_FRAME
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
        template<typename T_FRAMEOBJECT, typename T_CREATEINFOS, size_t FRAMES = static_cast<size_t>(XYVK_BUFFERINGMODE::DOUBLE)>
		class xyvk_frame : public xyvk_disposable {
		public:
			xyvk_graph& renderGraph;
            std::array<T_FRAMEOBJECT*, static_cast<size_t>(FRAMES)> framesInFlight;

			xyvk_frame operator=(const xyvk_frame&) = delete;
			xyvk_frame(const xyvk_frame&) = delete;
			~xyvk_frame() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) renderGraph.vkdevice.WaitIdle();
				
				for(T_FRAMEOBJECT* frame : framesInFlight)
					delete frame;
			}

            xyvk_frame(xyvk_graph& renderGraph, T_CREATEINFOS createInfos = T_CREATEINFOS()) : renderGraph(renderGraph) {
				onDispose.hook(xyvk_callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				for(size_t i = 0; i < static_cast<size_t>(FRAMES); i++)
					framesInFlight[i] = new T_FRAMEOBJECT(renderGraph.vkdevice, createInfos);
			}

			std::pair<T_FRAMEOBJECT*, size_t> GetCurrentFrame() {
				size_t index = renderGraph.frameCounter % static_cast<size_t>(FRAMES);
				return std::pair<T_FRAMEOBJECT*, size_t>(framesInFlight[index], renderGraph.frameCounter);
			}
		};
	}

#endif