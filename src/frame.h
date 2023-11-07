#ifndef FRAME_H
#define FRAME_H

// win32
//   msvc : user32.lib gdi32.lib opengl32.lib (shell32.lib)
//   mingw:

#ifndef FRAME_LOG
#  ifndef FRAME_QUIET
#    include <stdio.h>
#    define FRAME_LOG(...) fprintf(stderr, "FRAME: "__VA_ARGS__);
#  else
#    define FRAME_LOG(...)
#  endif // FRAME QUIET
#endif // FRAME_LOG

#include <stdbool.h>
#include <math.h>

#include <windows.h>
#include <GL/GL.h>

#ifndef FRAME_DEF
#  define FRAME_DEF static inline
#endif //FRAME_DEF

#ifndef PI
#  define PI 3.141592653589793f
#endif //PI

#define FRAME_DOUBLE_CLICK_TIME_MS 500

#define FRAME_BACKSPACE 8
#define FRAME_ESCAPE 27
#define FRAME_SPACE 32

#define FRAME_ARROW_LEFT 37
#define FRAME_ARROW_UP 38
#define FRAME_ARROW_RIGHT 39
#define FRAME_ARROW_DOWN 40

typedef enum{
  FRAME_EVENT_NONE = 0,
  FRAME_EVENT_KEYPRESS,
  FRAME_EVENT_KEYRELEASE,
  FRAME_EVENT_MOUSEPRESS,
  FRAME_EVENT_MOUSERELEASE,
  FRAME_EVENT_MOUSEWHEEL,
  FRAME_EVENT_FILEDROP,
}Frame_Event_Type;

typedef struct{
  MSG msg;
  Frame_Event_Type type;
  union{
    char key;
    long long value;
    int amount;
  }as;
}Frame_Event;

typedef struct{
  HDROP h_drop;
  char path[MAX_PATH];

  int count;
  int index;
}Frame_Dragged_Files;

typedef struct{
  HWND hwnd;
  HDC dc;
  RECT rect;
  POINT point;
  LARGE_INTEGER performance_frequency;
  LARGE_INTEGER time;
  bool is_shift_down;
  
  double dt;
  int running;
  int width, height;
}Frame;

typedef struct{
  HANDLE handle;
}Frame_Clipboard;

#define FRAME_RUNNING       0x1
#define FRAME_NOT_RESIZABLE 0x2
#define FRAME_DRAG_N_DROP   0x4
#define FRAME_FULLSCREEN    0x8

FRAME_DEF bool frame_init(Frame *w, int width, int height, const char *title, int flags);
FRAME_DEF bool frame_set_vsync(Frame *w, bool use_vsync);
FRAME_DEF bool frame_peek(Frame *w, Frame_Event *event);
FRAME_DEF bool frame_get_mouse_position(Frame *w, float *x, float *y);
FRAME_DEF void frame_swap_buffers(Frame *w);
FRAME_DEF bool frame_toggle_fullscreen(Frame *w);
FRAME_DEF void frame_free(Frame *w);
FRAME_DEF bool frame_show_cursor(Frame *w, bool show);

FRAME_DEF bool frame_dragged_files_init(Frame_Dragged_Files *files, Frame_Event *event);
FRAME_DEF bool frame_dragged_files_next(Frame_Dragged_Files *files, char **path);
FRAME_DEF void frame_dragged_files_free(Frame_Dragged_Files *files);

FRAME_DEF bool frame_clipboard_init(Frame_Clipboard *clipboard, Frame *w, char **text);
FRAME_DEF bool frame_clipboard_set(Frame *w, const char *text, size_t text_len);
FRAME_DEF void frame_clipboard_free(Frame_Clipboard *clipboard);

FRAME_DEF bool frame_compile_shader(GLuint *shader, GLenum shader_type, const char *shader_source);
FRAME_DEF bool frame_link_program(GLuint *program, GLuint vertex_shader, GLuint fragment_shader);

#ifndef FRAME_NO_RENDERER

typedef struct{
  float x, y;
}Frame_Renderer_Vec2f;

FRAME_DEF Frame_Renderer_Vec2f frame_renderer_vec2f(float x, float y);

typedef struct{
  float x, y, z, w;
}Frame_Renderer_Vec4f;

FRAME_DEF Frame_Renderer_Vec4f frame_renderer_vec4f(float x, float y, float z, float w);

typedef struct{
  Frame_Renderer_Vec2f position;
  Frame_Renderer_Vec4f color;
  Frame_Renderer_Vec2f uv;
}Frame_Renderer_Vertex;

#define FRAME_RENDERER_VERTEX_ATTR_POSITION 0
#define FRAME_RENDERER_VERTEX_ATTR_COLOR 1
#define FRAME_RENDERER_VERTEX_ATTR_UV 2

#define FRAME_RENDERER_CAP (1024 * 4)

typedef struct{
  GLuint vao, vbo;
  GLuint vertex_shader, fragment_shader;
  GLuint program;
  
  GLuint textures;
  unsigned int images_count;

#ifdef FRAME_STB_TRUETYPE
  float font_height;
  stbtt_bakedchar font_cdata[96]; // ASCII 32..126 is 95 glyphs
#endif //FRAME_STB_TRUETYPE
    
  int font_index;
  int tex_index;

  float width, height;
  Frame_Renderer_Vec4f background;

  Frame_Renderer_Vertex verticies[FRAME_RENDERER_CAP];
  int verticies_count;

  //Imgui things
  Frame_Renderer_Vec2f input;
  Frame_Renderer_Vec2f pos;
  bool clicked;
  bool released;
}Frame_Renderer;

static Frame_Renderer_Vec4f WHITE = {1, 1, 1, 1};
static Frame_Renderer_Vec4f RED   = {1, 0, 0, 1};
static Frame_Renderer_Vec4f BLUE  = {0, 0, 1, 1};
static Frame_Renderer_Vec4f GREEN = {0, 1, 0, 1};
static Frame_Renderer_Vec4f BLACK = {0, 0, 0, 1};

#define vec2f(x, y) frame_renderer_vec2f((x), (y))
#define vec4f(x, y, z, w) frame_renderer_vec4f((x), (y), (z), (w))
#define Vec4f Frame_Renderer_Vec4f
#define Vec2f Frame_Renderer_Vec2f

#define draw_triangle frame_renderer_triangle
#define draw_solid_triangle frame_renderer_solid_triangle
#define draw_solid_rect frame_renderer_solid_rect
#define draw_solid_rounded_rect frame_renderer_solid_rounded_rect
#define draw_solid_rounded_shaded_rect frame_renderer_solid_rounded_shaded_rect
#define draw_solid_rect_angle frame_renderer_solid_rect_angle
#define push_texture frame_renderer_push_texture
#define draw_texture frame_renderer_texture
#define draw_texture_colored frame_renderer_texture_colored
#define draw_solid_circle frame_renderer_solid_circle

#define button frame_renderer_button
#define texture_button frame_renderer_texture_button
#define texture_button_ex frame_renderer_texture_button_ex

#ifdef FRAME_STB_TRUETYPE
#  define push_font frame_renderer_push_font
#  define push_font_memory frame_renderer_push_font_memory
#  define draw_text(cstr, pos, factor) frame_renderer_text((cstr), strlen((cstr)), (pos), (factor), (WHITE))
#  define draw_text_colored(cstr, pos, factor, color) frame_renderer_text((cstr), strlen((cstr)), (pos), (factor), (color))
#  define draw_text_len(cstr, cstr_len, pos, factor) frame_renderer_text((cstr), (cstr_len), (pos), (factor), (WHITE))
#  define draw_text_len_colored(cstr, cstr_len, pos, factor, color) frame_renderer_text((cstr), (cstr_len), (pos), (factor), (color))

#  define measure_text(cstr, factor, size) frame_renderer_measure_text((cstr), strlen((cstr)), (factor), (size));
#  define measure_text_len(cstr, cstr_len, factor, size) frame_renderer_measure_text((cstr), (cstr_len), (factor), (size));

#  define draw_text_wrapped frame_renderer_text_wrapped
#endif //FRAME_STB_TRUETYPE

#ifdef FRAME_STB_IMAGE
#  define push_image frame_renderer_push_image
#  define push_image_memory frame_renderer_push_image_memory
#endif //FRAME_STB_IMAGE

FRAME_DEF bool frame_renderer_init(Frame_Renderer *r);
FRAME_DEF void frame_renderer_free();

FRAME_DEF void frame_renderer_begin(int width, int height);
FRAME_DEF void frame_renderer_set_color(Frame_Renderer_Vec4f color);
FRAME_DEF void frame_renderer_end();

