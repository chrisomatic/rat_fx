#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "window.h"

static GLFWwindow* window;

int window_width = 0;
int window_height = 0;
int view_width = 0;
int view_height = 0;

static key_cb_t key_cb = NULL;
static KeyMode key_mode = KEY_MODE_NONE;
static char* text_buf = NULL;
static int text_buf_max = 0;

static double window_coord_x = 0;
static double window_coord_y = 0;

static void window_size_callback(GLFWwindow* window, int _window_width, int _window_height);
static void window_maximize_callback(GLFWwindow* window, int maximized);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void char_callback(GLFWwindow* window, unsigned int code);
static void key_callback(GLFWwindow* window, int key, int scan_code, int action, int mods);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

static void text_buf_append(char c);
static void text_buf_backspace();

bool window_init(int _view_width, int _view_height)
{
    printf("Initializing GLFW.\n");

    if(!glfwInit())
    {
        fprintf(stderr,"Failed to init GLFW!\n");
        return false;
    }

    view_width = _view_width;
    view_height = _view_height;

    window_width = view_width;
    window_height = view_height;

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Want to use OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

    // printf("vw: %d, vh: %d\n", view_width, view_height);
    //window = glfwCreateWindow(view_width,view_height,"Postmortem",glfwGetPrimaryMonitor(),NULL);
    window = glfwCreateWindow(view_width,view_height,"Postmortem",NULL,NULL);

    if(window == NULL)
    {
        fprintf(stderr, "Failed to create GLFW Window!\n");
        glfwTerminate();
        return false;
    }

    glfwSetWindowAspectRatio(window,ASPECT_NUM,ASPECT_DEM);
    glfwSetWindowSizeCallback(window,window_size_callback);
    glfwSetWindowMaximizeCallback(window, window_maximize_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, char_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwMaximizeWindow(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    printf("Initializing GLEW.\n");

    // GLEW
    glewExperimental = 1;
    if(glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return false;
    }

    return true;
}


void window_get_mouse_coords(int* x, int* y)
{
    *x = (int)(window_coord_x);
    *y = (int)(window_coord_y);
}

void window_get_mouse_view_coords(int* x, int* y)
{
    window_get_mouse_coords(x,y);
    *x *= (view_width/(float)window_width);
    *y *= (view_height/(float)window_height);
}

void window_set_mouse_view_coords(int x, int y)
{
    double _x = (double)x / (view_width/(float)window_width);
    double _y = (double)y / (view_height/(float)window_height);
    glfwSetCursorPos(window, _x, _y);
}

void window_get_mouse_world_coords(float* x, float* y)
{
    int mouse_x, mouse_y;
    window_get_mouse_view_coords(&mouse_x, &mouse_y);

    Matrix* view = get_camera_transform();
    float cam_x = view->m[0][3];
    float cam_y = view->m[1][3];

    *x = mouse_x - cam_x;
    *y = mouse_y - cam_y;
}

void window_set_mouse_world_coords(float x, float y)
{
    Matrix* view = get_camera_transform();
    float cam_x = view->m[0][3];
    float cam_y = view->m[1][3];
    double _x = x + cam_x;
    double _y = y + cam_y;
    _x = (double)_x / (view_width/(float)window_width);
    _y = (double)_y / (view_height/(float)window_height);
    // printf("setting mouse world: %.2f, %.2f\n", x, y);
    glfwSetCursorPos(window, _x, _y);
}

void window_deinit()
{
    glfwTerminate();
}

void window_poll_events()
{
    glfwPollEvents();
}

bool window_should_close()
{
    return (glfwWindowShouldClose(window) != 0);
}

void window_set_close(int value)
{
    glfwSetWindowShouldClose(window,value);
}

void window_swap_buffers()
{
    glfwSwapBuffers(window);
}

static void window_size_callback(GLFWwindow* window, int width, int height)
{
    printf("Window: W %d, H %d\n",width,height);

    window_height = height;
    window_width  = width; //ASPECT_RATIO * window_height;

    int start_x = (window_width + window_width) / 2.0f - window_width;
    int start_y = (window_height + window_height) / 2.0f - window_height;

    glViewport(start_x,start_y,window_width,window_height);
}

static void window_maximize_callback(GLFWwindow* window, int maximized)
{
    if(maximized)
    {
        // The window was maximized
        printf("Maximized.\n");
    }
    else
    {
        // The window was restored
        printf("Restored.\n");
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    window_coord_x = xpos;
    window_coord_y = ypos;
}

typedef struct
{
    uint16_t* keys;
    int key;
    int bit_num;
} WindowKey;

static WindowKey window_keys[32];
static int window_keys_count = 0;

static WindowKey window_mouse_buttons[10];
static int window_mouse_buttons_count = 0;


bool window_controls_is_key_state(int key, int state)
{
    return glfwGetKey(window, key) == state;
}

void window_controls_set_cb(key_cb_t cb)
{
    key_cb = cb;
}

void window_controls_set_text_buf(char* buf, int max_len)
{
    text_buf = buf;
    text_buf_max = max_len;
}

void window_controls_set_key_mode(KeyMode mode)
{
    key_mode = mode;
}

KeyMode window_controls_get_key_mode()
{
    return key_mode;
}


void window_controls_clear_keys()
{
    memset(window_keys, 0, sizeof(WindowKey)*32);
    memset(window_mouse_buttons, 0, sizeof(WindowKey)*10);
    window_keys_count = 0;
    window_mouse_buttons_count = 0;
}

void window_controls_add_key(uint16_t* keys, int key, int bit_num)
{
    window_keys[window_keys_count].keys = keys;
    window_keys[window_keys_count].key = key;
    window_keys[window_keys_count].bit_num = bit_num;

    window_keys_count++;
}

void window_controls_add_mouse_button(uint16_t* keys, int key, int bit_num)
{
    window_mouse_buttons[window_mouse_buttons_count].keys = keys;
    window_mouse_buttons[window_mouse_buttons_count].key = key;
    window_mouse_buttons[window_mouse_buttons_count].bit_num = bit_num;

    window_mouse_buttons_count++;
}

bool window_is_cursor_enabled()
{
    int mode = glfwGetInputMode(window,GLFW_CURSOR);
    return (mode == GLFW_CURSOR_NORMAL);
}

void window_enable_cursor()
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void window_disable_cursor()
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

static void char_callback(GLFWwindow* window, unsigned int code)
{
    char c = (char)code;
    if(key_mode == KEY_MODE_TEXT)
        text_buf_append(c);
}

static void key_callback(GLFWwindow* window, int key, int scan_code, int action, int mods)
{
    printf("key callback: %d\n", key);

    if(action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        if(key_mode == KEY_MODE_NORMAL)
        {
            for(int i = 0; i < window_keys_count; ++i)
            {
                WindowKey* wk = &window_keys[i];
                if(key == wk->key)
                {
                    if(action == GLFW_PRESS)
                        (*wk->keys) |= wk->bit_num;
                    else
                        (*wk->keys) &= ~(wk->bit_num);
                }
            }
        }
        else if(key_mode == KEY_MODE_TEXT)
        {
            if(action == GLFW_PRESS)
            {
                if(key == GLFW_KEY_ENTER)
                {
                    text_buf_append('\n');
                }
                else if(key == GLFW_KEY_BACKSPACE)
                {
                    text_buf_backspace();
                }
            }
        }

    }

    if(key_mode != KEY_MODE_NONE && key_cb != NULL)
    {
        key_cb(window, key, scan_code, action, mods);
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        for(int i = 0; i < window_mouse_buttons_count; ++i)
        {
            WindowKey* wk = &window_mouse_buttons[i];

            if(button == wk->key)
            {
                if(action == GLFW_PRESS)
                {
                    (*wk->keys) |= wk->bit_num;

                    // if(window_is_cursor_enabled())
                    // {
                    //     window_disable_cursor();
                    // }
                }
                else
                    (*wk->keys) &= ~(wk->bit_num);
            }
        }
    }
}

static void text_buf_append(char c)
{
    if(text_buf != NULL)
    {
        int len = strlen(text_buf);

        int index = MIN(len,text_buf_max-1);

        text_buf[index] = c;

        // // always save space for \n at the end
        // if(len >= (text_buf_max-1))
        //     return;
        // text_buf[len] = c;
    }
}

static void text_buf_backspace()
{
    if(text_buf != NULL)
    {
        int len = strlen(text_buf);
        if(len == 0)
            return;
        text_buf[len-1] = '\0';
        printf("backspace\n");
    }
}


