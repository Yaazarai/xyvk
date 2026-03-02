#pragma once
#ifndef __XYVK_VERTEXQUAD
#define __XYVK_VERTEXQUAD
    #include "./xyvk_engine.hpp"

    namespace XYVK_NAMESPACE {
        /// @brief Default math coordinate and 2D camera projection functionality.
        class xyvk_vertexmath {
	    public:
            const static glm::mat4 Project2D(double width, double height, double camerax, double cameray, double znear = 1.0, double zfar = 0.0) {
                //return glm::transpose(glm::mat4(
                //    2.0/(width - 1), 0.0, 0.0, -1.0,
                //    0.0, 2.0/(height-1), 0.0, -1.0,
                //    0.0, 0.0, -2.0/(zfar-znear), -((zfar+znear)/(znear-zfar)),
                //    0.0, 0.0, 0.0, 1.0));
                // Defining the TOP and BOTTOM upside down will provide the proper translation transform for scaling
                // with Vulkan due to Vulkan's inverted Y-Axis without having to transpose the matrix.
                glm::mat4 projection = glm::ortho(0.0, width, 0.0, height, znear, zfar);
                return glm::translate(projection, glm::vec3(camerax, cameray, 0.0));

            }
            
            const static glm::vec2 GetUVCoords(glm::vec2 xy, glm::vec2 wh, bool forceClamp = true) {
                if (forceClamp)
                    xy = glm::clamp(xy, glm::vec2(0.0, 0.0), wh);
                return xy * (glm::vec2(1.0, 1.0) / wh);
            }

            const static glm::vec2 GetXYCoords(glm::vec2 uv, glm::vec2 wh, bool forceClamp = true) {
                if (forceClamp)
                    uv = glm::clamp(uv, glm::vec2(0.0, 0.0), glm::vec2(1.0, 1.0));

                return glm::vec2(uv.x * wh.x, uv.y * wh.y);
            }

            const static glm::float32 AngleClamp(glm::float32 a) {
                #ifndef GLM_FORCE_RADIANS
                return std::fmod((360.0f + std::fmod(a, 360.0f)), 360.0f);
                #else
                constexpr glm::float32 pi2 = glm::pi<glm::float32>() * 2.0f;
                return std::fmod((pi2 + std::fmod(a, pi2)), pi2);
                #endif
            }

            const static glm::float32 AngleDelta(glm::float32 a, glm::float32 b) {
                //// https://gamedev.stackexchange.com/a/4472
                glm::float32 absa, absb;
                #ifndef GLM_FORCE_RADIANS
                absa = std::fmod((360.0f + std::fmod(a, 360.0f)), 360.0f);
                absb = std::fmod((360.0f + std::fmod(b, 360.0f)), 360.0f);
                glm::float32 delta = glm::abs(absa - absb);
                glm::float32 sign = absa > absb || delta >= 180.0f ? -1.0f : 1.0f;
                return (180.0f - glm::abs(delta - 180.0f) * sign;
                #else
                constexpr glm::float32 pi = glm::pi<glm::float32>();
                constexpr glm::float32 pi2 = pi * 2.0f;
                absa = std::fmod((pi2 + std::fmod(a, pi2)), pi2);
                absb = std::fmod((pi2 + std::fmod(b, pi2)), pi2);
                glm::float32 delta = glm::abs(absa - absb);
                glm::float32 sign = (absa > absb || delta >= pi) ? -1.0f : 1.0f;
                return (pi - glm::abs(delta - pi)) * sign;
                #endif
            }
	    
			template<typename T>
			static size_t GetSizeofVector(std::vector<T> vector) { return vector.size() * sizeof(T); }

			template<typename T, size_t S>
			static size_t GetSizeofArray(std::array<T,S> array) { return S * sizeof(T); }
        };

        class xyvk_vertexquad {
        public:
            std::array<xyvk_vertex, 6> vertices;
            glm::vec2 origin, position, extent;
            glm::float32 depth, rotation;
            glm::vec4 uv;
            
            xyvk_vertexquad(glm::vec2 extent, glm::float32 depth = 1.0, glm::vec2 xy = glm::vec2(0.0), glm::vec2 origin = glm::vec2(0.0), glm::vec2 position = glm::vec2(0.0), glm::float32 rotation = glm::float32(0.0), glm::vec4 uv = glm::vec4(0.0, 0.0, 1.0, 1.0))
            : origin(origin), position(position), extent(extent), depth(depth), rotation(rotation), uv(uv) { Vertices(); }
            
            xyvk_vertexquad& Scale(glm::vec2 scalar) { extent *= scalar; return (*this); }
            xyvk_vertexquad& Resize(glm::vec2 wh) { extent = wh; return (*this); }
            xyvk_vertexquad& Rotate(glm::float32 radians, bool relative) { rotation = glm::mod(((relative)? (rotation + radians) : radians) + glm::two_pi<glm::float32>(), glm::two_pi<glm::float32>()); return (*this); }
            xyvk_vertexquad& Translate(glm::vec2 xy) { position += xy; return (*this); }
            xyvk_vertexquad& Position(glm::vec2 xy) { position = xy; return (*this); }
            xyvk_vertexquad& Origin(glm::vec2 xy) { origin = xy; return (*this); }
            xyvk_vertexquad& Depth(glm::float32 d) { depth = d; return (*this); }
            xyvk_vertexquad& TextCoords(glm::vec2 uv_xy1, glm::vec2 uv_xy2) { uv = glm::vec4(uv_xy1, uv_xy2); return (*this); }
            xyvk_vertexquad& VertexColor(size_t index, glm::vec4 vcolor) { vertices[index].color = vcolor; return (*this); }
            xyvk_vertexquad& VerticesColor(glm::vec4 vcolor) { for(size_t i = 0; i < 6; i++) vertices[i].color = vcolor; return (*this); }
            size_t SizeofQuad() { return sizeof(xyvk_vertex) * 6ULL; }

            const static std::vector<xyvk_vertex> GetVertexVector(std::vector<std::array<xyvk_vertex, 6>> quads) {
                std::vector<xyvk_vertex> vertices;
                for(std::array<xyvk_vertex, 6> quad : quads)
                    vertices.insert(vertices.end(), quad.begin(), quad.end());
                return vertices;
            }
            
            glm::vec4 GetAtlasUVs(glm::vec2 xy, glm::vec2 wh, glm::vec2 atlas) {
                glm::vec2 uv1, uv2;
                uv1 = xy / atlas;
                uv2 = uv1 + (wh / atlas);
                return glm::vec4(uv1, uv2);
            }

            std::array<xyvk_vertex, 6>& Vertices() {
                glm::mat2 rotmatrix = glm::mat2(glm::cos(rotation), -glm::sin(rotation), glm::sin(rotation), glm::cos(rotation));
                glm::vec2 pivot = position + origin;
                glm::vec2 xy1 = position + origin;
                glm::vec2 xy2 = xy1 + extent;
                glm::vec2 uv1 = glm::vec2(uv.x, uv.y);
                glm::vec2 uv2 = glm::vec2(uv.z, uv.w);

                vertices = {
                    xyvk_vertex({uv1.x, uv1.y}, {xy1.x, xy1.y, depth}, vertices[0].color),
                    xyvk_vertex({uv2.x, uv1.y}, {xy2.x, xy1.y, depth}, vertices[1].color),
                    xyvk_vertex({uv1.x, uv2.y}, {xy1.x, xy2.y, depth}, vertices[2].color),

                    xyvk_vertex({uv2.x, uv1.y}, {xy2.x, xy1.y, depth}, vertices[3].color),
                    xyvk_vertex({uv2.x, uv2.y}, {xy2.x, xy2.y, depth}, vertices[4].color),
                    xyvk_vertex({uv1.x, uv2.y}, {xy1.x, xy2.y, depth}, vertices[5].color),
                };

                for(size_t i = 0; i < 6; i++) {
                    glm::vec2 xypos = glm::vec2(vertices[i].position);
                    xypos -= pivot;
                    xypos = rotmatrix * xypos;
                    xypos += pivot;
                    vertices[i].position = glm::vec3(xypos, depth);
                }

                return vertices;
            }
        };
    }
#endif