FRAME_DEF void frame_renderer_imgui_begin(Frame *w, Frame_Event *e);
FRAME_DEF void frame_renderer_imgui_update(Frame *w, Frame_Event *e);
FRAME_DEF void frame_renderer_imgui_end();

// Primitives

FRAME_DEF void frame_renderer_vertex(Frame_Renderer_Vec2f p, Frame_Renderer_Vec4f c, Frame_Renderer_Vec2f uv);
FRAME_DEF void frame_renderer_triangle(Frame_Renderer_Vec2f p1, Frame_Renderer_Vec2f p2, Frame_Renderer_Vec2f p3, Frame_Renderer_Vec4f c1, Frame_Renderer_Vec4f c2, Frame_Renderer_Vec4f c3, Frame_Renderer_Vec2f uv1, Frame_Renderer_Vec2f uv2, Frame_Renderer_Vec2f uv3);
FRAME_DEF void frame_renderer_solid_triangle(Frame_Renderer_Vec2f p1, Frame_Renderer_Vec2f p2, Frame_Renderer_Vec2f p3, Frame_Renderer_Vec4f c);
FRAME_DEF void frame_renderer_quad(Frame_Renderer_Vec2f p1, Frame_Renderer_Vec2f p2, Frame_Renderer_Vec2f p3, Frame_Renderer_Vec2f p4,Frame_Renderer_Vec4f c1,Frame_Renderer_Vec4f c2,Frame_Renderer_Vec4f c3,Frame_Renderer_Vec4f c4, Frame_Renderer_Vec2f uv1, Frame_Renderer_Vec2f uv2, Frame_Renderer_Vec2f uv3, Frame_Renderer_Vec2f uv4);
FRAME_DEF void frame_renderer_solid_rect(Frame_Renderer_Vec2f pos, Frame_Renderer_Vec2f size,Frame_Renderer_Vec4f color);
FRAME_DEF void frame_renderer_solid_rounded_rect(Vec2f pos, Vec2f size, float radius, int parts, Vec4f color);
FRAME_DEF void frame_renderer_solid_rounded_shaded_rect(Frame_Renderer_Vec2f pos, Frame_Renderer_Vec2f size, float radius, int parts, float shade_px, Frame_Renderer_Vec4f color);
FRAME_DEF void frame_renderer_solid_rect_angle(Frame_Renderer_Vec2f pos, Frame_Renderer_Vec2f size, float angle, Frame_Renderer_Vec4f color);
FRAME_DEF bool frame_renderer_create_texture(int width, int height, unsigned int *index);
FRAME_DEF bool frame_renderer_push_to_texture(unsigned int tex, const void *data, int x_off, int y_off, int width, int height);
FRAME_DEF bool frame_renderer_push_texture(int width, int height, const void *data, bool grey, unsigned int *index);
FRAME_DEF void frame_renderer_texture(unsigned int texture, Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec2f uvp, Frame_Renderer_Vec2f uvs);
FRAME_DEF void frame_renderer_texture_colored(unsigned int texture, Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec2f uvp, Frame_Renderer_Vec2f uvs, Frame_Renderer_Vec4f c);
FRAME_DEF void frame_renderer_solid_circle(Frame_Renderer_Vec2f pos, float start_angle, float end_angle, float radius, int parts, Frame_Renderer_Vec4f color);

//Imgui-things
FRAME_DEF bool frame_renderer_button(Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec4f c);
FRAME_DEF bool frame_renderer_texture_button(unsigned int texture, Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s);
FRAME_DEF bool frame_renderer_texture_button_ex(unsigned int texture, Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec4f c, Frame_Renderer_Vec2f uvp, Frame_Renderer_Vec2f uvs);

FRAME_DEF bool frame_renderer_slider(Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec4f knot_color, Frame_Renderer_Vec4f color, float value, float *cursor);

#ifdef FRAME_STB_TRUETYPE
FRAME_DEF bool frame_renderer_push_font(const char *filepath, float pixel_height);
FRAME_DEF bool frame_renderer_push_font_memory(unsigned char *memory, size_t memory_len, float pixel_height);
FRAME_DEF void frame_renderer_measure_text(const char *cstr, size_t cstr_len, float scale, Vec2f *size);
FRAME_DEF void frame_renderer_text(const char *cstr, size_t cstr_len, Frame_Renderer_Vec2f pos, float scale, Frame_Renderer_Vec4f color);
FRAME_DEF void frame_renderer_text_wrapped(const char *cstr, size_t cstr_len, Frame_Renderer_Vec2f *pos, Frame_Renderer_Vec2f size, float scale, Frame_Renderer_Vec4f color);

FRAME_DEF bool frame_renderer_text_button(const char *cstr, size_t cstr_len, float scale, Frame_Renderer_Vec4f text_color, Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec4f c);
#endif //FRAME_STB_TRUETYPE

#ifdef FRAME_STB_IMAGE
FRAME_DEF bool frame_renderer_push_image(const char *filepath, int *width, int *height, unsigned int *index);
FRAME_DEF bool frame_renderer_push_image_memory(unsigned char *data, size_t data_len, int *width, int *height, unsigned int *index);
#endif //FRAME_STB_IMAGE

#endif //FRAME_NO_RENDERER


// opengl - functions / types / definitions
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5

#define GL_ARRAY_BUFFER 0x8892
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_ACTIVE_UNIFORMS 0x8B86

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_RGB_INTEGER 0x8D98
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_MULTISAMPLE  0x809D
#define GL_SAMPLES 0x80A9

#define GL_BGRA 0x80E1
#define GL_RGB 0x1907
#define GL_BGR 0x80E0

#define GL_MULTISAMPLE 0x809D
#define GL_MULTISAMPLE_ARB 0x809D
#define GL_MULTISAMPLE_BIT 0x20000000
#define GL_MULTISAMPLE_BIT_ARB 0x20000000
#define GL_MULTISAMPLE_FILTER_HINT_NV 0x8534
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_SAMPLE_BUFFERS 0x80A8
#define GL_SAMPLES 0x80A9

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

void glActiveTexture(GLenum texture);
void glGenVertexArrays(GLsizei n, GLuint *arrays);
void glBindVertexArray(GLuint array);
void glGenBuffers(GLsizei n, GLuint *buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const void * data, GLenum usage);
void glEnableVertexAttribArray(GLuint index);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer);
GLuint glCreateShader(GLenum shaderType);
void glShaderSource(GLuint shader, GLsizei count, const GLchar **_string, const GLint *length);
void glCompileShader(GLuint shader);
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
GLuint glCreateProgram(void);
void glAttachShader(GLuint program, GLuint shader);
void glLinkProgram(GLuint program);
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void glGetProgramiv(GLuint program, GLenum pname, GLint *params);
void glUseProgram(GLuint program);
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void * data);
void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void glUniform1f(GLint location, GLfloat v0);
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value);
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
void glUniform1i(GLint location, GLint v0);
GLint glGetUniformLocation(GLuint program, const GLchar *name);
void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void glGetUniformfv(GLuint program, GLint location, GLfloat *params);
void glGetUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params);
void glSampleCoverage(GLfloat value, GLboolean invert);
int wglSwapIntervalEXT(GLint interval);

#ifdef FRAME_IMPLEMENTATION

#ifndef FRAME_NO_RENDERER
static Frame_Renderer frame_renderer;
static bool frame_renderer_inited = false;
#endif //FRAME_NO_RENDERER

LRESULT CALLBACK Frame_Implementation_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

  if(message == WM_CLOSE ||
     message == WM_DESTROY) {
    Frame *w = (Frame *) GetWindowLongPtr(hWnd, 0);
    if(w != NULL) {
      w->running = 0;
    }
    PostQuitMessage(0);
    return 0;
  } else {
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  
}

FRAME_DEF void frame_win32_opengl_init();

