#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "display.h"
#include "interrupts.h"

#define WIDTH 160
#define HEIGHT 144
#define LY 0xFF44
#define LYC 0xFF45
#define STATUS 0xFF41

#define BLACK 0xFF000000
#define DARK_GRAY 0xFF555555
#define LIGHT_GRAY 0xFFAAAAAA
#define WHITE 0xFFFFFFFF


BYTE *lcd_ctrl = &rom[0xFF40];
unsigned int screen[23040]; // WIDTH * HEIGHT
GLFWwindow * window = NULL;
unsigned int texture;
int prev_mode = 0;
int mode_clock = 0;
char vertex_shader[1024 * 256];
char fragment_shader[1024 * 256];
int display_width = WIDTH * 5;
int display_height = HEIGHT * 5;
int vertex, fragment, program = 0;
unsigned int VBO, VAO, EBO = 0;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
int parse_file_into_str(const char *file_name, char *shader_str, int max_len);
void initalize_shader(int *vertex, int *fragment, int *program);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

/* 0xFF41 LCDC Status (R/W)
    Bit 6 - LYC=LY Coincidence Interrupt (1 = Enable) (R/W)
    Bit 5 - Mode 2 OAM Interrupt (1 = Enable) (R/W)
    Bit 4 - Mode 1 V-Blank Interrupt (1 = Enable) (R/W)
    Bit 3 - Mode 0 H-Blank Interrupt (1 = Enable) (R/W)
    Bit 2 - Coincidence Flag (0: LYC<>LY, 1: LYC = LY) (R Only)
    Bit 1-0 - Mode Flag (Mode 0-3) (R Only)
        0: During H-Blank
        1: During V-Blank
        2: During Searching OAM-RAM
        3: During Transferring Data to LCD Driver
 */

void draw(int cycles) {
   if (test_bit(7, lcd_ctrl) == 0) {
        rom[LY] = 0;
        rom[STATUS] &= 252;
        rom[STATUS] &= ~1;
        return;
   }

    mode_clock += cycles;
    switch (get_stat_mode()) {

        // Scanning OAM
    case 2:
        if (mode_clock >= 80) {
            mode_clock = 0;
            set_stat_mode(3);
        }
        break;

        // Reading OAM and VRAM
    case 3:
        if (mode_clock >= 172) {
            mode_clock = 0;
            set_stat_mode(0);
            draw_scanline();
        }
        break;

        // Horizontal blanking
    case 0:
        if (mode_clock >= 204) {
            mode_clock = 0;
            rom[LY] += 1;
            
            if (rom[LY] == 143) {
                set_stat_mode(1);
                render_display();
                int x = 0;
            }
            else {
                set_stat_mode(2);
            }
        }
        break;

        // Vertical blanking
    case 1:
        if (mode_clock >= 456) {
            // Request V-Blank Interrupt
            if (rom[LY] == 144) {
                request_interrupt(0);
            }
            mode_clock = 0;
            rom[LY] += 1;
            
            if (rom[LY] > 153) {
                set_stat_mode(2);
                rom[LY] = 0;
            }
        }
        break;
    }

    // Change mode flag in LCD Status Register
    if (prev_mode != get_stat_mode()) {
        prev_mode = get_stat_mode();

        // If interrupt is enabled for mode 0,1,or 2, then request lcd interrupt
        if (get_stat_mode() != 3) {
            BYTE b = test_bit(get_stat_mode() + 3, &rom[STATUS]);
            if (b)
                request_interrupt(LCD);
        }
    }

    if (rom[LY] == rom[LYC]) {
        rom[STATUS] |= 0x04;
        if(test_bit(6, &rom[STATUS]))
            request_interrupt(LCD);
    }
    else {
        rom[STATUS] &= ~0x04;
    }
}

/*  0xFF40 LCD Control (R/W)
    Bit 7 - LCD Display Enable  (0 = Off, 1 = On)
    Bit 6 - Window Tile Map Display Select (0 = 9800-9BFF, 1 = 9C00-9FFF)
    Bit 5 - Window Display Enable   (0 = Off, 1 = On)
    Bit 4 - BG & Window Tile Data Select  (0 = 8800-97FF, 1 = 8000-8FFF)
    Bit 3 - BG Tile Map Display Select  (0 = 9800-9BFF, 1 = 9C00-9FFF)
    Bit 2 - OBJ (Sprite) Size   (0 = 8x8, 1 = 8x16)
    Bit 1 - OBJ (Sprite) Display Enable (0 = Off, 1 = On)
    Bit 0 - BG Display  (0 = Off, 1 = On)
*/

void draw_scanline() {
  if (test_bit(0, lcd_ctrl) == 1) {
       draw_tile();
  }
  if (test_bit(1, lcd_ctrl) == 1) {
      draw_sprites();
  }
}

