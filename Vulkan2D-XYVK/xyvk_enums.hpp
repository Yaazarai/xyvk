#pragma once
#ifndef __XYVK_ENUMCLASSES
#define __XYVK_ENUMCLASSES
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
		enum class XYVK_SHADERTYPE { STAGING, COMPUTE, GRAPHICS, PRESENT };
		enum class XYVK_BUFFERTYPE { VERTEX, UNIFORM, STAGING };
		enum class XYVK_IMAGETYPE { SWAPCHAIN, ATTACHEMENT };
		enum class XYVK_BUFFERINGMODE { SINGLE, DOUBLE, TRIPLE };
		enum class XYVK_CMDBUFFERSTAGE { PIPE_TOP, PIPE_BOT, PIPE_ALL };
		
		enum class XYVK_DESCRIPTORTYPE {
			IMAGE_SAMPLER = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			UNIFORM_BUFFER = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		};

		enum class XYVK_SUBPASSTYPE {
			UNDEFINED, FRAGMENT, PRESENT, COMPUTE, TRANSFER
		};

		enum class XYVK_SHADERSTAGES {
			UNDEFINED,
			VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
			FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
			COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT
		};

		enum class XYVK_IMAGELAYOUT {
			TRANSFER_SRC = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			TRANSFER_DST = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			SHADER_READONLY = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			UNINITIALIZED = VK_IMAGE_LAYOUT_UNDEFINED,
			COLOR_ATTACHMENT = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			PRESENT_SRCKHR = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};
		
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		enum class XYVK_WINDOWEVENTS {
			POLL_EVENTS,
			WAIT_EVENTS
		};
		
		enum class XYVK_INPUTEVENTS {
			RELEASE = GLFW_RELEASE,
			PRESS = GLFW_PRESS,
		};
	}

#endif