FRAME_DEF bool frame_init(Frame *w, int width, int height, const char *title, int flags) {

  STARTUPINFO startupInfo;
  GetStartupInfo(&startupInfo);
  HMODULE hInstance = GetModuleHandle(NULL);
  DWORD nCmdShow = startupInfo.wShowWindow;

#if 1
  HINSTANCE user32Lib = LoadLibrary("user32.dll");
  if(!user32Lib) {
    return false;
  }

  PROC setProcessDPIAware = GetProcAddress(user32Lib, "SetProcessDPIAware");
  if(!setProcessDPIAware) {
    return false;
  }
  setProcessDPIAware();

  FreeLibrary(user32Lib);
#else
  SetProcessDPIAware();
#endif

  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = Frame_Implementation_WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = title;
  wc.cbWndExtra = sizeof(LONG_PTR);
  wc.cbClsExtra = 0;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  
  HICON icon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
  wc.hIcon = icon; // ICON when tabbing
  wc.hIconSm = icon; //ICON default
  
  if(!RegisterClassEx(&wc)) {
    return false;
  }

  int screen_width = GetSystemMetrics(SM_CXSCREEN);
  int screen_height = GetSystemMetrics(SM_CYSCREEN);

  // WS_THICKFRAME :: resizable
  // WS_MAXIMIZEBOX :: maximizable

#define FRAME_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define FRAME_RESIZABLE_STYLE (WS_THICKFRAME | WS_MAXIMIZEBOX)

  DWORD style = FRAME_STYLE;
  if(!(flags & FRAME_NOT_RESIZABLE)) {
    style |= FRAME_RESIZABLE_STYLE;
  } else {
    width -= 10;
    height -= 10;
  }

  //add space  
  width += 16;
  height += 39;

  DWORD frame_flags = 0;
  if((flags & FRAME_DRAG_N_DROP)) {
    frame_flags |= WS_EX_ACCEPTFILES;
  }

  w->hwnd = CreateWindowEx(frame_flags,
			   wc.lpszClassName,
			   wc.lpszClassName,
			   style,
			   screen_width / 2 - width/2,
			   screen_height / 2 - height/2,
			   width,
			   height,
			   NULL,
			   NULL,
			   hInstance,
			   NULL);
  if(!w->hwnd) {
    return false;
  }
  w->dc = GetDC(w->hwnd);

  //BEGIN opengl
  HDC w_dc = GetDC(w->hwnd);

  PIXELFORMATDESCRIPTOR desired_format = {0};
  desired_format.nSize = sizeof(desired_format);
  desired_format.nVersion = 1;
  desired_format.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
  desired_format.cColorBits = 32;
  desired_format.cAlphaBits = 8;

  int suggested_format_index = ChoosePixelFormat(w_dc, &desired_format);
  PIXELFORMATDESCRIPTOR suggested_format;
  DescribePixelFormat(w_dc, suggested_format_index, sizeof(suggested_format), &suggested_format);
  SetPixelFormat(w_dc, suggested_format_index, &suggested_format);
  
  HGLRC opengl_rc = wglCreateContext(w_dc);
  if(!wglMakeCurrent(w_dc, opengl_rc)) {
    return false;
  }
  ReleaseDC(w->hwnd, w_dc);
  //END opengl

  LONG_PTR lptr = {0};
  memcpy(&lptr, &w, sizeof(w));  
  SetWindowLongPtr(w->hwnd, 0, lptr);  

  ShowWindow(w->hwnd, nCmdShow);
  UpdateWindow(w->hwnd);

  w->running = FRAME_RUNNING;
  w->width = width;
  w->height = height;
  w->is_shift_down = false;

  // load non-default-opengl-functions
  frame_win32_opengl_init();

#ifndef FRAME_NO_RENDERER
  if(!frame_renderer_inited) {
    if(!frame_renderer_init(&frame_renderer)) {
      return false;	    
    }
	
    frame_renderer_inited = true;
  }
#endif //FRAME_NO_RENDERER

  // use vsync as default
  if(!frame_set_vsync(w, true)) {
    return false;
  }

  if((flags & FRAME_FULLSCREEN) && !frame_toggle_fullscreen(w)) {
    return false;
  }

  QueryPerformanceFrequency(&w->performance_frequency);
  QueryPerformanceCounter(&w->time);
  w->dt = 0;
    
  return true;
}

FRAME_DEF bool frame_set_vsync(Frame *w, bool use_vsync) {
  (void) w;
  return wglSwapIntervalEXT(use_vsync ? 1 : 0);
}

static char frame_german_keyboard[10] = {
  [1] ='!',
  [2] = '\"',
  [3] = '§',
  [4] = '$',
  [5] = '%',
  [6] = '&',
  [7] = '/',
  [8] = '(',
  [9] = ')', 
  [0] = '=',
};

FRAME_DEF bool frame_peek(Frame *w, Frame_Event *e) {
    
  MSG *msg = &e->msg;

  while(true) {
    if(!PeekMessage(msg, w->hwnd, 0, 0, PM_REMOVE)) {
      break;
    }

    TranslateMessage(msg);
    DispatchMessage(msg);

    e->type = FRAME_EVENT_NONE;

    switch(msg->message) {
    case WM_DROPFILES: {
      e->type = FRAME_EVENT_FILEDROP;
      e->as.value = msg->wParam;
    } break;
    case WM_RBUTTONUP:  {
      e->type = FRAME_EVENT_MOUSERELEASE;
      e->as.key = 'r';
      ReleaseCapture();
    } break;
    case WM_RBUTTONDOWN: {
      e->type = FRAME_EVENT_MOUSEPRESS;
      e->as.key = 'r';
      SetCapture(w->hwnd);
    } break;
    case WM_LBUTTONUP: {
      e->type = FRAME_EVENT_MOUSERELEASE;
      e->as.key = 'l';
      ReleaseCapture();
    } break;
    case WM_LBUTTONDOWN: {
      e->type = FRAME_EVENT_MOUSEPRESS;
      e->as.key = 'l';
      SetCapture(w->hwnd);
    } break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      bool was_down = ((msg->lParam & (1 << 30)) != 0);
      bool is_down = ((msg->lParam & (1 << 31)) == 0);

      if(was_down != is_down) {

	if(msg->wParam == VK_SHIFT) {
	  if(was_down) {
	    w->is_shift_down = false;
	  } else {
	    w->is_shift_down = true;
	  }
	  continue;
	}
	
	char c = (char) msg->wParam;
	if(w->is_shift_down) {
	  if('0' <= c && c <= '9') {
	    c = frame_german_keyboard[c - '0'];
	  }
	} else {
	  if('A' <= c && c <= 'Z') {
	    c += 32;
	  }
	}
	
	e->as.key = c;
	if(was_down) {
	  e->type = FRAME_EVENT_KEYRELEASE;
	} else {
	  e->type = FRAME_EVENT_KEYPRESS;
	}
	
      }
      
    } break;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL: {
      e->type = FRAME_EVENT_MOUSEWHEEL; 
      e->as.amount = GET_WHEEL_DELTA_WPARAM(msg->wParam) / 120;//apperantly this is 120`s steps
    } break;
    default: {
    } break;
    }

    
#ifndef FRAME_NO_RENDERER
    frame_renderer_imgui_update(w, e);
#endif //FRAME_NO_RENDERER

    if(e->type != FRAME_EVENT_NONE) {
      return true;
    }
  }

  // width, height
  if(!(w->running & FRAME_FULLSCREEN)) {
    if(GetClientRect(w->hwnd, &w->rect)) {
      w->width = (w->rect.right - w->rect.left);
      w->height = (w->rect.bottom - w->rect.top);
    }
  }

  //dt
  LARGE_INTEGER time;
  QueryPerformanceCounter(&time);
  w->dt = ((double) time.QuadPart - (double) w->time.QuadPart)
    * 1000
    / (double) w->performance_frequency.QuadPart;
  w->time = time;

  //frame_renderer
#ifndef FRAME_NO_RENDERER
  frame_renderer_imgui_begin(w, e);
  frame_renderer_begin(w->width, w->height);
#endif // FRAME_NO_RENDERER

  return false;
}

FRAME_DEF bool frame_get_mouse_position(Frame *w, float *x, float *y) {
  if(GetCursorPos(&w->point) && ScreenToClient(w->hwnd, &w->point)) {
    *x = (float) w->point.x;
    *y = (float) w->height - w->point.y;
    return true;
  }

  return false;
}
  
FRAME_DEF void frame_swap_buffers(Frame *w) {
#ifndef FRAME_NO_RENDERER
  frame_renderer_end();
  frame_renderer_imgui_end();
#endif // FRAME_NO_RENDERER
  
  SwapBuffers(w->dc);
}

FRAME_DEF bool frame_toggle_fullscreen(Frame *w) {

  DWORD style = (DWORD) GetWindowLongPtr(w->hwnd, GWL_STYLE);

  if(w->running & FRAME_FULLSCREEN) {

    style &= ~WS_POPUP;
    style |= FRAME_STYLE; // fix this
    if(!(w->running & FRAME_NOT_RESIZABLE)) {
      style |= FRAME_RESIZABLE_STYLE;
    }
    w->running &= ~FRAME_FULLSCREEN;
    
    SetWindowPos(w->hwnd, NULL,
		 w->rect.left,
		 w->rect.top,
		 w->rect.right - w->rect.left,
		 w->rect.bottom - w->rect.top,
		 SWP_FRAMECHANGED);    
  } else {    
    if(!GetWindowRect(w->hwnd, &w->rect)) {
      return false;
    }

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    style &= ~FRAME_STYLE;
    if(!(w->running & FRAME_NOT_RESIZABLE)) {
      style &= ~FRAME_RESIZABLE_STYLE;
    }
    style |= WS_POPUP;
    w->running |= FRAME_FULLSCREEN;
    w->width = width;
    w->height = height;

    SetWindowPos(w->hwnd, HWND_TOP, 0, 0, width, height, SWP_FRAMECHANGED);
  }

  SetWindowLongPtr(w->hwnd, GWL_STYLE, style);

  return true;
}

