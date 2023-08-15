#include "video_renderer.h"
#include <algorithm>
#include <string>
#include <thread>
#include "log_utils.h"
#include <GL/wglew.h>

#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

const char* const VideoRenderer::TAG = "VideoRenderer";

VideoRenderer::VideoRenderer(
    HWND wnd, int32_t viewport_width, int32_t viewport_height) : hwnd_(wnd),
    texture_y_(-1), texture_u_(-1), texture_v_(-1),
    uniform_y_(-1), uniform_u_(-1), uniform_v_(-1) {
    hdc_ = GetDC(hwnd_);
    viewport_width_ = viewport_width;
    viewport_height_ = viewport_height;

    std::thread gl_thread([this]() {
        Init();
        while (is_rendering_) {
            if (is_frame_active_) {
                std::unique_lock<std::mutex>(frame_mutex_);
                RenderFrame(video_frame_);
                is_frame_active_ = false;
            }
        }
        DestroyContext();
    });
    gl_thread.detach();
    
}

VideoRenderer::~VideoRenderer() {
    is_rendering_ = false;
}

void VideoRenderer::Init() {
    DestroyContext();
    if (!InitContext()) {
        tcrsdk::LogUtils::w(TAG, "VideoRenderer init context failed");
        return;
    }
    InitShaders();
    InitTexture();
    is_inited_ = true;
}

bool VideoRenderer::InitContext() {
    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
        PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
        32,                        //Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                        //Number of bits for the depthbuffer
        8,                        //Number of bits for the stencilbuffer
        0,                        //Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    int nPixelFormat = ChoosePixelFormat(hdc_, &pfd);
    if (nPixelFormat == 0) {
        return false;
    }
    bool bResult = SetPixelFormat(hdc_, nPixelFormat, &pfd);
    if (!bResult) {
        return false;
    }

    hrc_ = wglCreateContext(hdc_);
    if (!wglMakeCurrent(hdc_, hrc_)) {
        return false;
    }

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        return false;
    }
    return true;
}

void VideoRenderer::InitShaders() {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        tcrsdk::LogUtils::w(TAG, "OpenGL error: " + err);
    }
    std::string vsh_str =
        "attribute vec4 vertexIn;\n"
        "attribute vec2 textureIn;\n"
        "varying vec2 textureOut;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = vertexIn;\n"
        "    textureOut = textureIn;\n"
        "}\n";

    std::string fsh_str =
        "varying vec2 textureOut;\n"
        "uniform sampler2D tex_y;\n"
        "uniform sampler2D tex_u;\n"
        "uniform sampler2D tex_v;\n"
        "void main(void)\n"
        "{\n"
        "  float y = texture2D(tex_y, textureOut).r * 1.16438;\n"
        "  float u = texture2D(tex_u, textureOut).r;\n"
        "  float v = texture2D(tex_v, textureOut).r;\n"
        "  gl_FragColor = vec4(y + 1.59603 * v - 0.874202,\n"
        "    y - 0.391762 * u - 0.812968 * v + 0.531668,\n"
        "    y + 2.01723 * u - 1.08563, 1);\n"
        "}\n";
    GLint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertex_shader_source = vsh_str.c_str();
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    GLint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragment_shader_source = fsh_str.c_str();
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vertex_shader);
    glAttachShader(shader_program_, fragment_shader);
    glBindAttribLocation(shader_program_, ATTRIB_VERTEX, "vertexIn");
    glBindAttribLocation(shader_program_, ATTRIB_TEXTURE, "textureIn");

    glLinkProgram(shader_program_);
    glUseProgram(shader_program_);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

void VideoRenderer::CalcRenderPos() {
    static const GLfloat vertexVertices[] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f,  1.0f,
            1.0f,  1.0f,
    };

    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };

    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
    //Enable it
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
    glEnableVertexAttribArray(ATTRIB_TEXTURE);
}

void VideoRenderer::InitTexture() {
    CalcRenderPos();
    uniform_y_ = glGetUniformLocation(shader_program_, "tex_y");
    uniform_u_ = glGetUniformLocation(shader_program_, "tex_u");
    uniform_v_ = glGetUniformLocation(shader_program_, "tex_v");

    glGenTextures(1, &texture_y_);
    glBindTexture(GL_TEXTURE_2D, texture_y_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &texture_u_);
    glBindTexture(GL_TEXTURE_2D, texture_u_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &texture_v_);
    glBindTexture(GL_TEXTURE_2D, texture_v_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void VideoRenderer::DestroyContext() {
    wglDeleteContext(hrc_);
    wglMakeCurrent(NULL, NULL);
}

void VideoRenderer::OnFrame(const tcrsdk::VideoFrame& video_frame) {
    std::unique_lock<std::mutex> lock(frame_mutex_);
    video_frame_ = video_frame;
    is_frame_active_ = true;
}

void VideoRenderer::OnSurfaceChanged(int32_t width, int32_t height)
{
    viewport_width_ = width;
    viewport_height_ = height;
}

void VideoRenderer::RenderFrame(const tcrsdk::VideoFrame& video_frame) {
    if (!is_inited_) {
        return;
    }

    glViewport(0, 0, viewport_width_, viewport_height_);

    int32_t width = video_frame.width();
    int32_t height = video_frame.height();
    wglMakeCurrent(hdc_, hrc_);
    std::shared_ptr<tcrsdk::I420Buffer> buffer = video_frame.i420_buffer();

    if (render_width_ != width || render_height_ != height) {
        render_width_ = width;
        render_height_ = height;
    }
    glClearColor(0.0, 0.0, 0.0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_y_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, render_width_, render_height_,
        0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer->DataY());
    glUniform1i(uniform_y_, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_u_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, render_width_ / 2,
        render_height_ / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
        buffer->DataU());
    glUniform1i(uniform_u_, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_v_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, render_width_ / 2,
        render_height_ / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
        buffer->DataV());
    glUniform1i(uniform_v_, 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glFlush();
    SwapBuffers(hdc_);
}