void draw_tile() {
    /*  Specifies the position in the 256x256 pixels BG map where the upper left corner of the LCD is to be displayed */
    int scrollY = read_memory(0xFF42);
    int scrollX = read_memory(0xFF43);
    
    int bg_map_addr, tile_data_addr;

    
    if (test_bit(3, lcd_ctrl)) {
        bg_map_addr = 0x9C00;
    }
    else {
        bg_map_addr = 0x9800;
    }

    if (test_bit(4, lcd_ctrl)) {
        tile_data_addr = 0x8000;
    }
    else {
        tile_data_addr = 0x8800;
    }

    int scanline = read_memory(0xFF44);
    int yPos = scrollY + scanline;

    /* The screen can wrap around bg map, so % 256 is used to set the position at the top if its greater than 256. 
       Then it's divided by 8 to show which of the 32 tiles the display should be rendering. Each row has 32 bytes,
       so multiply the by 32*/
    int vertical_tile = ((yPos % 256) / 8) * 32;

    /* Set the pixels in the display for the current scanline */
    for (int x = 0; x < 20; x++) {
        
        int xPos = scrollX + (x * 8);
        int horizontal_tile = (xPos % 256) / 8;
        
        /* Retrieve index of tile to render */
        int tile_num = read_memory(bg_map_addr + vertical_tile + horizontal_tile);
        
        /* The tile data is 8x8 pixels. Eight rows that contain two bytes of data */
        int tile_data_row = (yPos % 8) * 2;
        
        /* Retrieve the tile data lower byte */
        BYTE tile_data_lb = read_memory((tile_num * 16) + tile_data_row + tile_data_addr);
        /* Retrieve the tile data upper byte */
        BYTE tile_data_ub = read_memory((tile_num * 16) + tile_data_row + tile_data_addr + 1);

        // Draw 8 pixels for the row of tile data
        for (int i = 7; i >= 0; i--) {
            int color_number = (test_bit(i, &tile_data_ub) << 1) | test_bit(i, &tile_data_lb);
            screen[(scanline * WIDTH) + (x * 8) + (7 - i)] = get_color(color_number);
        }
    }
}

void draw_sprites() {
    int scanline = read_memory(0xFF44);
    int sprite_size;

    if (test_bit(2, lcd_ctrl) == 1) {
        sprite_size = 16;
    }
    else {
        sprite_size = 8;
    }

    for (int i = 0; i < 40; i++) {
        /* There can be 40 sprites in the scene, and each sprite attribute is 4 bytes long */
        BYTE yPos = read_memory(0xFE00 + ((i * 4) + 0));
        BYTE xPos = read_memory(0xFE00 + ((i * 4) + 1));
        BYTE tile_num = read_memory(0xFE00 + ((i * 4) + 2));
        BYTE flags = read_memory(0xFE00 + ((i * 4) + 3));
        
        if (sprite_size = 8) {
            if (scanline < (yPos - 16) || scanline > (yPos - 16 + 7)) {
                continue;
            }
            if (xPos == 0 || xPos >= 168) {
                continue;
            }
        }
        else if (sprite_size = 16) {
            if (scanline < (yPos - 16) || scanline > (yPos - 16 + 15)) {
                continue;
            }
            if (xPos == 0 || xPos >= 168) {
                continue;
            }
        }

        int sprite_row = scanline - (yPos - 16);
        /* Retrieve the tile data lower byte */
        BYTE tile_data_lb = read_memory((tile_num * 16) + sprite_row + 0x8000);
        /* Retrieve the tile data upper byte */
        BYTE tile_data_ub = read_memory((tile_num * 16) + sprite_row + 0x8000 + 1);

        // Draw 8 pixels for the row of tile data
        for (int i = 7; i >= 0; i--) {
            if ((xPos - 8) + i < 0 || (xPos - 8) + i > 159) {
                continue;
            }
            int color_number = (test_bit(i, &tile_data_ub) << 1)  | test_bit(i, &tile_data_lb);
            screen[(scanline * WIDTH) + (xPos - 8) + (7 - i)] = get_color(color_number);
        }
    }
}

unsigned int get_color(int color_number){
    unsigned int color = 0;
    
    // Retreive Palette Data
    BYTE palette = read_memory(0xFF47);
    BYTE number = (palette >> (2 * color_number)) & 0x03;

    switch (number) {
    case 0x00:
        color = WHITE;
        break;
    case 0x01:
        color = LIGHT_GRAY;
        break;
    case 0x02:
        color = DARK_GRAY;
        break;
    case 0x03:
        color = BLACK;
        break;
    }
    return color;
}

int get_stat_mode(void) {
    return (rom[STATUS] & 0x03);
}

void set_stat_mode(unsigned int mode) {
    if (mode > 3) {
        return;
    }
    // Clear mode
    rom[STATUS] &= ~(0x03);
    // Set mode
    rom[STATUS] |= mode;
}