FRAME_DEF void frame_free(Frame *w) {	
  ReleaseDC(w->hwnd, w->dc);
  DestroyWindow(w->hwnd);
}

FRAME_DEF bool frame_show_cursor(Frame *w, bool show) {
  (void) w;

  CURSORINFO info;
  info.cbSize = sizeof(CURSORINFO);
  if(!GetCursorInfo(&info)) {
    return false;
  }

  bool visible = info.flags;
  if(show && !visible) {
    ShowCursor(true);
  } else if(!show && visible){
    ShowCursor(false);
  }

  return true;
}

FRAME_DEF bool frame_dragged_files_init(Frame_Dragged_Files *files, Frame_Event *event) {
  files->h_drop = (HDROP) event->as.value;
  files->count = DragQueryFile(files->h_drop, 0xffffffff, files->path, MAX_PATH);
  if(files->count <= 0) {
    return false;
  }
  files->index = 0;

  return true;
}

FRAME_DEF bool frame_dragged_files_next(Frame_Dragged_Files *files, char **path) {
  if(files->index >= files->count) {
    return false;
  }
  DragQueryFile(files->h_drop, files->index++, files->path, MAX_PATH);
  *path = files->path;

  return true;
}

FRAME_DEF void frame_dragged_files_free(Frame_Dragged_Files *files) {
  DragFinish(files->h_drop);
}

FRAME_DEF bool frame_clipboard_init(Frame_Clipboard *clipboard, Frame *w, char **text) {

  (void) w;
  
  if(!OpenClipboard(w->hwnd)) {
    return false;
  }
  
  clipboard->handle = GetClipboardData(CF_TEXT);
  if(clipboard->handle == NULL) {
    CloseClipboard();
    return false;
  }

  *text = GlobalLock(clipboard->handle);
  if((*text) == NULL) {
    CloseClipboard();
    return false;
  }  
  
  return true;
}

FRAME_DEF bool frame_clipboard_set(Frame *w, const char *text, size_t text_len) {

  HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, text_len + 1);
  if(!mem) {
    return false;
  }

  char *locked_mem = GlobalLock(mem);
  if(!locked_mem) {
    GlobalFree(mem);
    return false;
  }

  memcpy(locked_mem, text, text_len);
  locked_mem[text_len] = 0;
  
  GlobalUnlock(locked_mem);

  if(!OpenClipboard(w->hwnd)) {
    GlobalFree(mem);
    return false;
  }

  if(!EmptyClipboard()) {
    GlobalFree(mem);
    CloseClipboard();
    return false;
  }

  if(SetClipboardData(CF_TEXT, mem) == NULL) {
    GlobalFree(mem);
    CloseClipboard();
    return false;
  }

  CloseClipboard();
  
  return true;		
}

FRAME_DEF void frame_clipboard_free(Frame_Clipboard *clipboard) {
  GlobalUnlock(clipboard->handle);
  CloseClipboard();  
}

FRAME_DEF const char *frame_shader_type_name(GLenum shader) {
  switch (shader) {
  case GL_VERTEX_SHADER:
    return "GL_VERTEX_SHADER";
  case GL_FRAGMENT_SHADER:
    return "GL_FRAGMENT_SHADER";
  default:
    return "(Unknown)";
  }
}

FRAME_DEF bool frame_compile_shader(GLuint *shader, GLenum shader_type, const char *shader_source) {
  *shader = glCreateShader(shader_type);
  glShaderSource(*shader, 1, &shader_source, NULL);
  glCompileShader(*shader);

  GLint compiled = 0;
  glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

  if (!compiled) {
    GLchar message[1024];
    GLsizei message_size = 0;
    glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
    FRAME_LOG("Could not compile shader: %s\n", frame_shader_type_name(shader_type));
    FRAME_LOG("%.*s\n", message_size, message);
    return false;
  }

  return true;
}

FRAME_DEF bool frame_link_program(GLuint *program, GLuint vertex_shader, GLuint fragment_shader) {
  *program = glCreateProgram();
  glAttachShader(*program, vertex_shader);
  glAttachShader(*program, fragment_shader);

  glLinkProgram(*program);
  
  GLint linked = 0;
  glGetProgramiv(*program, GL_LINK_STATUS, &linked);
  if(!linked) {
    GLsizei message_size = 0;
    GLchar message[1024];

    glGetProgramInfoLog(*program, sizeof(message), &message_size, message);
    FRAME_LOG("Could not link program: %.*s\n", message_size, message);
    return false;
  }
  
  return true;
    
}

////////////////////////////////////////////////////////////////////////
// renderer - definitions
////////////////////////////////////////////////////////////////////////

#ifndef FRAME_NO_RENDERER

static const char* frame_renderer_vertex_shader_source =
  "#version 330 core\n"
  "\n"
  "layout(location = 0) in vec2 position;\n"
  "layout(location = 1) in vec4 color;\n"
  "layout(location = 2) in vec2 uv;\n"
  "\n"
  "uniform float resolution_x;\n"
  "uniform float resolution_y;\n"
  "\n"
  "out vec4 out_color;\n"
  "out vec2 out_uv;\n"
  "\n"
  "vec2 resolution_project(vec2 point) {\n"
  "    return 2 * point / vec2(resolution_x, resolution_y) - 1;\n"
  "}\n"
  "\n"
  "void main() {\n"
  "  out_color = color;\n"
  "  out_uv = uv;\n"
  "  gl_Position = vec4(resolution_project(position), 0, 1);\n"
  "}";

static const char *frame_renderer_fragment_shader_source=
  "#version 330 core\n"
  "\n"
  "uniform sampler2D font_tex;\n"
  "uniform sampler2D tex;\n"
  "\n"
  "in vec4 out_color;\n"
  "in vec2 out_uv;\n"
  "\n"
  "out vec4 fragColor;\n"
  "\n"
  "void main() {\n"
  "    if(out_uv.x < 0 && out_uv.y < 0) {\n"
  "        fragColor = out_color;\n"
  "    } else if(out_color.w < 0) {\n"
  "        vec4 color = texture(font_tex, vec2(out_uv.x, 1-out_uv.y));\n"
  "        float a = color.w * -out_color.w;\n"
  "        color = (color + vec4(1, 1, 1, 0)) * out_color;\n"
  "        color.w = a;\n"
  "        fragColor = color;\n"
  "    } else {\n"
  "        vec4 color = texture(tex, vec2(out_uv.x, 1-out_uv.y));\n"
  "        color = color * out_color;\n"
  "        fragColor = color;\n"
  "    }\n"
  "}\n";

FRAME_DEF Frame_Renderer_Vec2f frame_renderer_vec2f(float x, float y) {
  return (Frame_Renderer_Vec2f) { x, y};
}

FRAME_DEF Frame_Renderer_Vec4f frame_renderer_vec4f(float x, float y, float z, float w) {
  return (Frame_Renderer_Vec4f) { x, y, z, w};
}

FRAME_DEF bool frame_renderer_init(Frame_Renderer *r) {
  (void) WHITE;
  (void) RED;
  (void) BLUE;
  (void) GREEN;
  (void) BLACK;
    
  // introduce 'verticies' to opengl
  glGenVertexArrays(1, &r->vao);
  glBindVertexArray(r->vao);

  glGenBuffers(1, &r->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(r->verticies), r->verticies, GL_DYNAMIC_DRAW);

  // introduce 'Vertex' to opengl
  glEnableVertexAttribArray(FRAME_RENDERER_VERTEX_ATTR_POSITION);
  glVertexAttribPointer(FRAME_RENDERER_VERTEX_ATTR_POSITION,
			sizeof(Frame_Renderer_Vec2f)/sizeof(float),
			GL_FLOAT,
			GL_FALSE,
			sizeof(Frame_Renderer_Vertex),
			(GLvoid *) offsetof(Frame_Renderer_Vertex, position));

  glEnableVertexAttribArray(FRAME_RENDERER_VERTEX_ATTR_COLOR);
  glVertexAttribPointer(FRAME_RENDERER_VERTEX_ATTR_COLOR,
			sizeof(Frame_Renderer_Vec4f)/sizeof(float),
			GL_FLOAT,
			GL_FALSE,
			sizeof(Frame_Renderer_Vertex),
			(GLvoid *) offsetof(Frame_Renderer_Vertex, color));

  glEnableVertexAttribArray(FRAME_RENDERER_VERTEX_ATTR_UV);
  glVertexAttribPointer(FRAME_RENDERER_VERTEX_ATTR_UV,
			sizeof(Frame_Renderer_Vec2f)/sizeof(float),
			GL_FLOAT,
			GL_FALSE,
			sizeof(Frame_Renderer_Vertex),
			(GLvoid *) offsetof(Frame_Renderer_Vertex, uv));
  
  // compile shaders
  if(!frame_compile_shader(&r->vertex_shader, GL_VERTEX_SHADER, frame_renderer_vertex_shader_source)) {
    return false;
  }
    
  if(!frame_compile_shader(&r->fragment_shader, GL_FRAGMENT_SHADER, frame_renderer_fragment_shader_source)) {
    return false;
  }

  // link program
  if(!frame_link_program(&r->program, r->vertex_shader, r->fragment_shader)) {
    return false;
  }
  glUseProgram(r->program);


  r->images_count = 0;
  r->verticies_count = 0;
  r->font_index = -1;

  frame_renderer_imgui_end();
  frame_renderer.input = vec2f(-1.f, -1.f);
  
  return true;
}

