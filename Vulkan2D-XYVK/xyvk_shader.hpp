#pragma once
#ifndef __XYVK_SHADER
#define __XYVK_SHADER
	#include "./xyvk_engine.hpp"

	namespace XYVK_NAMESPACE {
		class xyvk_shader {
		public:
			xyvk_device& vkdevice;
			XYVK_SHADERSTAGES stage;
			std::vector<VkDescriptorSetLayoutBinding> descriptorBindings;
			std::string shaderpath;
			std::vector<uint32_t> spirv;
			VkResult initialized = VK_ERROR_INITIALIZATION_FAILED;

			xyvk_shader operator=(const xyvk_shader&) = delete;
			xyvk_shader(const xyvk_shader&) = delete;

			xyvk_shader(xyvk_device& vkdevice, XYVK_SHADERSTAGES stage, std::string shaderpath, std::vector<XYVK_DESCRIPTORTYPE> push_descriptors = {})
			: vkdevice(vkdevice), stage(stage), shaderpath(shaderpath) {
				for(uint32_t i = 0; i < push_descriptors.size(); i++) {
					VkDescriptorSetLayoutBinding layoutBinding = {
						.binding = 0, .descriptorCount = 1, .pImmutableSamplers = VK_NULL_HANDLE,
						.descriptorType = static_cast<VkDescriptorType>(push_descriptors[i]),
						.stageFlags = static_cast<VkShaderStageFlags>(stage)
					};
					descriptorBindings.push_back(layoutBinding);
				}

				initialized = VK_SUCCESS;

				std::ifstream file(shaderpath, std::ios::ate | std::ios::binary);
				if (!file.is_open()) initialized = VK_ERROR_INITIALIZATION_FAILED;
				size_t byte_size = static_cast<size_t>(file.tellg());
				spirv.resize(byte_size / sizeof(uint32_t));
				file.seekg(0);
				file.read(reinterpret_cast<char*>(spirv.data()), byte_size);
				file.close();
			}
		};
	}
#endif