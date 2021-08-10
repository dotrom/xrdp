
#if defined(HAVE_CONFIG_H)
#include <config_ac.h>
#endif

#if (!defined(XRDP_GLX)) && (!defined(XRDP_EGL))
#define XRDP_GLX
//#define XRDP_EGL
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>

#include <epoxy/gl.h>

#include "arch.h"
#include "os_calls.h"
#include "xorgxrdp_helper.h"
#include "xorgxrdp_helper_x11.h"

/* X11 */
static Display *g_display = NULL;
static int g_x_socket = 0;
static int g_screen_num = 0;
static Screen *g_screen = NULL;
static Window g_root_window = None;
static Visual *g_vis = NULL;
static GC g_gc;

//#undef XRDP_NVENC

#if defined(XRDP_NVENC)

#include "xorgxrdp_helper_nvenc.h"

/*****************************************************************************/
static int
xorgxrdp_helper_encoder_init(void)
{
    return xorgxrdp_helper_nvenc_init();
}

/*****************************************************************************/
static int
xorgxrdp_helper_encoder_delete_encoder(struct enc_info *ei)
{
    return xorgxrdp_helper_nvenc_delete_encoder(ei);
}

/*****************************************************************************/
static int
xorgxrdp_helper_encoder_create_encoder(int width, int height, int enc_texture,
                                       int tex_format, struct enc_info **ei)
{
    return xorgxrdp_helper_nvenc_create_encoder(width, height, enc_texture,
                                                tex_format, ei);
}

/*****************************************************************************/
static int
xorgxrdp_helper_encoder_encode(struct enc_info *ei, int enc_texture,
                               void *cdata, int *cdata_bytes)
{
    return xorgxrdp_helper_nvenc_encode(ei, enc_texture, cdata, cdata_bytes);
}

#else

struct enc_info
{
    int pad0;
};

/*****************************************************************************/
static int
xorgxrdp_helper_encoder_init(void)
{
    return 1;
}

/*****************************************************************************/
static int
xorgxrdp_helper_encoder_delete_encoder(struct enc_info *ei)
{
    return 1;
}

/*****************************************************************************/
static int
xorgxrdp_helper_encoder_create_encoder(int width, int height, int enc_texture,
                                       int tex_format, struct enc_info **ei)
{
    return 1;
}

/*****************************************************************************/
static int
xorgxrdp_helper_encoder_encode(struct enc_info *ei, int enc_texture,
                               void *cdata, int *cdata_bytes)
{
    return 1;
}

#endif

#if defined(XRDP_GLX)

#include <epoxy/glx.h>

static int g_n_fbconfigs = 0;
static int g_n_pixconfigs = 0;
static GLXFBConfig *g_fbconfigs = NULL;
static GLXFBConfig *g_pixconfigs = NULL;
static GLXContext g_gl_context = 0;

static const int g_fbconfig_attrs[] =
{
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    GLX_DOUBLEBUFFER,  True,
    GLX_RED_SIZE,      8,
    GLX_GREEN_SIZE,    8,
    GLX_BLUE_SIZE,     8,
    None
};

static const int g_pixconfig_attrs[] =
{
    GLX_BIND_TO_TEXTURE_RGBA_EXT,       True,
    GLX_DRAWABLE_TYPE,                  GLX_PIXMAP_BIT,
    GLX_BIND_TO_TEXTURE_TARGETS_EXT,    GLX_TEXTURE_2D_BIT_EXT,
    GLX_DOUBLEBUFFER,                   False,
    GLX_Y_INVERTED_EXT,                 True,
    None
};

static const int g_pixmap_attribs[] =
{
    GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
    GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGBA_EXT,
    None
};

typedef GLXPixmap inf_image_t;