FRAME_DEF void frame_renderer_free(Frame_Renderer *r) {
  (void) r;
}


FRAME_DEF void frame_renderer_begin(int width, int height) {

  Frame_Renderer *r = &frame_renderer;

  if(width > 0 && height > 0) {

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    Frame_Renderer_Vec4f *c = &r->background;
    glClearColor(c->x, c->y, c->z, c->w);

    // tell vertex shader what is the resolution is
    glUniform1fv(glGetUniformLocation(r->program, "resolution_x"), 1, &r->width);
    glUniform1fv(glGetUniformLocation(r->program, "resolution_y"), 1, &r->height);

    r->width = (float) width;
    r->height = (float) height;
  }


  if(r->font_index > 0) {
    GLint uniformLocation1 = glGetUniformLocation(r->program, "font_tex");
    glUniform1i(uniformLocation1, r->font_index);	
  }

  r->tex_index = -1;  
}

FRAME_DEF void frame_renderer_imgui_begin(Frame *w, Frame_Event *e) {

  (void) e;
  float x, y;
  frame_get_mouse_position(w, &x, &y);

  frame_renderer.pos = frame_renderer_vec2f((float) x, ((float) y));
  if(frame_renderer.clicked) {
    frame_renderer.input = frame_renderer.pos;
  }
}

FRAME_DEF void frame_renderer_imgui_update(Frame *w, Frame_Event *e) {
  (void) w;
  if(e->type == FRAME_EVENT_MOUSEPRESS) {    
    frame_renderer.clicked = true;
  } else if(e->type == FRAME_EVENT_MOUSERELEASE) {
    frame_renderer.released = true;
  }  
}

FRAME_DEF void frame_renderer_imgui_end(Frame *w, Frame_Event *e) {
  (void) w;
  (void) e;
  if(frame_renderer.released) {
    frame_renderer.input = vec2f(-1.f, -1.f);
  }
    
  frame_renderer.clicked = false;
  frame_renderer.released = false;
}

FRAME_DEF void frame_renderer_end() {
  Frame_Renderer *r = &frame_renderer;
  
  glBufferSubData(GL_ARRAY_BUFFER, 0, r->verticies_count * sizeof(Frame_Renderer_Vertex), r->verticies);
  glDrawArrays(GL_TRIANGLES, 0, r->verticies_count);
  r->verticies_count = 0;
}

FRAME_DEF void frame_renderer_set_color(Frame_Renderer_Vec4f color) {
  Frame_Renderer *r = &frame_renderer;
  r->background = color;
}

FRAME_DEF void frame_renderer_vertex(Frame_Renderer_Vec2f p, Frame_Renderer_Vec4f c, Frame_Renderer_Vec2f uv) {

  Frame_Renderer *r = &frame_renderer;

  if(r->verticies_count < FRAME_RENDERER_CAP) {
    Frame_Renderer_Vertex *last = &r->verticies[r->verticies_count];
    last->position = p;
    last->color = c;
    last->uv = uv;
    r->verticies_count++;	
  } 
}

FRAME_DEF void frame_renderer_triangle(Frame_Renderer_Vec2f p1, Frame_Renderer_Vec2f p2, Frame_Renderer_Vec2f p3, Frame_Renderer_Vec4f c1, Frame_Renderer_Vec4f c2, Frame_Renderer_Vec4f c3, Frame_Renderer_Vec2f uv1, Frame_Renderer_Vec2f uv2, Frame_Renderer_Vec2f uv3) {

  Frame_Renderer *r = &frame_renderer;

  if(r->verticies_count + 3 >= FRAME_RENDERER_CAP) {
    frame_renderer_end();
  }
    
  frame_renderer_vertex(p1, c1, uv1);
  frame_renderer_vertex(p2, c2, uv2);
  frame_renderer_vertex(p3, c3, uv3);	
}

FRAME_DEF void frame_renderer_solid_triangle(Frame_Renderer_Vec2f p1, Frame_Renderer_Vec2f p2, Frame_Renderer_Vec2f p3, Frame_Renderer_Vec4f c) {

  Frame_Renderer *r = &frame_renderer;

  if(r->verticies_count + 3 >= FRAME_RENDERER_CAP) {
    frame_renderer_end();
  }
	
  Frame_Renderer_Vec2f uv = frame_renderer_vec2f(-1, -1);
  frame_renderer_vertex(p1, c, uv);
  frame_renderer_vertex(p2, c, uv);
  frame_renderer_vertex(p3, c, uv);
}

FRAME_DEF void frame_renderer_quad(Frame_Renderer_Vec2f p1, Frame_Renderer_Vec2f p2, Frame_Renderer_Vec2f p3, Frame_Renderer_Vec2f p4, Frame_Renderer_Vec4f c1, Frame_Renderer_Vec4f c2, Frame_Renderer_Vec4f c3, Frame_Renderer_Vec4f c4, Frame_Renderer_Vec2f uv1, Frame_Renderer_Vec2f uv2, Frame_Renderer_Vec2f uv3, Frame_Renderer_Vec2f uv4) {
  frame_renderer_triangle(p1, p2, p4, c1, c2, c4, uv1, uv2, uv4);
  frame_renderer_triangle(p1, p3, p4, c1, c3, c4, uv1, uv3, uv4);
}

FRAME_DEF void frame_renderer_solid_rect(Frame_Renderer_Vec2f pos, Frame_Renderer_Vec2f size, Frame_Renderer_Vec4f color) {
  Vec2f uv = vec2f(-1, -1);
  frame_renderer_quad(pos,
		       frame_renderer_vec2f(pos.x + size.x, pos.y),
		       frame_renderer_vec2f(pos.x, pos.y + size.y),
		       frame_renderer_vec2f(pos.x + size.x, pos.y + size.y),
		       color, color, color, color, uv, uv, uv, uv);
}

FRAME_DEF void frame_renderer_solid_rounded_rect(Vec2f pos, Vec2f size, float radius, int parts, Vec4f color) {
  if(size.y < 4 * radius) radius = size.y / 4; 

  frame_renderer_solid_rect(vec2f(pos.x, pos.y + radius),
		  vec2f(size.x, size.y - 2 * radius),
		  color);

  frame_renderer_solid_rect(vec2f(pos.x + radius, pos.y),
		  vec2f(size.x - 2 * radius, size.y),
		  color);

  pos = vec2f(pos.x + radius, pos.y + radius);
  size = vec2f(size.x - 2 * radius, size.y - 2 * radius);

  //bottom left
  frame_renderer_solid_circle(pos,
		    PI, PI * 3 / 2,
		    radius,
		    parts,
		    color);

  //bottom right
  frame_renderer_solid_circle(vec2f(pos.x + size.x, pos.y),
		    PI * 3 / 2, 2 * PI,
		    radius,
		    parts,
		    color);

  //top left
  frame_renderer_solid_circle(vec2f(pos.x, pos.y + size.y),
		    PI / 2, PI,
		    radius,
		    parts,
		    color);

  //top right
  frame_renderer_solid_circle(vec2f(pos.x + size.x, pos.y + size.y),
		    0, PI /2,
		    radius,
		    parts,
		    color);
}

FRAME_DEF bool frame_renderer_button_impl(Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec4f *c) {
  Frame_Renderer_Vec2f pos = frame_renderer.input;  
  bool holding =
    p.x <= pos.x &&
    (pos.x - p.x) <= s.x &&
    p.y <= pos.y &&
    (pos.y - p.y) <= s.y;
  if(holding) {
    c->w *= .3f;
  } else {
    pos = frame_renderer.pos;  
    bool hovering =
      p.x <= pos.x &&
      (pos.x - p.x) <= s.x &&
      p.y <= pos.y &&
      (pos.y - p.y) <= s.y;
    if(hovering) {
      c->w *= .6f;
    }
  }
  return holding;
}

