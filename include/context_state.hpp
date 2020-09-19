#pragma once

#include <context_debug.hpp>
#include <buffer.hpp>

#include <glm/glm.hpp>

#include <map>
#include <unordered_map>

namespace GraphicsEngine {
    struct BindingPoint {
        Buffer::Type target;
        GLuint point;
    };
    bool operator<(const BindingPoint& a, const BindingPoint& b) noexcept;

    enum class Clear {
        Color = GL_COLOR_BUFFER_BIT,
        Depth = GL_DEPTH_BUFFER_BIT,
        Stencil = GL_STENCIL_BUFFER_BIT,
        ColorDepth = Color | Depth,
        ColorDepthStencil = ColorDepth | Stencil
    };

    enum class DepthFunc {
        Never = GL_NEVER,
        Less = GL_LESS,
        Equal = GL_EQUAL,
        Lequal = GL_LEQUAL,
        Greater = GL_GREATER,
        Notequal = GL_NOTEQUAL,
        Gequal = GL_GEQUAL,
        Always = GL_ALWAYS
    };

    enum class CullFace {
        Front = GL_FRONT,
        Back = GL_BACK,
        FrontBack = GL_FRONT_AND_BACK
    };

    enum class FrontFace {
        CW = GL_CW,
        CCW = GL_CCW
    };

    enum class PolygonMode {
        Point = GL_POINT,
        Line = GL_LINE,
        Fill = GL_FILL,
    };

    class ContextState {
    protected:
        std::unordered_map<GLenum, bool> enable_map;
        glm::uvec2 viewport {};
        glm::vec4 clear_color {};

        DepthFunc depth_func {DepthFunc::Less};
        GLboolean depth_mask {GL_TRUE};
        CullFace cull_face {CullFace::Back};
        FrontFace front_face {FrontFace::CCW};
        CullFace polygon_face {CullFace::FrontBack};
        PolygonMode polygon_mode {PolygonMode::Fill};

        GLuint shader_id {0};
        GLuint vertex_array_id {0};
        GLuint framebuffer_id {0};

        // contains [target, last buffer id]
        std::unordered_map<Buffer::Type, GLuint> buffer_target;
        // contains [binding point, last_buffer_id]
        std::map<BindingPoint, GLuint> buffer_point;

        GLuint active_texture {0};
        // contains [texture_image_unit, texture_id]
        std::map<GLuint, GLuint> texture_bound;

        ContextState() noexcept = default;
        void init() noexcept;

        static inline std::unordered_map<GLFWwindow*, ContextState*> state_map;
        void registerState(GLFWwindow* window);
        void unregisterState(GLFWwindow* window);

        friend class StateBuffer;
        friend class NamedBuffer;
        friend class ShaderProgram;
        friend class VertexArray;
        friend class StateTexture;
        friend class NamedTexture;
        friend class TextureBinder;
        friend class Framebuffer;
    public:
        virtual ~ContextState() = default;

        ContextState(const ContextState&) = delete;
        ContextState& operator=(const ContextState&) = delete;

        ContextState(ContextState&&) noexcept = default;
        ContextState& operator=(ContextState&&) noexcept = default;

        static bool hasState(GLFWwindow* window) noexcept;
        static ContextState* getState(GLFWwindow* window) noexcept;

        void setViewPort(glm::uvec2 viewport_size) noexcept;
        void clearColor(const glm::vec4& color) noexcept;
        void setDepthFunc(DepthFunc func) noexcept;
        void setDepthMask(GLboolean mask) noexcept;
        void setCullFace(CullFace mode) noexcept;
        void setFrontFace(FrontFace mode) noexcept;
        void setPolygonMode(CullFace face, PolygonMode mode) noexcept;
        void clear(Clear bits) noexcept;
        void disable(GLenum func) noexcept;
        void enable(GLenum func) noexcept;
    };
}