/*****************************************************************************/
static int
xorgxrdp_helper_inf_init(void)
{
    const char *ext_str;
    int glx_ver;
    int ok;

    if (!epoxy_has_glx(g_display))
    {
        return 1;
    }
    glx_ver = epoxy_glx_version(g_display, g_screen_num);
    g_writeln("glx_ver %d", glx_ver);
    if (glx_ver < 11) /* GLX version 1.1 */
    {
        g_writeln("glx_ver too old %d", glx_ver);
        return 1;
    }
    if (!epoxy_has_glx_extension(g_display, g_screen_num,
                                 "GLX_EXT_texture_from_pixmap"))
    {
        ext_str = glXQueryExtensionsString(g_display, g_screen_num);
        g_writeln("GLX_EXT_texture_from_pixmap not present [%s]", ext_str);
        return 1;
    }

    g_writeln("GLX_EXT_texture_from_pixmap present");
    g_fbconfigs = glXChooseFBConfig(g_display, g_screen_num,
                                    g_fbconfig_attrs, &g_n_fbconfigs);
    g_writeln("g_fbconfigs %p", g_fbconfigs);
    g_gl_context = glXCreateNewContext(g_display, g_fbconfigs[0],
                                       GLX_RGBA_TYPE, NULL, 1);
    g_writeln("g_gl_context %p", g_gl_context);
    ok = glXMakeCurrent(g_display, g_root_window, g_gl_context);
    g_writeln("ok %d", ok);
    g_pixconfigs = glXChooseFBConfig(g_display, g_screen_num,
                                     g_pixconfig_attrs, &g_n_pixconfigs);
    g_writeln("g_pixconfigs %p g_n_pixconfigs %d",
              g_pixconfigs, g_n_pixconfigs);

    return 0;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_create_image(Pixmap pixmap, inf_image_t *inf_image)
{
    *inf_image = glXCreatePixmap(g_display, g_pixconfigs[0],
                                 pixmap, g_pixmap_attribs);
    return 0;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_destroy_image(inf_image_t inf_image)
{
    glXDestroyPixmap(g_display, inf_image);
    return 0;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_bind_tex_image(inf_image_t inf_image)
{
    glXBindTexImageEXT(g_display, inf_image, GLX_FRONT_EXT, NULL);
    return 0;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_release_tex_image(inf_image_t inf_image)
{
    glXReleaseTexImageEXT(g_display, inf_image, GLX_FRONT_EXT);
    return 0;
}

#elif defined(XRDP_EGL)

#include <epoxy/egl.h>

static EGLDisplay g_egl_display;
static EGLContext g_egl_context;
static EGLSurface g_egl_surface;
static EGLConfig g_ecfg;
static EGLint g_num_config;

static EGLint g_choose_config_attr[] =
{
    EGL_COLOR_BUFFER_TYPE,     EGL_RGB_BUFFER,
    EGL_BUFFER_SIZE,           32,
    EGL_RED_SIZE,              8,
    EGL_GREEN_SIZE,            8,
    EGL_BLUE_SIZE,             8,
    EGL_ALPHA_SIZE,            8,

    EGL_DEPTH_SIZE,            24,
    EGL_STENCIL_SIZE,          8,

    EGL_SAMPLE_BUFFERS,        0,
    EGL_SAMPLES,               0,

    EGL_SURFACE_TYPE,          EGL_WINDOW_BIT | EGL_PIXMAP_BIT,
    EGL_RENDERABLE_TYPE,       EGL_OPENGL_BIT,

    //EGL_BIND_TO_TEXTURE_RGB,   EGL_TRUE,
    //EGL_Y_INVERTED_NOK, EGL_TRUE,

    EGL_NONE,

    //EGL_RED_SIZE,        8,
    //EGL_GREEN_SIZE,      8,
    //EGL_BLUE_SIZE,       8,
    //EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
    //EGL_RENDERABLE_TYPE, 0,
    //EGL_BIND_TO_TEXTURE_RGB, EGL_TRUE,
    //EGL_NONE
    //EGL_RED_SIZE,           8,
    //EGL_GREEN_SIZE,         8,
    //EGL_BLUE_SIZE,          8,
    //EGL_ALPHA_SIZE,         0,
    //EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
    //EGL_CONFIG_CAVEAT,      EGL_NONE,
    //EGL_MATCH_NATIVE_PIXMAP, 1,
    //EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE,
    //EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PIXMAP_BIT,
    //EGL_NONE
};
static EGLint g_create_context_attr[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

static const EGLint g_create_surface_attr[] =
{
    //EGL_Y_INVERTED_NOK, EGL_TRUE,
    EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE,
    //EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
    //EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
    //EGL_MIPMAP_TEXTURE, EGL_TRUE,
    //EGL_TEXTURE_TARGET
    EGL_NONE
};

typedef EGLSurface inf_image_t;

/*****************************************************************************/
static int
xorgxrdp_helper_inf_init(void)
{
    int ok;

    g_egl_display = eglGetDisplay((EGLNativeDisplayType) g_display);
    eglInitialize(g_egl_display, NULL, NULL);
    eglChooseConfig(g_egl_display, g_choose_config_attr, &g_ecfg,
                    1, &g_num_config);
    g_writeln("g_ecfg %p g_num_config %d", g_ecfg, g_num_config);
    g_egl_surface = eglCreateWindowSurface(g_egl_display, g_ecfg,
                                           g_root_window, NULL);
    g_writeln("g_egl_surface %p", g_egl_surface);
    eglBindAPI(EGL_OPENGL_API);
    g_egl_context = eglCreateContext(g_egl_display, g_ecfg,
                                     EGL_NO_CONTEXT, g_create_context_attr);
    g_writeln("g_egl_context %p", g_egl_context);
    ok = eglMakeCurrent(g_egl_display, g_egl_surface, g_egl_surface,
                       g_egl_context);
    g_writeln("ok %d", ok);

    return 0;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_create_image(Pixmap pixmap, inf_image_t *inf_image)
{
    *inf_image = eglCreatePixmapSurface(g_egl_display, g_ecfg,
                                        pixmap, g_create_surface_attr);
    return 0;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_destroy_image(inf_image_t inf_image)
{
    eglDestroySurface(g_egl_display, inf_image);
    return 0;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_bind_tex_image(inf_image_t inf_image)
{
    eglBindTexImage(g_egl_display, inf_image, EGL_BACK_BUFFER);
    return 0;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_release_tex_image(inf_image_t inf_image)
{
    eglReleaseTexImage(g_egl_display, inf_image, EGL_BACK_BUFFER);
    return 0;
}

#else

typedef void * inf_image_t;

/*****************************************************************************/
static int
xorgxrdp_helper_inf_init(void)
{
    return 1;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_create_image(Pixmap pixmap, inf_image_t *inf_image)
{
    return 1;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_destroy_image(inf_image_t inf_image)
{
    return 1;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_bind_tex_image(inf_image_t inf_image)
{
    return 1;
}

/*****************************************************************************/
static int
xorgxrdp_helper_inf_release_tex_image(inf_image_t inf_image)
{
    return 1;
}

#endif

struct mon_info
{
    int width;
    int height;
    Pixmap pixmap;
    inf_image_t inf_image;
    GLuint bmp_texture;
    GLuint enc_texture;
    int tex_format;
    GLfloat * (*get_vertices)(GLuint *vertices_bytes,
                              GLuint *vertices_pointes,
                              int num_crects, struct xh_rect *crects,
                              int width, int height);
    struct xh_rect viewport;
    struct enc_info *ei;
};

static struct mon_info g_mons[16];

static GLuint g_quad_vao = 0;
static GLuint g_fb = 0;

#define XH_SHADERCOPY       0
#define XH_SHADERRGB2YUV420 1
#define XH_SHADERRGB2YUV444 2

#define XH_NUM_SHADERS 3

struct shader_info
{
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLint tex_loc;
    GLint tex_size_loc;
    GLint ymath_loc;
    GLint umath_loc;
    GLint vmath_loc;
    int current_matrix;
};
static struct shader_info g_si[XH_NUM_SHADERS];

static const GLfloat g_vertices[] =
{
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f, -1.0f
};

struct rgb2yuv_matrix
{
    GLfloat ymath[4];
    GLfloat umath[4];
    GLfloat vmath[4];
};

static struct rgb2yuv_matrix g_rgb2yux_matrix[3] =
{
    {
        /* yuv bt601 lagecy */
        {  66.0/256.0,  129.0/256.0,  25.0/256.0,  16.0/256.0 },
        { -38.0/256.0,  -74.0/256.0, 112.0/256.0, 128.0/256.0 },
        { 112.0/256.0,  -94.0/256.0, -18.0/256.0, 128.0/256.0 }
    },
    {
        /* yuv bt709 full range, used in gfx h264 */
        {  54.0/256.0,  183.0/256.0,  18.0/256.0,   0.0/256.0 },
        { -29.0/256.0,  -99.0/256.0, 128.0/256.0, 128.0/256.0 },
        { 128.0/256.0, -116.0/256.0, -12.0/256.0, 128.0/256.0 }
    },
    {
        /* yuv remotefx and gfx progressive remotefx */
        {    0.299000,     0.587000,    0.114000,         0.0 },
        {   -0.168935,    -0.331665,    0.500590,         0.5 },
        {    0.499830,    -0.418531,   -0.081282,         0.5 }
    }
};

static const GLchar g_vs[] =
"\
attribute vec4 position;\n\
void main(void)\n\
{\n\
    gl_Position = vec4(position.xy, 0.0, 1.0);\n\
}\n";
static const GLchar g_fs_copy[] =
"\
uniform sampler2D tex;\n\
uniform vec2 tex_size;\n\
void main(void)\n\
{\n\
    gl_FragColor = texture2D(tex, gl_FragCoord.xy / tex_size);\n\
}\n";
static const GLchar g_fs_rgb_to_yuv420[] =
"\
uniform sampler2D tex;\n\
uniform vec2 tex_size;\n\
uniform vec4 ymath;\n\
uniform vec4 umath;\n\
uniform vec4 vmath;\n\
void main(void)\n\
{\n\
    vec4 pixel;\n\
    float f1;\n\
    float x;\n\
    float y;\n\
    if (gl_FragCoord.y < tex_size.y)\n\
    {\n\
        pixel = texture2D(tex, gl_FragCoord.xy / tex_size);\n\
        pixel = ymath * pixel;\n\
        f1 = clamp(pixel.r + pixel.g + pixel.b + ymath.a, 0.0, 1.0);\n\
        gl_FragColor = f1;\n\
    }\n\
    else\n\
    {\n\
        x = gl_FragCoord.x;\n\
        y = (gl_FragCoord.y - tex_size.y) * 2.0;\n\
        if (mod(gl_FragCoord.x, 2.0) < 1.0)\n\
        {\n\
            pixel = texture2D(tex, vec2(x, y) / tex_size);\n\
            pixel = umath * pixel;\n\
            f1 = clamp(pixel.r + pixel.g + pixel.b + umath.a, 0.0, 1.0);\n\
            pixel = texture2D(tex, vec2(x + 1.0, y) / tex_size);\n\
            pixel = umath * pixel;\n\
            f1 += clamp(pixel.r + pixel.g + pixel.b + umath.a, 0.0, 1.0);\n\
            pixel = texture2D(tex, vec2(x, y + 1.0) / tex_size);\n\
            pixel = umath * pixel;\n\
            f1 += clamp(pixel.r + pixel.g + pixel.b + umath.a, 0.0, 1.0);\n\
            pixel = texture2D(tex, vec2(x + 1.0, y + 1.0) / tex_size);\n\
            pixel = umath * pixel;\n\
            f1 += clamp(pixel.r + pixel.g + pixel.b + umath.a, 0.0, 1.0);\n\
            gl_FragColor = f1 / 4.0;\n\
        }\n\
        else\n\
        {\n\
            pixel = texture2D(tex, vec2(x, y) / tex_size);\n\
            pixel = vmath * pixel;\n\
            f1 = clamp(pixel.r + pixel.g + pixel.b + vmath.a, 0.0, 1.0);\n\
            pixel = texture2D(tex, vec2(x - 1.0, y) / tex_size);\n\
            pixel = vmath * pixel;\n\
            f1 += clamp(pixel.r + pixel.g + pixel.b + vmath.a, 0.0, 1.0);\n\
            pixel = texture2D(tex, vec2(x, y + 1.0) / tex_size);\n\
            pixel = vmath * pixel;\n\
            f1 += clamp(pixel.r + pixel.g + pixel.b + vmath.a, 0.0, 1.0);\n\
            pixel = texture2D(tex, vec2(x - 1.0, y + 1.0) / tex_size);\n\
            pixel = vmath * pixel;\n\
            f1 += clamp(pixel.r + pixel.g + pixel.b + vmath.a, 0.0, 1.0);\n\
            gl_FragColor = f1 / 4.0;\n\
        }\n\
    }\n\
}\n";
static const GLchar g_fs_rgb_to_yuv444[] =
"\
uniform sampler2D tex;\n\
uniform vec2 tex_size;\n\
uniform vec4 ymath;\n\
uniform vec4 umath;\n\
uniform vec4 vmath;\n\
void main(void)\n\
{\n\
    vec4 pixel;\n\
    vec4 pixel1;\n\
    vec4 pixel2;\n\
    vec4 pixel3;\n\
    pixel = texture2D(tex, gl_FragCoord.xy / tex_size);\n\
    pixel1 = ymath * pixel;\n\
    pixel2 = umath * pixel;\n\
    pixel3 = vmath * pixel;\n\
    pixel1 = vec4(pixel3.r + pixel3.g + pixel3.b + vmath.a,\n\
                  pixel2.r + pixel2.g + pixel2.b + umath.a,\n\
                  pixel1.r + pixel1.g + pixel1.b + ymath.a,\n\
                  1.0);\n\
    pixel2 = clamp(pixel1, 0.0, 1.0);\n\
    gl_FragColor = pixel2;\n\
}\n";

/*****************************************************************************/
int
xorgxrdp_helper_x11_init(void)
{
    const GLchar *vsource[XH_NUM_SHADERS];
    const GLchar *fsource[XH_NUM_SHADERS];
    GLint linked;
    GLint compiled;
    GLint vlength;
    GLint flength;
    GLuint quad_vbo;
    int index;
    int gl_ver;

    /* x11 */
    g_display = XOpenDisplay(0);
    if (g_display == NULL)
    {
        return 1;
    }
    g_x_socket = XConnectionNumber(g_display);
    g_screen_num = DefaultScreen(g_display);
    g_screen = ScreenOfDisplay(g_display, g_screen_num);
    g_root_window = RootWindowOfScreen(g_screen);
    g_vis = XDefaultVisual(g_display, g_screen_num);
    g_gc = DefaultGC(g_display, 0);
    if (xorgxrdp_helper_inf_init() != 0)
    {
        return 1;
    }
    gl_ver = epoxy_gl_version();
    g_writeln("gl_ver %d", gl_ver);
    if (gl_ver < 30)
    {
        g_writeln("gl_ver too old %d", gl_ver);
        return 1;
    }

    g_writeln("vendor: %s", (const char *) glGetString(GL_VENDOR));
    g_writeln("version: %s", (const char *) glGetString(GL_VERSION));

    /* create vertex array */
    glGenVertexArrays(1, &g_quad_vao);
    glBindVertexArray(g_quad_vao);
    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertices), g_vertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, NULL);
    glGenFramebuffers(1, &g_fb);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &quad_vbo);

    /* create copy shader */
    vsource[XH_SHADERCOPY] = g_vs;
    fsource[XH_SHADERCOPY] = g_fs_copy;
    /* create rgb2yuv shader */
    vsource[XH_SHADERRGB2YUV420] = g_vs;
    fsource[XH_SHADERRGB2YUV420] = g_fs_rgb_to_yuv420;
    /* create rgb2yuv shader */
    vsource[XH_SHADERRGB2YUV444] = g_vs;
    fsource[XH_SHADERRGB2YUV444] = g_fs_rgb_to_yuv444;

    for (index = 0; index < XH_NUM_SHADERS; index++)
    {
        g_si[index].vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        g_si[index].fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        vlength = g_strlen(vsource[index]);
        flength = g_strlen(fsource[index]);
        glShaderSource(g_si[index].vertex_shader, 1,
                       &(vsource[index]), &vlength);
        glShaderSource(g_si[index].fragment_shader, 1,
                       &(fsource[index]), &flength);
        glCompileShader(g_si[index].vertex_shader);
        glGetShaderiv(g_si[index].vertex_shader, GL_COMPILE_STATUS,
                      &compiled);
        g_writeln("xorgxrdp_helper_x11_init: vertex_shader compiled %d",
                  compiled);
        glCompileShader(g_si[index].fragment_shader);
        glGetShaderiv(g_si[index].fragment_shader, GL_COMPILE_STATUS,
                      &compiled);
        g_writeln("xorgxrdp_helper_x11_init: fragment_shader compiled %d",
                  compiled);
        g_si[index].program = glCreateProgram();
        glAttachShader(g_si[index].program, g_si[index].vertex_shader);
        glAttachShader(g_si[index].program, g_si[index].fragment_shader);
        glLinkProgram(g_si[index].program);
        glGetProgramiv(g_si[index].program, GL_LINK_STATUS, &linked);
        g_writeln("xorgxrdp_helper_x11_init: linked %d", linked);
        g_si[index].tex_loc =
            glGetUniformLocation(g_si[index].program, "tex");
        g_si[index].tex_size_loc =
            glGetUniformLocation(g_si[index].program, "tex_size");
        g_si[index].ymath_loc =
            glGetUniformLocation(g_si[index].program, "ymath");
        g_si[index].umath_loc =
            glGetUniformLocation(g_si[index].program, "umath");
        g_si[index].vmath_loc =
            glGetUniformLocation(g_si[index].program, "vmath");
        g_writeln("xorgxrdp_helper_x11_init: tex_loc %d tex_size_loc %d "
                  "ymath_loc %d umath_loc %d vmath_loc %d",
                  g_si[index].tex_loc, g_si[index].tex_size_loc,
                  g_si[index].ymath_loc, g_si[index].umath_loc,
                  g_si[index].vmath_loc);
        if ((g_si[index].ymath_loc >= 0) &&
            (g_si[index].umath_loc >= 0) &&
            (g_si[index].vmath_loc >= 0))
        {
            /* set default matrix */
            glUseProgram(g_si[index].program);
            glUniform4fv(g_si[index].ymath_loc, 1, g_rgb2yux_matrix[0].ymath);
            glUniform4fv(g_si[index].umath_loc, 1, g_rgb2yux_matrix[0].umath);
            glUniform4fv(g_si[index].vmath_loc, 1, g_rgb2yux_matrix[0].vmath);
        }
    }
    g_memset(g_mons, 0, sizeof(g_mons));
    xorgxrdp_helper_encoder_init();
    return 0;
}

/*****************************************************************************/
int
xorgxrdp_helper_x11_get_wait_objs(intptr_t *objs, int *obj_count)
{
    objs[*obj_count] = g_x_socket;
    (*obj_count)++;
    return 0;
}

/*****************************************************************************/
int
xorgxrdp_helper_x11_check_wai_objs(void)
{
    XEvent xevent;

    while (XPending(g_display) > 0)
    {
        g_writeln("xorgxrdp_helper_x11_check_wai_objs: loop");
        XNextEvent(g_display, &xevent);
    }
    return 0;
}

/*****************************************************************************/
int
xorgxrdp_helper_x11_delete_all_pixmaps(void)
{
    int index;
    struct mon_info *mi;

    for (index = 0; index < 16; index++)
    {
        mi = g_mons + index;
        if (mi->pixmap != 0)
        {
            xorgxrdp_helper_encoder_delete_encoder(mi->ei);
            glDeleteTextures(1, &(mi->bmp_texture));
            glDeleteTextures(1, &(mi->enc_texture));
            xorgxrdp_helper_inf_destroy_image(mi->inf_image);
            XFreePixmap(g_display, mi->pixmap);
            mi->pixmap = 0;
        }
    }
    return 0;
}

/*****************************************************************************/
static GLfloat *
get_vertices_all(GLuint *vertices_bytes, GLuint *vertices_pointes,
                 int num_crects, struct xh_rect *crects,
                 int width, int height)
{
    GLfloat *vertices;

    if (num_crects < 1)
    {
        return NULL;
    }
    vertices = g_new(GLfloat, 12);
    if (vertices == NULL)
    {
        return NULL;
    }
    vertices[0]  = -1;
    vertices[1]  =  1;
    vertices[2]  = -1;
    vertices[3]  = -1;
    vertices[4]  =  1;
    vertices[5]  =  1;
    vertices[6]  = -1;
    vertices[7]  = -1;
    vertices[8]  =  1;
    vertices[9]  =  1;
    vertices[10] =  1;
    vertices[11] = -1;
    *vertices_bytes = sizeof(GLfloat) * 12;
    *vertices_pointes = 6;
    return vertices;
}

/*****************************************************************************/
static GLfloat *
get_vertices420(GLuint *vertices_bytes, GLuint *vertices_pointes,
                int num_crects, struct xh_rect *crects,
                int width, int height)
{
    GLfloat *vertices;
    GLfloat *vert;
    GLfloat x1;
    GLfloat x2;
    GLfloat y1;
    GLfloat y2;
    int index;
    GLfloat fwidth;
    GLfloat fheight;
    const GLfloat fac13 = 1.0 / 3.0;
    const GLfloat fac23 = 2.0 / 3.0;
    const GLfloat fac43 = 4.0 / 3.0;
    struct xh_rect *crect;

    if (num_crects < 1)
    {
        return get_vertices_all(vertices_bytes, vertices_pointes,
                                num_crects, crects, width, height);
    }
    vertices = g_new(GLfloat, num_crects * 24);
    if (vertices == NULL)
    {
        return NULL;
    }
    fwidth = width  / 2.0;
    fheight = height / 2.0;
    for (index = 0; index < num_crects; index++)
    {
        crect = crects + index;
        x1 = crect->x / fwidth;
        y1 = crect->y / fheight;
        x2 = (crect->x + crect->w) / fwidth;
        y2 = (crect->y + crect->h) / fheight;
        vert = vertices + index * 24;
        /* y box */
        vert[0]  =  x1 - 1.0;
        vert[1]  =  y1 * fac23 - 1.0;
        vert[2]  =  x1 - 1.0;
        vert[3]  =  y2 * fac23 - 1.0;
        vert[4]  =  x2 - 1.0;
        vert[5]  =  y1 * fac23 - 1.0;
        vert[6]  =  x1 - 1.0;
        vert[7]  =  y2 * fac23 - 1.0;
        vert[8]  =  x2 - 1.0;
        vert[9]  =  y1 * fac23 - 1.0;
        vert[10] =  x2 - 1.0;
        vert[11] =  y2 * fac23 - 1.0;
        /* uv box */
        vert[12] =  x1 - 1.0;
        vert[13] = (y1 * fac13 + fac43) - 1.0;
        vert[14] =  x1 - 1.0;
        vert[15] = (y2 * fac13 + fac43) - 1.0;
        vert[16] =  x2 - 1.0;
        vert[17] = (y1 * fac13 + fac43) - 1.0;
        vert[18] =  x1  - 1.0;
        vert[19] = (y2 * fac13 + fac43) - 1.0;
        vert[20] =  x2 - 1.0;
        vert[21] = (y1 * fac13 + fac43) - 1.0;
        vert[22] =  x2 - 1.0;
        vert[23] = (y2 * fac13 + fac43) - 1.0;
    }
    *vertices_bytes = sizeof(GLfloat) * num_crects * 24;
    *vertices_pointes = num_crects * 12;
    return vertices;
}

/*****************************************************************************/
static GLfloat *
get_vertices444(GLuint *vertices_bytes, GLuint *vertices_pointes,
                int num_crects, struct xh_rect *crects,
                int width, int height)
{
    GLfloat *vertices;
    GLfloat *vert;
    GLfloat x1;
    GLfloat x2;
    GLfloat y1;
    GLfloat y2;
    int index;
    GLfloat fwidth;
    GLfloat fheight;
    struct xh_rect *crect;

    if (num_crects < 1)
    {
        return get_vertices_all(vertices_bytes, vertices_pointes,
                                num_crects, crects, width, height);
    }
    vertices = g_new(GLfloat, num_crects * 12);
    if (vertices == NULL)
    {
        return NULL;
    }
    fwidth = width  / 2.0;
    fheight = height / 2.0;
    for (index = 0; index < num_crects; index++)
    {
        crect = crects + index;
        x1 = crect->x / fwidth;
        y1 = crect->y / fheight;
        x2 = (crect->x + crect->w) / fwidth;
        y2 = (crect->y + crect->h) / fheight;
        vert = vertices + index * 12;
        vert[0]  = x1 - 1.0;
        vert[1]  = y1 - 1.0;
        vert[2]  = x1 - 1.0;
        vert[3]  = y2 - 1.0;
        vert[4]  = x2 - 1.0;
        vert[5]  = y1 - 1.0;
        vert[6]  = x1 - 1.0;
        vert[7]  = y2 - 1.0;
        vert[8]  = x2 - 1.0;
        vert[9]  = y1 - 1.0;
        vert[10] = x2 - 1.0;
        vert[11] = y2 - 1.0;
    }
    *vertices_bytes = sizeof(GLfloat) * num_crects * 12;
    *vertices_pointes = num_crects * 6;
    return vertices;
}

/*****************************************************************************/
int
xorgxrdp_helper_x11_create_pixmap(int width, int height, int magic,
                                  int con_id, int mon_id)
{
    struct mon_info *mi;
    Pixmap pixmap;
    XImage *ximage;
    int img[64];
    inf_image_t inf_image;
    GLuint bmp_texture;
    GLuint enc_texture;

    mi = g_mons + (mon_id & 0xF);
    mi->tex_format = XH_YUV420;
    //mi->tex_format = XH_YUV444;
    if (mi->pixmap != 0)
    {
        g_writeln("xorgxrdp_helper_x11_create_pixmap: error already setup");
        return 1;
    }
    g_writeln("xorgxrdp_helper_x11_create_pixmap: width %d height %d, "
              "magic 0x%8.8x, con_id %d mod_id %d", width, height,
              magic, con_id, mon_id);
    pixmap = XCreatePixmap(g_display, g_root_window, width, height, 24);
    g_writeln("xorgxrdp_helper_x11_create_pixmap: pixmap %d", (int) pixmap);

    if (xorgxrdp_helper_inf_create_image(pixmap, &inf_image) != 0)
    {
        return 1;
    }
    g_writeln("xorgxrdp_helper_x11_create_pixmap: inf_image %p",
              (void *) inf_image);

    g_memset(img, 0, sizeof(img));
    img[0] = magic;
    img[1] = con_id;
    img[2] = mon_id;
    ximage = XCreateImage(g_display, g_vis, 24, ZPixmap, 0, (char *) img,
                          4, 4, 32, 0);
    XPutImage(g_display, pixmap, g_gc, ximage, 0, 0, 0, 0, 4, 4);
    XFree(ximage);

    glEnable(GL_TEXTURE_2D);
    /* texture that gets encoded */
    glGenTextures(1, &enc_texture);
    glBindTexture(GL_TEXTURE_2D, enc_texture);
    if (mi->tex_format == XH_YUV420)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height * 3 / 2, 0,
                     GL_RED, GL_UNSIGNED_BYTE, NULL);
        mi->get_vertices = get_vertices420;
        mi->viewport.x = 0;
        mi->viewport.y = 0;
        mi->viewport.w = width;
        mi->viewport.h = height * 3 / 2;
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, NULL);
        mi->get_vertices = get_vertices444;
        mi->viewport.x = 0;
        mi->viewport.y = 0;
        mi->viewport.w = width;
        mi->viewport.h = height;
    }
    /* texture that binds with pixmap */
    glGenTextures(1, &bmp_texture);
    glBindTexture(GL_TEXTURE_2D, bmp_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (xorgxrdp_helper_encoder_create_encoder(width, height,
                                               enc_texture, mi->tex_format,
                                               &(mi->ei)) != 0)
    {
        return 1;
    }

    mi->pixmap = pixmap;
    mi->inf_image = inf_image;
    mi->enc_texture = enc_texture;
    mi->width = width;
    mi->height = height;
    mi->bmp_texture = bmp_texture;

    return 0;
}

/*****************************************************************************/
int
xorgxrdp_helper_x11_encode_pixmap(int width, int height, int mon_id,
                                  int num_crects, struct xh_rect *crects,
                                  void *cdata, int *cdata_bytes)
{
    struct mon_info *mi;
    struct shader_info *si;
    int rv;
    GLuint vao;
    GLuint vbo;
    GLfloat *vertices;
    GLuint vertices_bytes;
    GLuint vertices_pointes;

    mi = g_mons + (mon_id & 0xF);
    if ((width != mi->width) || (height != mi->height))
    {
        return 1;
    }
    /* rgb to yuv */
    si = g_si + mi->tex_format % XH_NUM_SHADERS;
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mi->bmp_texture);
    xorgxrdp_helper_inf_bind_tex_image(mi->inf_image);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, mi->enc_texture, 0);
    glUseProgram(si->program);
    /* setup vertices from crects */
    vertices = mi->get_vertices(&vertices_bytes, &vertices_pointes,
                                num_crects, crects, width, height);
    if (vertices == NULL)
    {
        return 1;
    }
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices_bytes, vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, NULL);
    /* uniforms */
    glUniform2f(si->tex_size_loc, mi->width, mi->height);
    /* viewport and draw */
    glViewport(mi->viewport.x, mi->viewport.y, mi->viewport.w, mi->viewport.h);
    glDrawArrays(GL_TRIANGLES, 0, vertices_pointes);
    /* cleanup */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    g_free(vertices);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    xorgxrdp_helper_inf_release_tex_image(mi->inf_image);
    glBindTexture(GL_TEXTURE_2D, 0);
    /* sync before encoding */
    glFinish();
    /* encode */
    rv = xorgxrdp_helper_encoder_encode(mi->ei, mi->enc_texture,
                                        cdata, cdata_bytes);
    return rv;
}