FRAME_DEF bool frame_renderer_button(Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec4f c) {
  bool holding = frame_renderer_button_impl(p, s, &c);
  frame_renderer_solid_rect(p, s, c);
  return frame_renderer.released && holding;
}

FRAME_DEF bool frame_renderer_texture_button(unsigned int texture, Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s) {
  Frame_Renderer_Vec4f c = vec4f(1, 1, 1, 1);
  bool holding = frame_renderer_button_impl(p, s, &c);
  frame_renderer_texture_colored(texture, p, s, vec2f(0, 0), vec2f(1, 1), c);
  return frame_renderer.released && holding;
}

FRAME_DEF bool frame_renderer_texture_button_ex(unsigned int texture, Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec4f c, Frame_Renderer_Vec2f uvp, Frame_Renderer_Vec2f uvs) {
  bool holding = frame_renderer_button_impl(p, s, &c);
  frame_renderer_texture_colored(texture, p, s, uvp, uvs, c);
  return frame_renderer.released && holding;
}

FRAME_DEF bool frame_renderer_slider(Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec4f knot_color, Frame_Renderer_Vec4f color, float value, float *cursor) {
  
  Frame_Renderer_Vec2f cursor_pos = vec2f(p.x + s.x * value, p.y + s.y/2);
  float cursor_radius = s.y * 1.125f;
  *cursor = value;

  Frame_Renderer_Vec2f input = frame_renderer.input;
  float dx = input.x - cursor_pos.x;
  float dy = input.y - cursor_pos.y;
  bool knot_clicked = ((dx * dx) + (dy *dy)) <= (cursor_radius * cursor_radius);

  if(knot_clicked) {
    cursor_pos.x = frame_renderer.pos.x;
    if((cursor_pos.x - p.x) < 0) cursor_pos.x = p.x;
    if(s.x < (cursor_pos.x - p.x)) cursor_pos.x = p.x + s.x;
    *cursor = (cursor_pos.x - p.x) / s.x;
  }
  
  frame_renderer_solid_rect(p, vec2f(cursor_pos.x - p.x, s.y), knot_color);
  frame_renderer_solid_rect(vec2f(cursor_pos.x, p.y), vec2f(s.x - cursor_pos.x + p.x, s.y), color);
  frame_renderer_solid_circle(cursor_pos, 0, 2 * PI, cursor_radius, 20, knot_color);

  bool clicked =
    p.x <= input.x &&
    (input.x - p.x) <= s.x&&
    p.y <= input.y &&
    (input.y - p.y) <= s.y;    
  if(frame_renderer.released && knot_clicked) {    
    return true;
  }
  if(frame_renderer.clicked && clicked) {
    *cursor = (frame_renderer.input.x - p.x) / s.x;
    return true;
  }

  return false;
}

FRAME_DEF void frame_renderer_solid_rounded_shaded_rect(Frame_Renderer_Vec2f pos,
							  Frame_Renderer_Vec2f size,
							  float radius,
							  int parts,
							  float shade_px,
							  Frame_Renderer_Vec4f color) {
  frame_renderer_solid_rounded_rect(vec2f(pos.x - shade_px, pos.y - shade_px),
				     vec2f(size.x + 2 *shade_px, size.y + 2 *shade_px),
				     radius, parts, vec4f(0, 0, 0, color.w * .5f));
  frame_renderer_solid_rounded_rect(pos, size, radius, parts, color);
}

FRAME_DEF void frame_renderer_solid_rect_angle(Frame_Renderer_Vec2f pos, Frame_Renderer_Vec2f size,
						 float angle,
						 Frame_Renderer_Vec4f color) {
  Vec2f uv = vec2f(-1, -1);

  float s = sinf(angle);
  float c = cosf(angle);

  float dy1 = s * size.x;
  float dx1 = c * size.x;
  
  float dy2 = c * size.y;
  float dx2 = s * size.y;

  frame_renderer_triangle(pos,
			   frame_renderer_vec2f(pos.x + dx1, pos.y + dy1),
			   frame_renderer_vec2f(pos.x - dx2, pos.y + dy2),
			   color, color, color, uv, uv, uv);

  frame_renderer_triangle(frame_renderer_vec2f(pos.x + dx1, pos.y + dy1),
			   frame_renderer_vec2f(pos.x - dx2, pos.y + dy2),
			   frame_renderer_vec2f(pos.x + dx1 - dx2, pos.y + dy1 + dy2),
			   color, color, color, uv, uv, uv);
}

FRAME_DEF void frame_renderer_texture(unsigned int texture,
					Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s,
					Frame_Renderer_Vec2f uvp, Frame_Renderer_Vec2f uvs) {

  Frame_Renderer *r = &frame_renderer;

  if(r->tex_index != -1) {
    frame_renderer_end();
  }

  r->tex_index = (int) texture;
  GLint uniformLocation1 = glGetUniformLocation(r->program, "tex");
  glUniform1i(uniformLocation1, r->tex_index);
    
  Vec4f c = vec4f(1, 1, 1, 1);
  frame_renderer_quad(p,
		       frame_renderer_vec2f(p.x + s.x, p.y),
		       frame_renderer_vec2f(p.x, p.y + s.y),
		       frame_renderer_vec2f(p.x + s.x, p.y + s.y),
		       c, c, c, c,
		       uvp,
		       frame_renderer_vec2f(uvp.x + uvs.x, uvp.y),
		       frame_renderer_vec2f(uvp.x, uvp.y + uvs.y),
		       frame_renderer_vec2f(uvp.x + uvs.x, uvp.y + uvs.y));
}

FRAME_DEF void frame_renderer_texture_colored(unsigned int texture,
						Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s,
					        Frame_Renderer_Vec2f uvp, Frame_Renderer_Vec2f uvs,
						Frame_Renderer_Vec4f c) {
  Frame_Renderer *r = &frame_renderer;
  
  if(r->tex_index != -1) {
    frame_renderer_end();
  }

  r->tex_index = (int) texture;
  GLint uniformLocation1 = glGetUniformLocation(r->program, "tex");
  glUniform1i(uniformLocation1, r->tex_index);
  
  frame_renderer_quad(
		       p,
		       frame_renderer_vec2f(p.x + s.x, p.y),
		       frame_renderer_vec2f(p.x, p.y + s.y),
		       frame_renderer_vec2f(p.x + s.x, p.y + s.y),
		       c, c, c, c,
		       uvp,
		       frame_renderer_vec2f(uvp.x + uvs.x, uvp.y),
		       frame_renderer_vec2f(uvp.x, uvp.y + uvs.y),
		       frame_renderer_vec2f(uvp.x + uvs.x, uvp.y + uvs.y));
}


FRAME_DEF void frame_renderer_solid_circle(Frame_Renderer_Vec2f pos,
					     float start_angle, float end_angle,
					     float radius,
					     int parts,
					     Frame_Renderer_Vec4f color) {
  float P = end_angle - start_angle;
  float A = start_angle;

  Frame_Renderer_Vec2f old = {radius * cosf(A),
			       radius * sinf(A)};

  float t = 0.0f;
  float dt = 1.f / (float) parts;

  for(int j=1;j<=parts;j++) {
    t += dt;
    Frame_Renderer_Vec2f new = {radius * cosf(A + P * t),
				 radius * sinf(A + P * t)};
    frame_renderer_solid_triangle(pos,
				   frame_renderer_vec2f(pos.x + new.x, pos.y + new.y),
				   frame_renderer_vec2f(pos.x + old.x, pos.y + old.y),
				   color);
    old = new;
  }

  
}

FRAME_DEF bool frame_renderer_create_texture(int width, int height, unsigned int *index) {
  if(!frame_renderer_push_texture(width, height, NULL, false, index)) {
    return false;
  }

  return true;
}

FRAME_DEF bool frame_renderer_push_to_texture(unsigned int tex, const void *data, int x_off, int y_off, int width, int height) {
  Frame_Renderer *r = &frame_renderer;
  
  if(tex >= r->images_count) return false;
  
  GLenum current_texture;
  switch(tex) {
  case 0:
    current_texture = GL_TEXTURE0;
    break;
  case 1:
    current_texture = GL_TEXTURE1;
    break;
  case 2:
    current_texture = GL_TEXTURE2;
    break;
  case 3:
    current_texture = GL_TEXTURE3;
    break;
  default:
    return false;
  }

  glActiveTexture(current_texture);
  
  glTexSubImage2D(GL_TEXTURE_2D,
		  0,
		  x_off, y_off,
		  width, height,
		  GL_RGBA,
		  GL_UNSIGNED_INT_8_8_8_8_REV,
		  data);

  return true;
}