int display_init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(display_width, display_height, "Gameboy", NULL, NULL);
    if (window == NULL)
    {
        glfwTerminate();
        return -1;
    }
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
   
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return -1;
    }
    parse_file_into_str("vertex_shader.vs", vertex_shader, 1024 * 256);
    parse_file_into_str("fragment_shader.fs", fragment_shader, 1024 * 256);
    initalize_shader(&vertex, &fragment, &program);
    
    GLfloat vertices[] = {
        // positions          // texture coords
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f,   // top right
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,   // bottom left
        -1.0f,  1.0f, 0.0f,   0.0f, 1.0f    // top left 
    };

    GLuint indices[6] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // load and create a texture 
    // -------------------------
    // texture 1
    // ---------
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, screen);

    glUseProgram(program);
    return 0;
    
}

void render_display() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, screen);

    // render container
    glUseProgram(program);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glfwSwapBuffers(window);
    glfwPollEvents();
}

GLFWwindow* get_window() {
    return window;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
void initalize_shader(int *vertex, int *fragment, int *program) {
    const GLchar *p;
    
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    p = (const GLchar *)vertex_shader;
    glShaderSource(vertexShader, 1, &p, NULL);
    glCompileShader(vertexShader);
    
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    }
    
    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    p = (const GLchar *)fragment_shader;
    glShaderSource(fragmentShader, 1, &p, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    }
    // link shaders
    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    }
    *vertex = vertexShader;
    *fragment = fragmentShader;
    *program = shaderProgram;
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

int parse_file_into_str(const char *file_name, char *shader_str, int max_len) {
    FILE *file;
    errno_t err;
    if (err = fopen_s(&file, file_name, "r") != 0) {
        // char buf[200];
        // strerror_s(buf, sizeof buf, err);
        // fprintf_s(stderr, "cannot open file '%s': %s\n",
         //    filename, buf);
    }
    size_t cnt = fread(shader_str, 1, max_len - 1, file);
    if ((int)cnt >= max_len - 1) {
        //gl_log_err("WARNING: file %s too big - truncated.\n", file_name);
    }
    if (ferror(file)) {
        //gl_log_err("ERROR: reading shader file %s\n", file_name);
        fclose(file);
        return 0;
    }
    // append \0 to end of file string
    shader_str[cnt] = 0;
    fclose(file);
    return 1;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    // Handle Direction Keys
    if (check_state()) {
        switch (key) {
            // Right
        case GLFW_KEY_D:
            if (action == GLFW_PRESS) {
                cpu_reset_bit(0, &rom[0xFF00]);
            }
            else if (action == GLFW_RELEASE) {
                cpu_set_bit(0, &rom[0xFF00]);
            }
            break;
            // Left
        case GLFW_KEY_A:
            if (action == GLFW_PRESS)
                cpu_reset_bit(1, &rom[0xFF00]);
            else if (action == GLFW_RELEASE)
                cpu_set_bit(1, &rom[0xFF00]);
            break;
            // Up
        case GLFW_KEY_W:
            if (action == GLFW_PRESS)
                cpu_reset_bit(2, &rom[0xFF00]);
            else if (action == GLFW_RELEASE)
                cpu_set_bit(2, &rom[0xFF00]);
            break;
            // Down
        case GLFW_KEY_S:
            if (action == GLFW_PRESS)
                cpu_reset_bit(3, &rom[0xFF00]);
            else if (action == GLFW_RELEASE)
                cpu_set_bit(3, &rom[0xFF00]);
            break;
        }
    }
    // Handle Button Keys
    else {
        switch (key) {
            // A
        case GLFW_KEY_LEFT:
            if (action == GLFW_PRESS)
                cpu_reset_bit(0, &rom[0xFF00]);
            else if (action == GLFW_RELEASE)
                cpu_set_bit(0, &rom[0xFF00]);
            break;
            // B
        case GLFW_KEY_UP:
            if (action == GLFW_PRESS)
                cpu_reset_bit(1, &rom[0xFF00]);
            else if (action == GLFW_RELEASE)
                cpu_set_bit(1, &rom[0xFF00]);
            break;
            // Start
        case GLFW_KEY_ENTER:
            if (action == GLFW_PRESS)
                cpu_reset_bit(2, &rom[0xFF00]);
            else if (action == GLFW_RELEASE)
                cpu_set_bit(2, &rom[0xFF00]);
            break;
            // Select
        case GLFW_KEY_SPACE:
            if (action == GLFW_PRESS)
                cpu_reset_bit(3, &rom[0xFF00]);
            else if (action == GLFW_RELEASE)
                cpu_set_bit(3, &rom[0xFF00]);
            break;
        }
    }
}


int check_state() {
    BYTE Joypad = rom[0xFF00];
    // Test for Direction Keys
    if (test_bit(4, &Joypad)) {
        return 1;
    }
    else {
        return 0;
    }
}