FRAME_DEF bool frame_renderer_push_texture(int width, int height, const void *data, bool grey, unsigned int *index) {

  Frame_Renderer *r = &frame_renderer;

  GLenum current_texture;
  switch(r->images_count) {
  case 0:
    current_texture = GL_TEXTURE0;
    break;
  case 1:
    current_texture = GL_TEXTURE1;
    break;
  case 2:
    current_texture = GL_TEXTURE2;
    break;
  case 3:
    current_texture = GL_TEXTURE3;
    break;
  default:
    return false;
  }
  
  glActiveTexture(current_texture);
  
  glGenTextures(1, &r->textures);
  glBindTexture(GL_TEXTURE_2D, r->textures);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if(grey) {
    glTexImage2D(GL_TEXTURE_2D,
		 0,
		 GL_ALPHA,
		 width,
		 height,
		 0,
		 GL_ALPHA,
		 GL_UNSIGNED_BYTE,
		 data);	
  } else {
    glTexImage2D(GL_TEXTURE_2D,
		 0,
		 GL_RGBA,
		 width,
		 height,
		 0,
		 GL_RGBA,
		 GL_UNSIGNED_INT_8_8_8_8_REV,
		 data);
  }

  *index = r->images_count++;

  return true;
}

#ifdef FRAME_STB_TRUETYPE
#include <stdio.h>

#define FRAME_RENDERER_STB_TEMP_BITMAP_SIZE 1024

FRAME_DEF bool frame_renderer_push_font(const char *filepath, float pixel_height) {

  HANDLE handle = CreateFile(filepath, GENERIC_READ,
			 FILE_SHARE_READ,
			 NULL,
			 OPEN_EXISTING,
			 FILE_ATTRIBUTE_NORMAL,
			 NULL);
  if(handle == INVALID_HANDLE_VALUE) {
    FRAME_LOG("Can not open file: %s\n", filepath);
    return false;    
  }

  DWORD m = GetFileSize(handle, NULL);
  if(m == INVALID_FILE_SIZE) {
    FRAME_LOG("Can not query file size: %s\n", filepath);
    CloseHandle(handle);
    return false;
  }

  unsigned char *buffer = (unsigned char *) malloc((size_t) m);
  if(!buffer) {
    FRAME_LOG("Can not allocate enough memory\n");
    CloseHandle(handle);
    return false;
  }

  DWORD n;
  if(!ReadFile(handle, buffer, m, &n, NULL) || n != m) {
    FRAME_LOG("Failed to read from file: %s\n", filepath);	
    free(buffer);
    CloseHandle(handle);
  }

  CloseHandle(handle);
  
  unsigned char *temp_bitmap = malloc(FRAME_RENDERER_STB_TEMP_BITMAP_SIZE *
				      FRAME_RENDERER_STB_TEMP_BITMAP_SIZE);
  
  Frame_Renderer *r = &frame_renderer;
  stbtt_BakeFontBitmap(buffer, 0, pixel_height,
		       temp_bitmap,
		       FRAME_RENDERER_STB_TEMP_BITMAP_SIZE,
		       FRAME_RENDERER_STB_TEMP_BITMAP_SIZE,
		       32, 96, r->font_cdata);

  unsigned int tex;
  bool result = push_texture(1024, 1024, temp_bitmap, true, &tex);

  r->font_index = (int) tex;
  r->font_height = pixel_height;

  free(buffer);
  free(temp_bitmap);
    
  return result;
  
}

FRAME_DEF bool frame_renderer_push_font_memory(unsigned char *memory, size_t memory_len, float pixel_height) {
  (void) memory_len;
  
  unsigned char *temp_bitmap = malloc(FRAME_RENDERER_STB_TEMP_BITMAP_SIZE *
				      FRAME_RENDERER_STB_TEMP_BITMAP_SIZE);
  
  Frame_Renderer *r = &frame_renderer;
  stbtt_BakeFontBitmap(memory, 0, pixel_height,
		       temp_bitmap,
		       FRAME_RENDERER_STB_TEMP_BITMAP_SIZE,
		       FRAME_RENDERER_STB_TEMP_BITMAP_SIZE,
		       32, 96, r->font_cdata);

  unsigned int tex;
  bool result = push_texture(1024, 1024, temp_bitmap, true, &tex);

  r->font_index = (int) tex;
  r->font_height = pixel_height;

  free(temp_bitmap);
    
  return result;
}

FRAME_DEF void frame_renderer_text(const char *cstr, size_t cstr_len, Frame_Renderer_Vec2f pos, float factor, Frame_Renderer_Vec4f color) {

  Frame_Renderer *r = &frame_renderer;

  float x = 0;
  float y = 0;
  color.w *= -1;
    
  for(size_t i=0;i<cstr_len;i++) {
    unsigned char c = cstr[i];
    if (c < 32 && c >= 128) {
      continue;
    }

    float _y = y;

    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(r->font_cdata,
		       FRAME_RENDERER_STB_TEMP_BITMAP_SIZE,
		       FRAME_RENDERER_STB_TEMP_BITMAP_SIZE, c-32, &x, &y, &q,1);
    //1=opengl & d3d10+,0=d3d9


    Frame_Renderer_Vec2f p = vec2f(pos.x + q.x0 * factor,pos.y + y + _y + factor * (y - q.y1) );
    Frame_Renderer_Vec2f s = vec2f((q.x1 - q.x0) * factor, (q.y1 - q.y0) * factor);
    Frame_Renderer_Vec2f uvp = vec2f(q.s0, 1 - q.t1);
    Frame_Renderer_Vec2f uvs = vec2f(q.s1 - q.s0, q.t1 - q.t0);
	
    frame_renderer_quad(
			 p,
			 frame_renderer_vec2f(p.x + s.x, p.y),
			 frame_renderer_vec2f(p.x, p.y + s.y),
			 frame_renderer_vec2f(p.x + s.x, p.y + s.y),
			 color, color, color, color,
			 uvp,
			 frame_renderer_vec2f(uvp.x + uvs.x, uvp.y),
			 frame_renderer_vec2f(uvp.x, uvp.y + uvs.y),
			 frame_renderer_vec2f(uvp.x + uvs.x, uvp.y + uvs.y));
  }

}

FRAME_DEF void frame_renderer_text_wrapped(const char *cstr, size_t cstr_len, Frame_Renderer_Vec2f *pos, Frame_Renderer_Vec2f size, float scale, Frame_Renderer_Vec4f color) {
  Frame_Renderer *r = &frame_renderer;
  Vec2f text_size;
  
  size_t i = 0;
  while(i < cstr_len) {
    size_t j=1;
    for(;j<cstr_len - i;j++) {
      measure_text_len(cstr + i, j, scale, &text_size);
      if(text_size.x >= size.x) break;
    }

    draw_text_len_colored(cstr + i, j, *pos, scale, color);
    i += j;
    pos->y -= r->font_height * scale;    
  }
}

FRAME_DEF bool frame_renderer_text_button(const char *cstr, size_t cstr_len, float scale, Frame_Renderer_Vec4f text_color, Frame_Renderer_Vec2f p, Frame_Renderer_Vec2f s, Frame_Renderer_Vec4f c) {
  bool holding = frame_renderer_button_impl(p, s, &c);
  frame_renderer_solid_rect(p, s, c);
  if(holding) {
    text_color.w *= .5;
  }

  Vec2f size;
  frame_renderer_measure_text(cstr, cstr_len, scale, &size);

  if(size.x <= s.x && size.y <= s.y) {
    Vec2f pos = vec2f(p.x + s.x / 2 - size.x / 2,
		      p.y + s.y / 2 - size.y / 2);
    frame_renderer_text(cstr, cstr_len, pos, scale, text_color);
  }
  
  return frame_renderer.released && holding;
}

FRAME_DEF void frame_renderer_measure_text(const char *cstr, size_t cstr_len, float factor, Vec2f *size) {
  Frame_Renderer *r = &frame_renderer;

  float hi = 0;
  float lo = 0;

  size->y = 0;
  size->x = 0;

  float x = 0;
  float y = 0;
    
  for(size_t i=0;i<cstr_len;i++) {
    unsigned char c = cstr[i];
    if (c < 32 && c >= 128) {
      continue;
    }

    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(r->font_cdata,
		       FRAME_RENDERER_STB_TEMP_BITMAP_SIZE,
		       FRAME_RENDERER_STB_TEMP_BITMAP_SIZE,
		       c-32, &x, &y, &q, 1);

    size->x = q.y1 - q.y0;
    float height = q.x1 - q.x0;
    if(height > hi) hi = height;
    if(height < lo) lo = height;
  }
  size->x = x * factor;  
  size->y = (hi - lo) * factor;
}

#endif //FRAME_STB_TRUETYPE

#ifdef FRAME_STB_IMAGE

FRAME_DEF bool frame_renderer_push_image(const char *filepath, int *width, int *height, unsigned int *index) {

  unsigned char *image_data = stbi_load(filepath, width, height, NULL, 4);
  if(image_data == NULL) {
    return false;
  }

  bool result = frame_renderer_push_texture(*width, *height, image_data, false, index);
  stbi_image_free(image_data);

  return result;
}

FRAME_DEF bool frame_renderer_push_image_memory(unsigned char *data, size_t data_len, int *width, int *height, unsigned int *index) {
  unsigned char *image_data = stbi_load_from_memory(data, (int) data_len, width, height, 0, 4);
  if(image_data == NULL) {
    return false;
  }

  bool result = frame_renderer_push_texture(*width, *height, image_data, false, index);
  stbi_image_free(image_data);

  return result;
}

#endif // FRAME_STB_IMAGE

#endif //FRAME_NO_RENDERER

////////////////////////////////////////////////////////////////////////
// opengl - definitions
////////////////////////////////////////////////////////////////////////

PROC _glActiveTexture = NULL;
void glActiveTexture(GLenum texture) { _glActiveTexture(texture); }

PROC _glGenVertexArrays = NULL;
void glGenVertexArrays(GLsizei n, GLuint *arrays) { _glGenVertexArrays(n, arrays); }

PROC _glBindVertexArray = NULL;
void glBindVertexArray(GLuint array) { _glBindVertexArray(array); }

PROC _glGenBuffers = NULL;
void glGenBuffers(GLsizei n, GLuint *buffers) { _glGenBuffers(n, buffers); }

PROC _glBindBuffer = NULL;
void glBindBuffer(GLenum target, GLuint buffer) { _glBindBuffer(target, buffer); }

PROC _glBufferData = NULL;
void glBufferData(GLenum target, GLsizeiptr size, const void * data, GLenum usage) { _glBufferData(target, size, data, usage); }

PROC _glEnableVertexAttribArray = NULL;
void glEnableVertexAttribArray(GLuint index) { _glEnableVertexAttribArray(index); }

PROC _glVertexAttribPointer = NULL;
void glVertexAttribPointer(GLuint index,
			   GLint size,
			   GLenum type,
			   GLboolean normalized,
			   GLsizei stride,
			   const void * pointer) {
  _glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

PROC _glCreateShader = NULL;
GLuint glCreateShader(GLenum shaderType) { return (GLuint) _glCreateShader(shaderType); }

PROC _glShaderSource = NULL;
void glShaderSource(GLuint shader,
		    GLsizei count,
		    const GLchar **_string,
		    const GLint *length) {
  _glShaderSource(shader, count, _string, length);
}

PROC _glCompileShader = NULL;
void glCompileShader(GLuint shader) { _glCompileShader(shader); }

PROC _glGetShaderiv = NULL;
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
  _glGetShaderiv(shader, pname, params);
}

PROC _glGetShaderInfoLog = NULL;
void glGetShaderInfoLog(GLuint shader,
			GLsizei maxLength,
			GLsizei *length,
			GLchar *infoLog) {
  _glGetShaderInfoLog(shader, maxLength, length, infoLog);
}

PROC _glCreateProgram = NULL;
GLuint glCreateProgram(void) { return (GLuint) _glCreateProgram(); }

PROC _glAttachShader = NULL;
void glAttachShader(GLuint program, GLuint shader) { _glAttachShader(program, shader); }

PROC _glLinkProgram = NULL;
void glLinkProgram(GLuint program) { _glLinkProgram(program); }

PROC _glGetProgramInfoLog = NULL;
void glGetProgramInfoLog(GLuint program,
			 GLsizei maxLength,
			 GLsizei *length,
			 GLchar *infoLog) {
  _glGetProgramInfoLog(program, maxLength, length, infoLog);
}

PROC _glGetProgramiv = NULL;
void glGetProgramiv(GLuint program,
		    GLenum pname,
		    GLint *params) {
  _glGetProgramiv(program, pname, params);
}

PROC _glUseProgram = NULL;
void glUseProgram(GLuint program) { _glUseProgram(program); }

PROC _glBufferSubData = NULL;
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void * data) {
  _glBufferSubData(target, offset, size, data);
}

PROC _glUniform2f = NULL;
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
  _glUniform2f(location, v0, v1);
}

PROC _glUniform1f = NULL;
void glUniform1f(GLint location, GLfloat v0) {
  _glUniform1f(location, v0);
}

PROC _glUniform1fv = NULL;
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
  _glUniform1fv(location, count, value);
}

PROC _glUniform2fv = NULL;
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
  _glUniform2fv(location, count, value);
}

PROC _glUniform1i = NULL;
void glUniform1i(GLint location, GLint v0) {
  _glUniform1i(location, v0);
}

PROC _glGetUniformLocation = NULL;
GLint glGetUniformLocation(GLuint program, const GLchar *name) { return (GLint) _glGetUniformLocation(program, name); }

PROC _glGetActiveUniform = NULL;
void glGetActiveUniform(GLuint program,
			GLuint index,
			GLsizei bufSize,
			GLsizei *length,
			GLint *size,
			GLenum *type,
			GLchar *name) {
  _glGetActiveUniform(program, index, bufSize, length, size, type, name);
}

PROC _glGetUniformfv = NULL;
void glGetUniformfv(GLuint program, GLint location, GLfloat *params) {
  _glGetUniformfv(program, location, params);
}

PROC _glGetUniformiv = NULL;
void glGetUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params) {
  _glGetUniformiv(program, location, bufSize, params);
}

PROC _wglSwapIntervalEXT = NULL;
int wglSwapIntervalEXT(GLint interval) {
  return (int) _wglSwapIntervalEXT(interval);
}

PROC _glSampleCoverage = NULL;
void glSampleCoverage(GLfloat value, GLboolean invert) {
  _glSampleCoverage(value, invert);
}


FRAME_DEF void frame_win32_opengl_init() {
  if(_glActiveTexture != NULL) {
    return;
  }
  
  _glActiveTexture = wglGetProcAddress("glActiveTexture");
  _glGenVertexArrays = wglGetProcAddress("glGenVertexArrays");
  _glBindVertexArray= wglGetProcAddress("glBindVertexArray");
  _glGenBuffers= wglGetProcAddress("glGenBuffers");
  _glBindBuffer= wglGetProcAddress("glBindBuffer");
  _glBufferData= wglGetProcAddress("glBufferData");
  _glEnableVertexAttribArray= wglGetProcAddress("glEnableVertexAttribArray");
  _glVertexAttribPointer= wglGetProcAddress("glVertexAttribPointer");
  _glCreateShader= wglGetProcAddress("glCreateShader");
  _glShaderSource= wglGetProcAddress("glShaderSource");
  _glCompileShader= wglGetProcAddress("glCompileShader");
  _glGetShaderiv= wglGetProcAddress("glGetShaderiv");
  _glGetShaderInfoLog= wglGetProcAddress("glGetShaderInfoLog");
  _glCreateProgram= wglGetProcAddress("glCreateProgram");
  _glAttachShader= wglGetProcAddress("glAttachShader");
  _glLinkProgram= wglGetProcAddress("glLinkProgram");
  _glGetProgramInfoLog= wglGetProcAddress("glGetProgramInfoLog");
  _glGetProgramiv= wglGetProcAddress("glGetProgramiv");
  _glUseProgram= wglGetProcAddress("glUseProgram");
  _glBufferSubData= wglGetProcAddress("glBufferSubData");
  _glUniform2f= wglGetProcAddress("glUniform2f");
  _glUniform1f= wglGetProcAddress("glUniform1f");
  _glUniform1i= wglGetProcAddress("glUniform1i");
  _glGetUniformLocation= wglGetProcAddress("glGetUniformLocation");
  _glGetActiveUniform= wglGetProcAddress("glGetActiveUniform");
  _glGetUniformfv= wglGetProcAddress("glGetUniformfv");
  _glUniform1fv= wglGetProcAddress("glUniform1fv");
  _glUniform2fv= wglGetProcAddress("glUniform2fv");
  _glGetUniformiv= wglGetProcAddress("glGetUniformiv");
  _wglSwapIntervalEXT = wglGetProcAddress("wglSwapIntervalEXT");
}

#endif //FRAME_IMPLEMENTATION

#endif //FRAME_H
