#define _USE_MATH_DEFINES // Define this before including cmath to get M_PI
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm> // For std::min, std::max, std::abs
#include <string>
#include <sstream> // For std::stringstream

// For image saving functionality
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" // Make sure this header file is in your include path

// --- Theme Colors ---
// Main Background: Light blue-grey
const float BG_R = 0.91f, BG_G = 0.95f, BG_B = 0.96f; // E8F3F7

// Panel Background: Cream white
const float PANEL_R = 1.0f, PANEL_G = 1.0f, PANEL_B = 1.0f; // FFFFFF

// Accent Color (Top Bar, potentially selected tool outline)
const float ACCENT_R = 0.698f, ACCENT_G = 0.898f, ACCENT_B = 0.914f; // B2EBF2

// Text Color: Dark Grey
const float TEXT_R = 0.2f, TEXT_G = 0.2f, TEXT_B = 0.2f; // 333333

// Border/Outline Color: Medium Grey
const float BORDER_R = 0.7f, BORDER_G = 0.7f, BORDER_B = 0.7f;

// Button Default: Light Grey
const float BUTTON_DEFAULT_R = 0.92f, BUTTON_DEFAULT_G = 0.92f, BUTTON_DEFAULT_B = 0.92f;

// Button Hover: Slightly brighter
const float BUTTON_HOVER_R = 0.85f, BUTTON_HOVER_G = 0.85f, BUTTON_HOVER_B = 0.85f;

// Button Selected: Medium Grey (This color is now primarily for tool buttons, not color swatches)
const float BUTTON_SELECTED_R = 0.75f, BUTTON_SELECTED_G = 0.75f, BUTTON_SELECTED_B = 0.75f;

// Clear Button Color: Soft Red
const float CLEAR_BUTTON_R = 0.9f, CLEAR_BUTTON_G = 0.4f, CLEAR_BUTTON_B = 0.4f;

// Shadow Color
const float SHADOW_R = 0.05f, SHADOW_G = 0.05f, SHADOW_B = 0.05f; // Very dark grey
const float SHADOW_ALPHA = 0.2f; // Semi-transparent

// Grid Color
const float GRID_R = 0.85f, GRID_G = 0.85f, GRID_B = 0.85f; // Faint grey

// --- Structures ---
struct Point {
    float x, y;
    float r, g, b;
    Point(float x_coord = 0, float y_coord = 0, float red = 0, float green = 0, float blue = 0)
        : x(x_coord), y(y_coord), r(red), g(green), b(blue) {}
};

struct Stroke {
    std::vector<Point> points; // Points for brush/eraser/line/outline
    int tool; // 0=brush, 1=eraser, 2=rectangle, 3=circle, 4=line, 5=fill
    float size; // Size for brush/eraser/outline thickness
    float fillColor[3] = {0, 0, 0}; // Color for fill tool
    Point rectStart, rectEnd; // For filled rectangle bounds
    Point circleCenter; // For filled circle center
    float circleRadius; // For filled circle radius
};

// --- Global Variables ---
std::vector<Stroke> strokes;
Stroke currentStroke;
float currentColor[3] = {0.0f, 0.0f, 0.0f}; // Active drawing color
float customColor[3] = {0.0f, 0.0f, 0.0f}; // RGB slider state
float brushSize = 3.0f;
float eraserSize = 10.0f;
int currentTool = 0; // 0=brush, 1=eraser, 2=rectangle, 3=circle, 4=line, 5=fill
bool isDrawing = false;
bool isDraggingBrushSlider = false;
bool isDraggingEraserSlider = false;
bool isDraggingColorSliderR = false; // New flag for Red color slider dragging
bool isDraggingColorSliderG = false; // New flag for Green color slider dragging
bool isDraggingColorSliderB = false; // New flag for Blue color slider dragging
bool isHoveringBrushSlider = false;
bool isHoveringEraserSlider = false;

Point shapeStart, shapeEnd; // For shape previews

int windowWidth = 1000, windowHeight = 700;
float uiWidth = 0.20f; // Wider sidebar for better spacing

bool showGrid = false; // Grid toggle

// Array defining the order of tools in the UI
int tools_order[] = {0, 1, 2, 3, 4, 5};
std::string toolNames[] = {"Brush", "Eraser", "Rectangle", "Circle", "Line", "Fill"};

// UI Layout Constants (in OpenGL coordinates, from -1.0 to 1.0)
const float SIDEBAR_LEFT_GL = -1.0f;
const float SIDEBAR_RIGHT_GL = -1.0f + uiWidth;
const float PADDING_X_GL = 0.025f; // Horizontal padding
const float PADDING_Y_GL = 0.035f; // Vertical padding

const float SLIDER_VERTICAL_SPACING_GL = 0.015f;
const float SECTION_PADDING_Y_GL = 0.05f;

const float BUTTON_HEIGHT_GL = 0.08f;
const float COLOR_SWATCH_SIZE_GL = 0.07f; // Slightly smaller color swatches
const float SLIDER_HEIGHT_GL = 0.03f;
const float SLIDER_THUMB_WIDTH_GL = 0.012f; // Slightly wider thumb
const float CORNER_RADIUS_GL = 0.01f; // Global corner radius for UI elements

// Adjusted vertical height for major UI labels to fix alignment
const float UI_LABEL_BLOCK_HEIGHT = 0.05f; // Adjusted for better vertical alignment and spacing

// Top Bar Constants
const float TOP_BAR_HEIGHT_GL = COLOR_SWATCH_SIZE_GL + 2 * PADDING_Y_GL;
const float CANVAS_TOP_GL = 1.0f - TOP_BAR_HEIGHT_GL;

// Status Bar Constants for defining the drawing area
const float STATUS_BAR_TEXT_SCALE = 0.003f;
const float CALCULATED_STATUS_BAR_HEIGHT_GL = (STATUS_BAR_TEXT_SCALE * 1.0f) + PADDING_Y_GL; // Text height + vertical padding for status bar
const float DRAWING_AREA_BOTTOM_GL = -1.0f + CALCULATED_STATUS_BAR_HEIGHT_GL; // The effective bottom limit for drawing

const float ICON_DRAW_SIZE_GL = 0.05f; // Standard size for tool icons

// --- Helper Functions (Coordinates & Hit Testing) ---

void screenToGL(double x, double y, float& glX, float& glY) {
    glX = (x / windowWidth) * 2.0f - 1.0f;
    glY = 1.0f - (y / windowHeight) * 2.0f;
}

bool isInSidebar(double xpos, double ypos) {
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);
    return glX >= SIDEBAR_LEFT_GL && glX < SIDEBAR_RIGHT_GL && glY < CANVAS_TOP_GL;
}

bool isInTopBar(double xpos, double ypos) {
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);
    return glY >= CANVAS_TOP_GL;
}

bool isInCanvas(float glX, float glY) {
    // Check if the GL coordinates are within the main drawing canvas area,
    // excluding the sidebar, top bar, AND the status bar area at the bottom.
    return glX >= SIDEBAR_RIGHT_GL && glX <= 1.0f && glY >= DRAWING_AREA_BOTTOM_GL && glY < CANVAS_TOP_GL;
}


// --- Drawing Primitives ---

void drawRect(float x, float y, float w, float h, float r, float g, float b, float alpha = 1.0f) {
    glColor4f(r, g, b, alpha);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawRectOutline(float x, float y, float w, float h, float r, float g, float b, float line_width = 1.0f) {
    glColor3f(r, g, b);
    glLineWidth(line_width);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawCircle(float cx, float cy, float radius, float r, float g, float b, bool filled = true, float line_width = 1.0f) {
    glColor3f(r, g, b);
    if (filled) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
    } else {
        glLineWidth(line_width);
        glBegin(GL_LINE_LOOP);
    }
    for (int i = 0; i <= 360; i += 10) {
        float angle = i * M_PI / 180.0f;
        glVertex2f(cx + radius * std::cos(angle), cy + radius * std::sin(angle));
    }
    glEnd();
}

void drawRoundedRect(float x, float y, float w, float h, float r, float g, float b, float corner_radius_gl) {
    glColor3f(r, g, b);
    int segments = 20; // Number of segments for each corner arc

    glBegin(GL_TRIANGLE_FAN);
    // Center rectangle
    glVertex2f(x + corner_radius_gl, y + corner_radius_gl);
    glVertex2f(x + w - corner_radius_gl, y + corner_radius_gl);
    glVertex2f(x + w - corner_radius_gl, y + h - corner_radius_gl);
    glVertex2f(x + corner_radius_gl, y + h - corner_radius_gl);
    glEnd();

    // Fill the straight parts
    drawRect(x + corner_radius_gl, y, w - 2 * corner_radius_gl, h, r, g, b); // Top/bottom segments
    drawRect(x, y + corner_radius_gl, w, h - 2 * corner_radius_gl, r, g, b); // Left/right segments

    // Draw corners
    float cx, cy;
    for (int i = 0; i < 4; ++i) {
        glBegin(GL_TRIANGLE_FAN);
        // Determine center for each corner
        if (i == 0) { // Bottom-Left
            cx = x + corner_radius_gl; cy = y + corner_radius_gl;
            glVertex2f(cx, cy);
            for (int j = 180; j <= 270; ++j) {
                float angle = j * M_PI / 180.0f;
                glVertex2f(cx + corner_radius_gl * cos(angle), cy + corner_radius_gl * sin(angle));
            }
        } else if (i == 1) { // Bottom-Right
            cx = x + w - corner_radius_gl; cy = y + corner_radius_gl;
            glVertex2f(cx, cy);
            for (int j = 270; j <= 360; ++j) {
                float angle = j * M_PI / 180.0f;
                glVertex2f(cx + corner_radius_gl * cos(angle), cy + corner_radius_gl * sin(angle));
            }
        } else if (i == 2) { // Top-Right
            cx = x + w - corner_radius_gl; cy = y + h - corner_radius_gl;
            glVertex2f(cx, cy);
            for (int j = 0; j <= 90; ++j) {
                float angle = j * M_PI / 180.0f;
                glVertex2f(cx + corner_radius_gl * cos(angle), cy + corner_radius_gl * sin(angle));
            }
        } else { // Top-Left
            cx = x + corner_radius_gl; cy = y + h - corner_radius_gl;
            glVertex2f(cx, cy);
            for (int j = 90; j <= 180; ++j) {
                float angle = j * M_PI / 180.0f;
                glVertex2f(cx + corner_radius_gl * cos(angle), cy + corner_radius_gl * sin(angle));
            }
        }
        glEnd();
    }
}

void drawRoundedRectOutline(float x, float y, float w, float h, float r, float g, float b, float corner_radius_gl, float line_width = 1.0f) {
    glColor3f(r, g, b);
    glLineWidth(line_width);
    int segments = 10; // Segments per quarter circle

    glBegin(GL_LINE_LOOP);
    // Top segment
    glVertex2f(x + corner_radius_gl, y + h);
    glVertex2f(x + w - corner_radius_gl, y + h);
    // Top-Right arc
    float cx = x + w - corner_radius_gl; float cy = y + h - corner_radius_gl;
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * (M_PI / 2.0f);
        glVertex2f(cx + corner_radius_gl * std::cos(angle), cy + corner_radius_gl * std::sin(angle));
    }
    // Right segment
    glVertex2f(x + w, y + corner_radius_gl);
    // Bottom-Right arc
    cx = x + w - corner_radius_gl; cy = y + corner_radius_gl;
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * (M_PI / 2.0f) + (M_PI / 2.0f);
        glVertex2f(cx + corner_radius_gl * std::cos(angle), cy + corner_radius_gl * std::sin(angle));
    }
    // Bottom segment
    glVertex2f(x + w - corner_radius_gl, y);
    glVertex2f(x + corner_radius_gl, y);
    // Bottom-Left arc
    cx = x + corner_radius_gl; cy = y + corner_radius_gl;
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * (M_PI / 2.0f) + M_PI;
        glVertex2f(cx + corner_radius_gl * std::cos(angle), cy + corner_radius_gl * std::sin(angle));
    }
    // Left segment
    glVertex2f(x, y + h - corner_radius_gl);
    // Top-Left arc
    cx = x + corner_radius_gl; cy = y + h - corner_radius_gl;
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * (M_PI / 2.0f) + (M_PI * 3.0f / 2.0f);
        glVertex2f(cx + corner_radius_gl * std::cos(angle), cy + corner_radius_gl * std::sin(angle));
    }
    glEnd();
}

void drawShadow(float x, float y, float w, float h, float r, float g, float b, float alpha, float offset_x, float offset_y, float corner_radius_gl = 0.0f) {
    if (corner_radius_gl > 0) {
        drawRoundedRect(x + offset_x, y - offset_y, w, h, r, g, b, corner_radius_gl);
    } else {
        drawRect(x + offset_x, y - offset_y, w, h, r, g, b, alpha);
    }
}


// --- UI Elements ---

void drawThemedButton(float x, float y, float w, float h, float r_bg, float g_bg, float b_bg, bool selected, bool hovered, float corner_radius_gl, bool has_shadow = true) {
    if (has_shadow) {
        drawShadow(x, y, w, h, SHADOW_R, SHADOW_G, SHADOW_B, SHADOW_ALPHA, 0.006f, 0.006f, corner_radius_gl);
    }

    float final_r = r_bg, final_g = g_bg, final_b = b_bg;
    // For color swatches, if selected, we keep their color and apply outline.
    // For other buttons, the selected state changes their background color.
    if (!selected) { // Apply hover effect only if not selected
        if (hovered) {
            final_r = BUTTON_HOVER_R; final_g = BUTTON_HOVER_G; final_b = BUTTON_HOVER_B;
        }
    } else { // If selected, keep original color, and indicate selection with outline
        // The final_r,g,b are already r_bg,g_bg,b_bg from initialization, which is correct for selected color swatches
    }

    drawRoundedRect(x, y, w, h, final_r, final_g, final_b, corner_radius_gl);
    // Outline for selected state (dark text color)
    if (selected) {
        drawRoundedRectOutline(x, y, w, h, TEXT_R, TEXT_G, TEXT_B, corner_radius_gl, 1.5f);
    } else { // Default border color for non-selected
        drawRoundedRectOutline(x, y, w, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);
    }
}

// UI element: Draws simple text using line segments (for labels)
void drawText(float x, float y, const char* text, float r, float g, float b, float scale = 0.005f, float line_width = 1.5f) {
    glColor3f(r, g, b);
    glLineWidth(line_width); // Line thickness for text characters
    glPushMatrix(); // Save current transformation matrix
    glTranslatef(x, y, 0.0f); // Move to the text's starting position
    glScalef(scale, scale, 1.0f); // Scale characters

    // Basic character drawing using GL_LINES (simplified glyphs from previous version)
    // (Retained for consistency with previous text rendering, not a full font library)
    for (int i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        glBegin(GL_LINES);
        switch (c) {
            case 'A': glVertex2f(0,0); glVertex2f(0.5,1); glVertex2f(0.5,1); glVertex2f(1,0); glVertex2f(0,0.5); glVertex2f(1,0.5); break;
            case 'B': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(0.7,0.9); glVertex2f(0.7,0.9); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(0.7,0.1); glVertex2f(0.7,0.1); glVertex2f(0,0); break;
            case 'C': glVertex2f(1,1); glVertex2f(0,0.8); glVertex2f(0,0.8); glVertex2f(0,0.2); glVertex2f(0,0.2); glVertex2f(1,0); break;
            case 'D': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(0.7,0.8); glVertex2f(0.7,0.8); glVertex2f(0.7,0.2); glVertex2f(0.7,0.2); glVertex2f(0,0); break;
            case 'E': glVertex2f(1,1); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(0,0); glVertex2f(0,0); glVertex2f(1,0); glVertex2f(0,0.5); glVertex2f(0.7,0.5); break;
            case 'F': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(1,1); glVertex2f(0,0.5); glVertex2f(0.7,0.5); break;
            case 'G': glVertex2f(1,1); glVertex2f(0,0.8); glVertex2f(0,0.8); glVertex2f(0,0.2); glVertex2f(0,0.2); glVertex2f(1,0); glVertex2f(1,0); glVertex2f(1,0.5); glVertex2f(0.5,0.5); glVertex2f(1,0.5); break;
            case 'H': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(1,0); glVertex2f(1,1); glVertex2f(0,0.5); glVertex2f(1,0.5); break;
            case 'I': glVertex2f(0,1); glVertex2f(1,1); glVertex2f(0.5,1); glVertex2f(0.5,0); glVertex2f(0,0); glVertex2f(1,0); break;
            case 'J': glVertex2f(1,1); glVertex2f(1,0.5); glVertex2f(1,0.5); glVertex2f(0.5,0); glVertex2f(0.5,0); glVertex2f(0,0.2); break;
            case 'K': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(1,1); glVertex2f(0,0.5); glVertex2f(1,0); glVertex2f(0,0.5); break;
            case 'L': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,0); glVertex2f(1,0); break;
            case 'M': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(1,1); glVertex2f(1,1); glVertex2f(1,0); break;
            case 'N': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(1,0); glVertex2f(1,0); glVertex2f(1,1); break;
            case 'O': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(1,1); glVertex2f(1,1); glVertex2f(1,0); glVertex2f(1,0); glVertex2f(0,0); break;
            case 'P': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(1,1); glVertex2f(1,1); glVertex2f(1,0.5); glVertex2f(1,0.5); glVertex2f(0,0.5); break;
            case 'Q': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(1,1); glVertex2f(1,1); glVertex2f(1,0); glVertex2f(1,0); glVertex2f(0,0); glVertex2f(0.5,0.5); glVertex2f(1,0); break;
            case 'R': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(1,1); glVertex2f(1,1); glVertex2f(1,0.5); glVertex2f(1,0.5); glVertex2f(0,0.5); glVertex2f(0.5,0.5); glVertex2f(1,0); break;
            case 'S': glVertex2f(1,1); glVertex2f(0,1); glVertex2f(0,1); glVertex2f(0,0.5); glVertex2f(0,0.5); glVertex2f(1,0.5); glVertex2f(1,0.5); glVertex2f(1,0); glVertex2f(1,0); glVertex2f(0,0); break;
            case 'T': glVertex2f(0,1); glVertex2f(1,1); glVertex2f(0.5,1); glVertex2f(0.5,0); break;
            case 'U': glVertex2f(0,1); glVertex2f(0,0); glVertex2f(0,0); glVertex2f(1,0); glVertex2f(1,0); glVertex2f(1,1); break;
            case 'V': glVertex2f(0,1); glVertex2f(0.5,0); glVertex2f(0.5,0); glVertex2f(1,1); break;
            case 'W': glVertex2f(0,1); glVertex2f(0.25,0); glVertex2f(0.25,0); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(0.75,0); glVertex2f(0.75,0); glVertex2f(1,1); break;
            case 'X': glVertex2f(0,1); glVertex2f(1,0); glVertex2f(0,0); glVertex2f(1,1); break;
            case 'Y': glVertex2f(0,1); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(1,1); glVertex2f(0.5,0.5); glVertex2f(0.5,0); break;
            case 'Z': glVertex2f(0,1); glVertex2f(1,1); glVertex2f(1,1); glVertex2f(0,0); glVertex2f(0,0); glVertex2f(1,0); break;
            // Lowercase characters (simplified)
            case 'a': glVertex2f(0,0); glVertex2f(0.5,0); glVertex2f(0.5,0.5); glVertex2f(0,0.5); glVertex2f(0.5,0.5); glVertex2f(0.5,1); break;
            case 'b': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,0.5); glVertex2f(0.5,0.75); glVertex2f(0.5,0.75); glVertex2f(0,0); break;
            case 'c': glVertex2f(0.5,1); glVertex2f(0,0.75); glVertex2f(0,0.75); glVertex2f(0,0.25); glVertex2f(0,0.25); glVertex2f(0.5,0); break;
            case 'd': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,0); glVertex2f(0.5,0.25); glVertex2f(0.5,0.25); glVertex2f(0.5,0.75); glVertex2f(0.5,0.75); glVertex2f(0,1); break;
            case 'e': glVertex2f(0,0.5); glVertex2f(1,0.5); glVertex2f(1,0.5); glVertex2f(0.5,1); glVertex2f(0.5,1); glVertex2f(0,0.75); glVertex2f(0,0.75); glVertex2f(0,0.25); glVertex2f(0,0.25); glVertex2f(0.5,0); glVertex2f(0.5,0); glVertex2f(1,0); break;
            case 'f': glVertex2f(0.5,0); glVertex2f(0.5,1); glVertex2f(0,0.75); glVertex2f(1,0.75); break;
            case 'g': glVertex2f(0.5,1); glVertex2f(0,0.75); glVertex2f(0,0.75); glVertex2f(0,0.25); glVertex2f(0,0.25); glVertex2f(0.5,0); glVertex2f(0.5,0); glVertex2f(0.5,-0.5); glVertex2f(0.5,-0.5); glVertex2f(1,-0.25); break;
            case 'h': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(0,0.5); glVertex2f(1,0.5); glVertex2f(1,0.5); glVertex2f(1,0); break;
            case 'i': glVertex2f(0.5,0); glVertex2f(0.5,0.75); glVertex2f(0.5,1); glVertex2f(0.5,1); break;
            case 'j': glVertex2f(1,0.75); glVertex2f(1,0); glVertex2f(1,0); glVertex2f(0.5,-0.25); glVertex2f(0.5,-0.25); glVertex2f(0,0); break;
            case 'k': glVertex2f(0,0); glVertex2f(0,1); glVertex2f(1,1); glVertex2f(0,0.5); glVertex2f(1,0); glVertex2f(0,0.5); break;
            case 'l': glVertex2f(0.5,0); glVertex2f(0.5,1); break;
            case 'm': glVertex2f(0,0); glVertex2f(0,0.5); glVertex2f(0,0.5); glVertex2f(0.5,1); glVertex2f(0.5,1); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(1,1); glVertex2f(1,1); glVertex2f(1,0.5); break;
            case 'n': glVertex2f(0,0); glVertex2f(0,0.5); glVertex2f(0,0.5); glVertex2f(0.5,1); glVertex2f(0.5,1); glVertex2f(1,0.5); break;
            case 'o': glVertex2f(0,0.5); glVertex2f(0,0); glVertex2f(0,0); glVertex2f(0.5,0); glVertex2f(0.5,0); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(0,0.5); break;
            case 'p': glVertex2f(0,0); glVertex2f(0,-0.5); glVertex2f(0,0); glVertex2f(0.5,0); glVertex2f(0.5,0); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(0,0.5); break;
            case 'q': glVertex2f(0,0); glVertex2f(0,-0.5); glVertex2f(0,0); glVertex2f(0.5,0); glVertex2f(0.5,0); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(0,0.5); glVertex2f(0.5,-0.25); glVertex2f(1,-0.5); break;
            case 'r': glVertex2f(0,0); glVertex2f(0,0.5); glVertex2f(0,0.5); glVertex2f(0.5,1); break;
            case 's': glVertex2f(0.5,1); glVertex2f(0,0.75); glVertex2f(0,0.75); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(0,0.25); glVertex2f(0,0.25); glVertex2f(0.5,0); break;
            case 't': glVertex2f(0.5,0); glVertex2f(0.5,1); glVertex2f(0.25,0.75); glVertex2f(0.75,0.75); break;
            case 'u': glVertex2f(0,1); glVertex2f(0,0.25); glVertex2f(0,0.25); glVertex2f(0.5,0); glVertex2f(0.5,0); glVertex2f(0.5,1); break;
            case 'v': glVertex2f(0,1); glVertex2f(0.5,0); glVertex2f(0.5,0); glVertex2f(1,1); break;
            case 'w': glVertex2f(0,1); glVertex2f(0.25,0); glVertex2f(0.25,0); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(0.75,0); glVertex2f(0.75,0); glVertex2f(1,1); break;
            case 'x': glVertex2f(0,1); glVertex2f(1,0); glVertex2f(0,0); glVertex2f(1,1); break;
            case 'y': glVertex2f(0,1); glVertex2f(0.5,0.5); glVertex2f(0.5,0.5); glVertex2f(1,1); glVertex2f(0.5,0.5); glVertex2f(0.5,0); glVertex2f(0.5,0); glVertex2f(1,-0.25); break;
            case 'z': glVertex2f(0,1); glVertex2f(1,1); glVertex2f(1,1); glVertex2f(0,0); glVertex2f(0,0); glVertex2f(1,0); break;
            case ' ': glTranslatef(0.8f, 0, 0); break;
            case '.': glVertex2f(0.5,0); glVertex2f(0.5,0.1); break;
            case '!': glVertex2f(0.5,0); glVertex2f(0.5,0.75); glVertex2f(0.5,1); glVertex2f(0.5,1); break;
        }
        glEnd();
        glTranslatef(1.2f, 0, 0);
    }
    glPopMatrix();
}

// Helper: Draws a stylized pencil icon
void drawPencilIcon(float cx, float cy, float size, bool selected) {
    float half_size = size / 2.0f;
    float body_width = half_size * 0.4f;
    float body_height = half_size * 0.8f;
    float tip_height = half_size * 0.4f;
    float eraser_height = half_size * 0.2f;

    // Pencil body (yellowish color)
    glColor3f(0.9f, 0.8f, 0.2f);
    drawRect(cx - body_width/2, cy - body_height/2 + eraser_height/2, body_width, body_height, 0.9f, 0.8f, 0.2f);

    // Pencil tip (dark grey triangle)
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - body_width/2, cy + body_height/2 + eraser_height/2);
    glVertex2f(cx + body_width/2, cy + body_height/2 + eraser_height/2);
    glVertex2f(cx, cy + body_height/2 + tip_height + eraser_height/2);
    glEnd();

    // Pencil eraser (pink rectangle)
    glColor3f(0.9f, 0.6f, 0.7f);
    drawRect(cx - body_width/2, cy - body_height/2 - eraser_height/2, body_width, eraser_height, 0.9f, 0.6f, 0.7f);

    // Outline for the whole pencil
    glColor3f(0.2f, 0.2f, 0.2f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx - body_width/2, cy - body_height/2 - eraser_height/2);
    glVertex2f(cx + body_width/2, cy - body_height/2 - eraser_height/2);
    glVertex2f(cx + body_width/2, cy + body_height/2 + eraser_height/2);
    glVertex2f(cx, cy + body_height/2 + tip_height + eraser_height/2); // Tip top
    glVertex2f(cx - body_width/2, cy + body_height/2 + eraser_height/2);
    glEnd();
}

// Helper: Draws a stylized eraser icon
void drawEraserIcon(float cx, float cy, float size, bool selected) {
    float half_size = size / 2.0f;
    float eraser_width = half_size * 0.8f;
    float eraser_height = half_size * 0.5f;
    float tip_height = half_size * 0.2f;

    // Main eraser body (light grey)
    glColor3f(0.7f, 0.7f, 0.7f);
    drawRect(cx - eraser_width/2, cy - eraser_height/2, eraser_width, eraser_height, 0.7f, 0.7f, 0.7f);

    // Small contrasting tip (pinkish)
    glColor3f(0.9f, 0.5f, 0.5f);
    drawRect(cx - eraser_width/2, cy + eraser_height/2, eraser_width, tip_height, 0.9f, 0.5f, 0.5f);

    // Outline for the eraser
    glColor3f(0.2f, 0.2f, 0.2f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx - eraser_width/2, cy - eraser_height/2);
    glVertex2f(cx + eraser_width/2, cy - eraser_height/2);
    glVertex2f(cx + eraser_width/2, cy + eraser_height/2 + tip_height);
    glVertex2f(cx - eraser_width/2, cy + eraser_height/2 + tip_height);
    glEnd();
}

// UI element: Draws the preset color palette in the top bar
void drawPresetColorPalette(double mouseX_gl, double mouseY_gl) {
    float colors[][3] = {
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}
    };
    float swatch_y = 1.0f - PADDING_Y_GL - COLOR_SWATCH_SIZE_GL;
    float swatch_start_x = -1.0f + PADDING_X_GL;
    float swatch_spacing_x = COLOR_SWATCH_SIZE_GL + PADDING_X_GL / 2.0f;

    for (int i = 0; i < 8; i++) {
        float x = swatch_start_x + i * swatch_spacing_x;
        bool selected = (std::abs(currentColor[0] - colors[i][0]) < 0.01f &&
                         std::abs(currentColor[1] - colors[i][1]) < 0.01f &&
                         std::abs(currentColor[2] - colors[i][2]) < 0.01f);
        bool hovered = (mouseX_gl >= x && mouseX_gl <= x + COLOR_SWATCH_SIZE_GL &&
                        mouseY_gl >= swatch_y && mouseY_gl <= swatch_y + COLOR_SWATCH_SIZE_GL);

        // Pass the actual color as background, selected state will draw an outline
        drawThemedButton(x, swatch_y, COLOR_SWATCH_SIZE_GL, COLOR_SWATCH_SIZE_GL, colors[i][0], colors[i][1], colors[i][2], selected, hovered, CORNER_RADIUS_GL);
    }
}

// Helper struct to return calculated Y positions for main UI sections
struct SectionYPositions {
    float toolsSectionTopY;
    float colorsSectionTopY;
    float sizesSectionTopY;
};

// Function to calculate the top Y position of each major UI section
SectionYPositions getSectionYPositions() {
    float y_accumulated_offset_from_canvas_top = PADDING_Y_GL; // Initial padding from top of canvas area

    // --- TOOLS section calculation ---
    float tools_top_y = CANVAS_TOP_GL - y_accumulated_offset_from_canvas_top;
    y_accumulated_offset_from_canvas_top += (UI_LABEL_BLOCK_HEIGHT + PADDING_Y_GL); // "TOOLS" label block
    y_accumulated_offset_from_canvas_top += (6 * BUTTON_HEIGHT_GL + 5 * PADDING_Y_GL); // 6 tool buttons + 5 spaces
    y_accumulated_offset_from_canvas_top += SECTION_PADDING_Y_GL; // Padding after tools section
    y_accumulated_offset_from_canvas_top += SECTION_PADDING_Y_GL / 2.0f; // Separator

    // --- COLORS section calculation ---
    float colors_top_y = CANVAS_TOP_GL - y_accumulated_offset_from_canvas_top;
    y_accumulated_offset_from_canvas_top += (UI_LABEL_BLOCK_HEIGHT + PADDING_Y_GL); // "COLORS" label block
    y_accumulated_offset_from_canvas_top += (SLIDER_HEIGHT_GL * 1.5f + PADDING_Y_GL); // Color preview
    y_accumulated_offset_from_canvas_top += (3 * SLIDER_HEIGHT_GL + 2 * SLIDER_VERTICAL_SPACING_GL); // 3 RGB sliders + 2 spaces
    y_accumulated_offset_from_canvas_top += SECTION_PADDING_Y_GL; // Padding after colors section
    y_accumulated_offset_from_canvas_top += SECTION_PADDING_Y_GL / 2.0f; // Separator

    // --- SIZES section calculation ---
    float sizes_top_y = CANVAS_TOP_GL - y_accumulated_offset_from_canvas_top;
    // No need to accumulate further, as this is the last section we define layout for here.

    return {tools_top_y, colors_top_y, sizes_top_y};
}


// Helper struct to return calculated Y positions for individual color sliders
struct ColorSliderYPositions {
    float rSliderBottomY;
    float gSliderBottomY;
    float bSliderBottomY;
};

// Helper function to consistently calculate the Y positions of the color sliders
ColorSliderYPositions getIndividualColorSliderYPositions(float colorsSectionTopY) {
    float current_y_in_section = colorsSectionTopY; // Start from the top of the Colors section

    current_y_in_section -= (PADDING_Y_GL + UI_LABEL_BLOCK_HEIGHT + PADDING_Y_GL); // COLORS label + padding
    current_y_in_section -= (SLIDER_HEIGHT_GL * 1.5f + PADDING_Y_GL); // Color preview + padding

    float r_slider_bottom_y = current_y_in_section - SLIDER_HEIGHT_GL;
    current_y_in_section = r_slider_bottom_y - SLIDER_VERTICAL_SPACING_GL; 

    float g_slider_bottom_y = current_y_in_section - SLIDER_HEIGHT_GL;
    current_y_in_section = g_slider_bottom_y - SLIDER_VERTICAL_SPACING_GL;

    float b_slider_bottom_y = current_y_in_section - SLIDER_HEIGHT_GL;

    return {r_slider_bottom_y, g_slider_bottom_y, b_slider_bottom_y};
}

// Helper struct to return calculated Y positions for individual size sliders
struct SizeSliderYPositions {
    float brushSliderBottomY;
    float eraserSliderBottomY;
    float brushLabelTopY; // For placing label
    float eraserLabelTopY; // For placing label
};

// Helper function to consistently calculate the Y positions of the size sliders
SizeSliderYPositions getIndividualSizeSliderYPositions(float sizesSectionTopY) {
    float current_y_in_section = sizesSectionTopY; // Start from the top of the Sizes section

    float label_text_scale_size = 0.009f * 1.5f; // Increased scale for "SIZE" text
    float sub_label_text_scale = 0.007f * 1.5f; // Increased scale for sub-labels

    current_y_in_section -= (PADDING_Y_GL + UI_LABEL_BLOCK_HEIGHT + PADDING_Y_GL); // SIZE label + padding

    // --- Pencil Size Label & Slider ---
    float pencil_label_top_y = current_y_in_section - PADDING_Y_GL; // Position for label's top
    float brush_slider_bottom_y = pencil_label_top_y - sub_label_text_scale - PADDING_Y_GL - SLIDER_HEIGHT_GL;

    // --- Eraser Size Label & Slider ---
    float eraser_label_top_y = brush_slider_bottom_y - SLIDER_VERTICAL_SPACING_GL - PADDING_Y_GL; // Position for label's top
    float eraser_slider_bottom_y = eraser_label_top_y - sub_label_text_scale - PADDING_Y_GL - SLIDER_HEIGHT_GL;
    
    return {brush_slider_bottom_y, eraser_slider_bottom_y, pencil_label_top_y, eraser_label_top_y};
}


// UI element: Draws the tool selection buttons in the sidebar
// This function now takes its starting_y and does not return current_y.
void drawToolButtons(float toolsSectionTopY, double mouseX_gl, double mouseY_gl) {
    float start_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float btn_w = uiWidth - 2 * PADDING_X_GL;
    float btn_h = BUTTON_HEIGHT_GL;
    float row_spacing = btn_h + PADDING_Y_GL;

    float current_y_for_drawing = toolsSectionTopY; // Start from the top of the Tools section

    drawText(start_x, current_y_for_drawing - PADDING_Y_GL - UI_LABEL_BLOCK_HEIGHT, "TOOLS", TEXT_R, TEXT_G, TEXT_B, 0.009f, 2.0f);
    current_y_for_drawing -= (PADDING_Y_GL + UI_LABEL_BLOCK_HEIGHT + PADDING_Y_GL); // Position after label

    for (int i = 0; i < 6; i++) {
        int tool_idx = tools_order[i];
        float x_button_area = start_x;
        float y_button_area = current_y_for_drawing - i * row_spacing; // Correctly place each button

        bool selected = (currentTool == tool_idx);
        bool hovered = (mouseX_gl >= x_button_area && mouseX_gl <= x_button_area + btn_w &&
                        mouseY_gl >= y_button_area && mouseY_gl <= y_button_area + btn_h);

        drawThemedButton(x_button_area, y_button_area, btn_w, btn_h, BUTTON_DEFAULT_R, BUTTON_DEFAULT_G, BUTTON_DEFAULT_B, selected, hovered, CORNER_RADIUS_GL);

        float iconX = x_button_area + btn_w / 2.0f;
        float iconY = y_button_area + btn_h / 2.0f;

        glColor3f(TEXT_R, TEXT_G, TEXT_B);
        glLineWidth(2.0f);

        switch (tool_idx) {
            case 0: // Brush
                drawPencilIcon(iconX, iconY, ICON_DRAW_SIZE_GL, selected);
                break;
            case 1: // Eraser
                drawEraserIcon(iconX, iconY, ICON_DRAW_SIZE_GL, selected);
                break;
            case 2: // Rectangle - now a filled rectangle with an outline
                {
                    float rect_size = ICON_DRAW_SIZE_GL * 0.7f; // Smaller filled rectangle
                    drawRoundedRect(iconX - rect_size/2, iconY - rect_size/2, rect_size, rect_size, TEXT_R * 1.5f, TEXT_G * 1.5f, TEXT_B * 1.5f, CORNER_RADIUS_GL * 1.5f); // Light fill
                    drawRoundedRectOutline(iconX - ICON_DRAW_SIZE_GL/2, iconY - ICON_DRAW_SIZE_GL/2, ICON_DRAW_SIZE_GL, ICON_DRAW_SIZE_GL, TEXT_R, TEXT_G, TEXT_B, CORNER_RADIUS_GL * 2.0f); // Outer outline
                }
                break;
            case 3: // Circle
                drawCircle(iconX, iconY, ICON_DRAW_SIZE_GL/2, TEXT_R, TEXT_G, TEXT_B, false, 2.0f);
                break;
            case 4: // Line
                glBegin(GL_LINES);
                glVertex2f(iconX - ICON_DRAW_SIZE_GL/2, iconY - ICON_DRAW_SIZE_GL/2);
                glVertex2f(iconX + ICON_DRAW_SIZE_GL/2, iconY + ICON_DRAW_SIZE_GL/2);
                glEnd();
                break;
            case 5: // Fill
                drawRoundedRect(iconX - ICON_DRAW_SIZE_GL/2, iconY - ICON_DRAW_SIZE_GL/2, ICON_DRAW_SIZE_GL, ICON_DRAW_SIZE_GL, currentColor[0], currentColor[1], currentColor[2], CORNER_RADIUS_GL * 2.0f);
                drawRoundedRectOutline(iconX - ICON_DRAW_SIZE_GL/2, iconY - ICON_DRAW_SIZE_GL/2, ICON_DRAW_SIZE_GL, ICON_DRAW_SIZE_GL, TEXT_R, TEXT_G, TEXT_B, CORNER_RADIUS_GL * 2.0f);
                break;
        }
    }
}

// UI element: Draws the RGB color sliders in the sidebar
// This function now takes its starting_y and does not return current_y.
void drawColorSlidersSidebar(float colorsSectionTopY, double mouseX_gl, double mouseY_gl) {
    float x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float w = uiWidth - 2 * PADDING_X_GL;
    float h = SLIDER_HEIGHT_GL;

    ColorSliderYPositions slider_y_pos = getIndividualColorSliderYPositions(colorsSectionTopY); // Get precise slider Ys

    float current_y_for_drawing = colorsSectionTopY; // Start from the top of the Colors section

    drawText(x, current_y_for_drawing - PADDING_Y_GL - UI_LABEL_BLOCK_HEIGHT, "COLORS", TEXT_R, TEXT_G, TEXT_B, 0.009f, 2.0f);
    current_y_for_drawing -= (PADDING_Y_GL + UI_LABEL_BLOCK_HEIGHT + PADDING_Y_GL); // Position after label

    // Current Color Preview
    float preview_h = SLIDER_HEIGHT_GL * 1.5f;
    drawRoundedRect(x, current_y_for_drawing - preview_h, w, preview_h, customColor[0], customColor[1], customColor[2], CORNER_RADIUS_GL);
    drawRoundedRectOutline(x, current_y_for_drawing - preview_h, w, preview_h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);
    current_y_for_drawing -= (preview_h + PADDING_Y_GL); // Position after preview

    // Red (R) Slider
    float slider_top_y_r = slider_y_pos.rSliderBottomY + h; // Use pre-calculated bottom to get top
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_y_pos.rSliderBottomY);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(x + w, slider_y_pos.rSliderBottomY);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(x + w, slider_y_pos.rSliderBottomY + h);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_y_pos.rSliderBottomY + h);
    glEnd();
    float thumb_x_r = x + customColor[0] * (w - SLIDER_THUMB_WIDTH_GL);
    bool hovered_r = (mouseX_gl >= x && mouseX_gl <= x + w && mouseY_gl >= slider_y_pos.rSliderBottomY && mouseY_gl <= slider_y_pos.rSliderBottomY + h);
    drawRoundedRect(thumb_x_r, slider_y_pos.rSliderBottomY, SLIDER_THUMB_WIDTH_GL, h, hovered_r ? BUTTON_HOVER_R : BUTTON_DEFAULT_R, hovered_r ? BUTTON_HOVER_G : BUTTON_DEFAULT_G, hovered_r ? BUTTON_HOVER_B : BUTTON_DEFAULT_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(thumb_x_r, slider_y_pos.rSliderBottomY, SLIDER_THUMB_WIDTH_GL, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(x, slider_y_pos.rSliderBottomY, w, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);

    // Green (G) Slider
    float slider_top_y_g = slider_y_pos.gSliderBottomY + h;
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_y_pos.gSliderBottomY);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(x + w, slider_y_pos.gSliderBottomY);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(x + w, slider_y_pos.gSliderBottomY + h);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_y_pos.gSliderBottomY + h);
    glEnd();
    float thumb_x_g = x + customColor[1] * (w - SLIDER_THUMB_WIDTH_GL);
    bool hovered_g = (mouseX_gl >= x && mouseX_gl <= x + w && mouseY_gl >= slider_y_pos.gSliderBottomY && mouseY_gl <= slider_y_pos.gSliderBottomY + h);
    drawRoundedRect(thumb_x_g, slider_y_pos.gSliderBottomY, SLIDER_THUMB_WIDTH_GL, h, hovered_g ? BUTTON_HOVER_R : BUTTON_DEFAULT_R, hovered_g ? BUTTON_HOVER_G : BUTTON_DEFAULT_G, hovered_g ? BUTTON_HOVER_B : BUTTON_DEFAULT_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(thumb_x_g, slider_y_pos.gSliderBottomY, SLIDER_THUMB_WIDTH_GL, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(x, slider_y_pos.gSliderBottomY, w, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);

    // Blue (B) Slider
    float slider_top_y_b = slider_y_pos.bSliderBottomY + h;
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_y_pos.bSliderBottomY);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(x + w, slider_y_pos.bSliderBottomY);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(x + w, slider_y_pos.bSliderBottomY + h);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_y_pos.bSliderBottomY + h);
    glEnd();
    float thumb_x_b = x + customColor[2] * (w - SLIDER_THUMB_WIDTH_GL);
    bool hovered_b = (mouseX_gl >= x && mouseX_gl <= x + w && mouseY_gl >= slider_y_pos.bSliderBottomY && mouseY_gl <= slider_y_pos.bSliderBottomY + h);
    drawRoundedRect(thumb_x_b, slider_y_pos.bSliderBottomY, SLIDER_THUMB_WIDTH_GL, h, hovered_b ? BUTTON_HOVER_R : BUTTON_DEFAULT_R, hovered_b ? BUTTON_HOVER_G : BUTTON_DEFAULT_G, hovered_b ? BUTTON_HOVER_B : BUTTON_DEFAULT_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(thumb_x_b, slider_y_pos.bSliderBottomY, SLIDER_THUMB_WIDTH_GL, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(x, slider_y_pos.bSliderBottomY, w, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);
}

// UI element: Draws the brush and eraser size sliders in the sidebar
// This function now takes its starting_y and does not return current_y.
void drawSizeSelectorsSidebar(float sizesSectionTopY, double mouseX_gl, double mouseY_gl) {
    float x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float w = uiWidth - 2 * PADDING_X_GL;
    float h = SLIDER_HEIGHT_GL;
    float label_text_scale = 0.009f * 1.5f; // Increased scale for "SIZE" text
    float sub_label_text_scale = 0.007f * 1.5f; // Increased scale for sub-labels

    SizeSliderYPositions slider_y_pos = getIndividualSizeSliderYPositions(sizesSectionTopY); // Get precise slider Ys

    float current_y_for_drawing = sizesSectionTopY; // Start from the top of the Sizes section

    drawText(x, current_y_for_drawing - PADDING_Y_GL - UI_LABEL_BLOCK_HEIGHT, "SIZE", TEXT_R, TEXT_G, TEXT_B, label_text_scale, 2.0f);
    current_y_for_drawing -= (PADDING_Y_GL + UI_LABEL_BLOCK_HEIGHT + PADDING_Y_GL); // Position after label

    // --- Pencil Size Label & Slider ---
    drawText(x, slider_y_pos.brushLabelTopY - sub_label_text_scale/2.0f, "Pencil Size", TEXT_R, TEXT_G, TEXT_B, sub_label_text_scale, 2.0f); 

    float slider_top_y_brush = slider_y_pos.brushSliderBottomY + h;
    bool hovered_brush_track = (mouseX_gl >= x && mouseX_gl <= x + w && mouseY_gl >= slider_y_pos.brushSliderBottomY && mouseY_gl <= slider_y_pos.brushSliderBottomY + h);
    drawRoundedRect(x, slider_y_pos.brushSliderBottomY, w, h, BUTTON_DEFAULT_R, BUTTON_DEFAULT_G, BUTTON_DEFAULT_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(x, slider_y_pos.brushSliderBottomY, w, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);
    
    float thumb_x_brush = x + (brushSize - 1.0f) / 19.0f * (w - SLIDER_THUMB_WIDTH_GL);
    thumb_x_brush = std::max(x, std::min(x + w - SLIDER_THUMB_WIDTH_GL, thumb_x_brush));

    bool hovered_brush_thumb = (mouseX_gl >= thumb_x_brush && mouseX_gl <= thumb_x_brush + SLIDER_THUMB_WIDTH_GL &&
                                mouseY_gl >= slider_y_pos.brushSliderBottomY && mouseY_gl <= slider_y_pos.brushSliderBottomY + h);
    isHoveringBrushSlider = hovered_brush_track; 
    drawRoundedRect(thumb_x_brush, slider_y_pos.brushSliderBottomY, SLIDER_THUMB_WIDTH_GL, h, hovered_brush_thumb ? ACCENT_R : TEXT_R, hovered_brush_thumb ? ACCENT_G : TEXT_G, hovered_brush_thumb ? ACCENT_B : TEXT_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(thumb_x_brush, slider_y_pos.brushSliderBottomY, SLIDER_THUMB_WIDTH_GL, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);
    
    // --- Eraser Size Label & Slider ---
    drawText(x, slider_y_pos.eraserLabelTopY - sub_label_text_scale/2.0f, "Eraser Size", TEXT_R, TEXT_G, TEXT_B, sub_label_text_scale, 2.0f); 

    float slider_top_y_eraser = slider_y_pos.eraserSliderBottomY + h;
    bool hovered_eraser_track = (mouseX_gl >= x && mouseX_gl <= x + w && mouseY_gl >= slider_y_pos.eraserSliderBottomY && mouseY_gl <= slider_y_pos.eraserSliderBottomY + h);
    drawRoundedRect(x, slider_y_pos.eraserSliderBottomY, w, h, BUTTON_DEFAULT_R, BUTTON_DEFAULT_G, BUTTON_DEFAULT_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(x, slider_y_pos.eraserSliderBottomY, w, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);

    float thumb_x_eraser = x + (eraserSize - 1.0f) / 19.0f * (w - SLIDER_THUMB_WIDTH_GL);
    thumb_x_eraser = std::max(x, std::min(x + w - SLIDER_THUMB_WIDTH_GL, thumb_x_eraser));

    bool hovered_eraser_thumb = (mouseX_gl >= thumb_x_eraser && mouseX_gl <= thumb_x_eraser + SLIDER_THUMB_WIDTH_GL &&
                                 mouseY_gl >= slider_y_pos.eraserSliderBottomY && mouseY_gl <= slider_y_pos.eraserSliderBottomY + h);
    isHoveringEraserSlider = hovered_eraser_track; 
    drawRoundedRect(thumb_x_eraser, slider_y_pos.eraserSliderBottomY, SLIDER_THUMB_WIDTH_GL, h, hovered_eraser_thumb ? ACCENT_R : TEXT_R, hovered_eraser_thumb ? ACCENT_G : TEXT_G, hovered_eraser_thumb ? ACCENT_B : TEXT_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(thumb_x_eraser, slider_y_pos.eraserSliderBottomY, SLIDER_THUMB_WIDTH_GL, h, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);
}

// UI element: Draws all buttons in the top horizontal bar (Clear, Save)
void drawTopBarButtons(double mouseX_gl, double mouseY_gl) {
    // Clear Button
    float clear_btn_w = BUTTON_HEIGHT_GL * 1.5f;
    float clear_btn_h = BUTTON_HEIGHT_GL;
    float clear_btn_x = 1.0f - PADDING_X_GL - clear_btn_w;
    float clear_btn_y = 1.0f - PADDING_Y_GL - clear_btn_h;

    bool hovered_clear = (mouseX_gl >= clear_btn_x && mouseX_gl <= clear_btn_x + clear_btn_w && mouseY_gl >= clear_btn_y && mouseY_gl <= clear_btn_y + clear_btn_h);
    drawThemedButton(clear_btn_x, clear_btn_y, clear_btn_w, clear_btn_h, CLEAR_BUTTON_R, CLEAR_BUTTON_G, CLEAR_BUTTON_B, false, hovered_clear, CORNER_RADIUS_GL);
    glColor3f(1.0f, 1.0f, 1.0f); // White color for the "X" icon
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2f(clear_btn_x + clear_btn_w * 0.25f, clear_btn_y + clear_btn_h * 0.25f);
    glVertex2f(clear_btn_x + clear_btn_w * 0.75f, clear_btn_y + clear_btn_h * 0.75f);
    glVertex2f(clear_btn_x + clear_btn_w * 0.25f, clear_btn_y + clear_btn_h * 0.75f);
    glVertex2f(clear_btn_x + clear_btn_w * 0.75f, clear_btn_y + clear_btn_h * 0.25f);
    glEnd();

    // Save Button
    float save_btn_w = BUTTON_HEIGHT_GL * 1.5f;
    float save_btn_h = BUTTON_HEIGHT_GL;
    float save_btn_x = clear_btn_x - PADDING_X_GL / 2.0f - save_btn_w; // Position to the left of Clear button
    float save_btn_y = clear_btn_y; // Same vertical position

    bool hovered_save = (mouseX_gl >= save_btn_x && mouseX_gl <= save_btn_x + save_btn_w && mouseY_gl >= save_btn_y && mouseY_gl <= save_btn_y + save_btn_h);
    drawThemedButton(save_btn_x, save_btn_y, save_btn_w, save_btn_h, BUTTON_DEFAULT_R, BUTTON_DEFAULT_G, BUTTON_DEFAULT_B, false, hovered_save, CORNER_RADIUS_GL);
    // Draw "SAVE" text slightly adjusted to center it visually
    drawText(save_btn_x + PADDING_X_GL / 2.0f, save_btn_y + save_btn_h / 2.0f - 0.007f, "SAVE", TEXT_R, TEXT_G, TEXT_B, 0.006f, 1.5f);
}

// UI element: Draws the status bar at the bottom right of the canvas
void drawStatusBar() {
    std::string status_text = "Tool: " + toolNames[currentTool];
    std::stringstream ss;
    ss.precision(1); // Set precision for floating point output
    ss << std::fixed;
    if (currentTool == 0) { // Brush
        ss << ", Size: " << brushSize;
    } else if (currentTool == 1) { // Eraser
        ss << ", Size: " << eraserSize;
    }
    status_text += ss.str();

    float text_scale = STATUS_BAR_TEXT_SCALE; // Use the dedicated constant
    float text_width = std::strlen(status_text.c_str()) * text_scale * 0.8f; // Rough estimate
    float text_height = text_scale * 1.0f; // Rough estimate

    float bar_height = text_height + PADDING_Y_GL; // Total height includes padding
    float bar_width = text_width + 2 * PADDING_X_GL;

    float x = 1.0f - PADDING_X_GL - bar_width;
    float y = -1.0f; // Position at the very bottom of the window in GL coords

    drawRoundedRect(x, y, bar_width, bar_height, PANEL_R, PANEL_G, PANEL_B, CORNER_RADIUS_GL);
    drawRoundedRectOutline(x, y, bar_width, bar_height, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL);

    // Text position within the status bar
    drawText(x + PADDING_X_GL, y + (bar_height - text_height)/2.0f, status_text.c_str(), TEXT_R, TEXT_G, TEXT_B, text_scale);
}

// Drawing logic: Renders all previously completed strokes/shapes
void drawStrokes() {
    for (const auto& stroke : strokes) {
        if (stroke.tool == 5) { // If it's a fill stroke
            glColor3f(stroke.fillColor[0], stroke.fillColor[1], stroke.fillColor[2]);
            if (stroke.circleRadius > 0) {
                drawCircle(stroke.circleCenter.x, stroke.circleCenter.y, stroke.circleRadius,
                           stroke.fillColor[0], stroke.fillColor[1], stroke.fillColor[2], true);
            } else {
                float minX = std::min(stroke.rectStart.x, stroke.rectEnd.x);
                float maxX = std::max(stroke.rectStart.x, stroke.rectEnd.x);
                float minY = std::min(stroke.rectStart.y, stroke.rectEnd.y);
                float maxY = std::max(stroke.rectStart.y, stroke.rectEnd.y);
                drawRect(minX, minY, maxX - minX, maxY - minY,
                         stroke.fillColor[0], stroke.fillColor[1], stroke.fillColor[2]);
            }
            continue;
        }

        if (stroke.tool == 1) { // Eraser
            // Eraser now draws with the canvas background color for seamless erasing
            glColor3f(BG_R, BG_G, BG_B); 
        } else { // Brush or Shapes
            if (!stroke.points.empty()) {
                glColor3f(stroke.points[0].r, stroke.points[0].g, stroke.points[0].b);
            } else {
                glColor3f(0.0f, 0.0f, 0.0f);
            }
        }

        glPointSize(stroke.size);
        glBegin(GL_POINTS);
        for (const auto& point : stroke.points) {
            glVertex2f(point.x, point.y);
        }
        glEnd();

        if (stroke.points.size() > 1) {
            glLineWidth(stroke.size / 2.0f);
            if (stroke.tool == 2) { // Rectangle outline
                glBegin(GL_LINE_LOOP);
            } else if (stroke.tool == 3) { // Circle outline
                glBegin(GL_LINE_LOOP); 
            } else if (stroke.tool == 4) { // Line tool
                glBegin(GL_LINES);
            } else { // Brush/Eraser
                glBegin(GL_LINE_STRIP);
            }
            for (const auto& point : stroke.points) {
                glVertex2f(point.x, point.y);
            }
            glEnd();
        }
    }
}

// Drawing logic: Renders the preview for shapes (rectangle, circle, line) before final commit
void drawShapePreview() {
    if (!isDrawing || (currentTool < 2 && currentTool != 5) || currentTool > 5) return; 
    
    if (std::abs(shapeStart.x - shapeEnd.x) < 0.001f && std::abs(shapeStart.y - shapeEnd.y) < 0.001f) {
        return;
    }

    glColor3f(currentColor[0], currentColor[1], currentColor[2]);
    glLineWidth(brushSize / 2.0f);

    const float canvasMinX = SIDEBAR_RIGHT_GL;
    const float canvasMaxX = 1.0f;
    const float canvasMinY = DRAWING_AREA_BOTTOM_GL; // Use the new constant
    const float canvasMaxY = CANVAS_TOP_GL;

    switch (currentTool) {
        case 2: // Rectangle preview
        case 5: // Fill tool (previews potential rectangle area)
            glBegin(GL_LINE_LOOP);
            glVertex2f(shapeStart.x, shapeStart.y);
            glVertex2f(shapeEnd.x, shapeStart.y);
            glVertex2f(shapeEnd.x, shapeEnd.y);
            glVertex2f(shapeStart.x, shapeEnd.y);
            glEnd();
            break;
        case 3: // Circle preview
            {
                float radius = std::sqrt(std::pow(shapeEnd.x - shapeStart.x, 2) + std::pow(shapeEnd.y - shapeStart.y, 2));

                float maxRadiusX = std::min(shapeStart.x - canvasMinX, canvasMaxX - shapeStart.x);
                float maxRadiusY = std::min(shapeStart.y - canvasMinY, canvasMaxY - shapeStart.y);
                radius = std::min({radius, maxRadiusX, maxRadiusY});
                radius = std::max(0.0f, radius);

                drawCircle(shapeStart.x, shapeStart.y, radius, currentColor[0], currentColor[1], currentColor[2], false);
            }
            break;
        case 4: // Line preview
            glBegin(GL_LINES);
            glVertex2f(shapeStart.x, shapeStart.y);
            glVertex2f(shapeEnd.x, shapeEnd.y);
            glEnd();
            break;
    }
}

// Drawing logic: Renders the stroke that is currently being drawn (for brush/eraser)
void drawCurrentStroke() {
    if (!isDrawing || currentStroke.points.empty()) return;

    if (currentTool == 1) { // Eraser
        // Eraser now draws with the canvas background color for seamless erasing
        glColor3f(BG_R, BG_G, BG_B);
    } else { // Brush
        glColor3f(currentColor[0], currentColor[1], currentColor[2]);
    }

    glPointSize(currentStroke.size);
    glBegin(GL_POINTS);
    for (const auto& point : currentStroke.points) {
        glVertex2f(point.x, point.y);
    }
    glEnd();

    if (currentStroke.points.size() > 1) {
        glLineWidth(currentStroke.size / 2.0f);
        glBegin(GL_LINE_STRIP);
        for (const auto& point : currentStroke.points) {
            glVertex2f(point.x, point.y);
        }
        glEnd();
    }
}

// Drawing logic: Draws a faint grid on the canvas
void drawGrid() {
    if (!showGrid) return;

    glColor3f(GRID_R, GRID_G, GRID_B);
    glLineWidth(0.5f);

    float grid_step_gl_x = 0.05f; // Grid line every 0.05 GL units horizontally
    float grid_step_gl_y = 0.05f; // Grid line every 0.05 GL units vertically

    glBegin(GL_LINES);
    // Vertical lines
    for (float x = SIDEBAR_RIGHT_GL; x <= 1.0f; x += grid_step_gl_x) {
        glVertex2f(x, DRAWING_AREA_BOTTOM_GL); // Use the new constant
        glVertex2f(x, CANVAS_TOP_GL);
    }
    // Horizontal lines
    for (float y = DRAWING_AREA_BOTTOM_GL; y <= CANVAS_TOP_GL; y += grid_step_gl_y) { // Use the new constant
        glVertex2f(SIDEBAR_RIGHT_GL, y);
        glVertex2f(1.0f, y);
    }
    glEnd();
}

// Function to save the canvas content as a JPG image
void saveScreenshotAsJpg(const char* filename, int windowWidth_px, int windowHeight_px) {
    // Calculate the canvas area in pixel coordinates
    int canvas_x_pixel = static_cast<int>((SIDEBAR_RIGHT_GL + 1.0f) / 2.0f * windowWidth_px);
    
    // Convert DRAWING_AREA_BOTTOM_GL (OpenGL Y coord) to pixel Y coord (from bottom of window)
    int canvas_y_pixel = static_cast<int>((DRAWING_AREA_BOTTOM_GL + 1.0f) / 2.0f * windowHeight_px);
    
    int canvas_width_pixel = static_cast<int>((1.0f - SIDEBAR_RIGHT_GL) / 2.0f * windowWidth_px);
    int canvas_height_pixel = static_cast<int>((CANVAS_TOP_GL - DRAWING_AREA_BOTTOM_GL) / 2.0f * windowHeight_px); // Height relative to new bottom limit

    // Allocate buffer for RGB data (3 bytes per pixel)
    std::vector<unsigned char> pixels(canvas_width_pixel * canvas_height_pixel * 3);

    // Read pixels from the frame buffer.
    glReadPixels(canvas_x_pixel, canvas_y_pixel, canvas_width_pixel, canvas_height_pixel, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL reads pixels from bottom-left, but image formats like JPG usually expect top-left.
    // Need to flip the image vertically.
    std::vector<unsigned char> flipped_pixels(canvas_width_pixel * canvas_height_pixel * 3);
    int row_size = canvas_width_pixel * 3;
    for (int y = 0; y < canvas_height_pixel; ++y) {
        // Copy rows from bottom to top into the flipped_pixels buffer from top to bottom
        std::memcpy(&flipped_pixels[(canvas_height_pixel - 1 - y) * row_size], &pixels[y * row_size], row_size);
    }

    // Save as JPG using stb_image_write
    if (stbi_write_jpg(filename, canvas_width_pixel, canvas_height_pixel, 3, flipped_pixels.data(), 90)) {
        std::cout << "Screenshot saved to " << filename << std::endl;
    } else {
        std::cerr << "Failed to save screenshot to " << filename << std::endl;
    }
}


// --- Event Handlers ---

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);

    if (action == GLFW_PRESS) {
        bool handledClick = false;

        // --- Handle Left-Click Press ---
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            // Check for clicks in the Top Bar area (Preset Colors, Clear Button, Save Button)
            if (isInTopBar(xpos, ypos)) {
                // Preset Colors
                float colors[][3] = {
                    {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                    {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}
                };
                float swatch_y = 1.0f - PADDING_Y_GL - COLOR_SWATCH_SIZE_GL;
                float swatch_start_x = -1.0f + PADDING_X_GL;
                float swatch_spacing_x = COLOR_SWATCH_SIZE_GL + PADDING_X_GL / 2.0f;

                for (int i = 0; i < 8; i++) {
                    float x = swatch_start_x + i * swatch_spacing_x;
                    if (glX >= x && glX <= x + COLOR_SWATCH_SIZE_GL && glY >= swatch_y && glY <= swatch_y + COLOR_SWATCH_SIZE_GL) {
                        currentColor[0] = colors[i][0]; currentColor[1] = colors[i][1]; currentColor[2] = colors[i][2];
                        std::memcpy(customColor, currentColor, sizeof(customColor));
                        handledClick = true;
                        break;
                    }
                }

                // Clear Button
                float clear_btn_w = BUTTON_HEIGHT_GL * 1.5f;
                float clear_btn_h = BUTTON_HEIGHT_GL;
                float clear_btn_x = 1.0f - PADDING_X_GL - clear_btn_w;
                float clear_btn_y = 1.0f - PADDING_Y_GL - clear_btn_h;
                if (glX >= clear_btn_x && glX <= clear_btn_x + clear_btn_w && glY >= clear_btn_y && glY <= clear_btn_y + clear_btn_h) {
                    strokes.clear();
                    handledClick = true;
                }

                // Save Button
                float save_btn_w = BUTTON_HEIGHT_GL * 1.5f;
                float save_btn_h = BUTTON_HEIGHT_GL;
                float save_btn_x = clear_btn_x - PADDING_X_GL / 2.0f - save_btn_w; // Position to the left of Clear button
                float save_btn_y = clear_btn_y; // Same vertical position

                if (glX >= save_btn_x && glX <= save_btn_x + save_btn_w && glY >= save_btn_y && glY <= save_btn_y + save_btn_h) {
                    saveScreenshotAsJpg("sketchmate_drawing.jpg", windowWidth, windowHeight);
                    handledClick = true;
                }
            }

            // Check for clicks in the Sidebar area (Tools, Colors, Sizes)
            if (!handledClick && isInSidebar(xpos, ypos)) {
                SectionYPositions section_y_pos = getSectionYPositions();

                // Tool buttons click detection (directly from toolsSectionTopY)
                float tool_start_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
                float tool_btn_w = uiWidth - 2 * PADDING_X_GL;
                float tool_btn_h = BUTTON_HEIGHT_GL;
                float tool_row_spacing = tool_btn_h + PADDING_Y_GL; 

                float current_y_for_tools_click = section_y_pos.toolsSectionTopY;
                current_y_for_tools_click -= (PADDING_Y_GL + UI_LABEL_BLOCK_HEIGHT + PADDING_Y_GL); // Account for "TOOLS" label + padding

                for (int i = 0; i < 6; i++) {
                    int tool_idx = tools_order[i];
                    float x_button_area = tool_start_x;
                    float y_button_area = current_y_for_tools_click - i * tool_row_spacing;

                    if (glX >= x_button_area && glX <= x_button_area + tool_btn_w &&
                        glY >= y_button_area && glY <= y_button_area + tool_btn_h) {
                        currentTool = tool_idx;
                        handledClick = true;
                        break;
                    }
                }

                // Color sliders click detection
                if (!handledClick) {
                    float color_slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
                    float color_slider_w = uiWidth - 2 * PADDING_X_GL;
                    float color_slider_h = SLIDER_HEIGHT_GL;
                    
                    ColorSliderYPositions color_y_pos = getIndividualColorSliderYPositions(section_y_pos.colorsSectionTopY); // Get calculated positions

                    if (glX >= color_slider_x && glX <= color_slider_x + color_slider_w) {
                        if (glY >= color_y_pos.rSliderBottomY && glY <= color_y_pos.rSliderBottomY + color_slider_h) {
                            float clamped_glX = std::max(color_slider_x, std::min(color_slider_x + color_slider_w, glX));
                            customColor[0] = (clamped_glX - color_slider_x - SLIDER_THUMB_WIDTH_GL / 2.0f) / (color_slider_w - SLIDER_THUMB_WIDTH_GL);
                            customColor[0] = std::max(0.0f, std::min(1.0f, customColor[0]));
                            currentColor[0] = customColor[0]; handledClick = true;
                            isDraggingColorSliderR = true;
                        } else if (glY >= color_y_pos.gSliderBottomY && glY <= color_y_pos.gSliderBottomY + color_slider_h) {
                            float clamped_glX = std::max(color_slider_x, std::min(color_slider_x + color_slider_w, glX));
                            customColor[1] = (clamped_glX - color_slider_x - SLIDER_THUMB_WIDTH_GL / 2.0f) / (color_slider_w - SLIDER_THUMB_WIDTH_GL);
                            customColor[1] = std::max(0.0f, std::min(1.0f, customColor[1]));
                            currentColor[1] = customColor[1]; handledClick = true;
                            isDraggingColorSliderG = true;
                        } else if (glY >= color_y_pos.bSliderBottomY && glY <= color_y_pos.bSliderBottomY + color_slider_h) {
                            float clamped_glX = std::max(color_slider_x, std::min(color_slider_x + color_slider_w, glX));
                            customColor[2] = (clamped_glX - color_slider_x - SLIDER_THUMB_WIDTH_GL / 2.0f) / (color_slider_w - SLIDER_THUMB_WIDTH_GL);
                            customColor[2] = std::max(0.0f, std::min(1.0f, customColor[2]));
                            currentColor[2] = customColor[2]; handledClick = true;
                            isDraggingColorSliderB = true;
                        }
                    }
                }

                // Size sliders click detection
                if (!handledClick) {
                    float size_slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
                    float size_slider_w = uiWidth - 2 * PADDING_X_GL;
                    float size_slider_h = SLIDER_HEIGHT_GL;

                    SizeSliderYPositions size_y_pos = getIndividualSizeSliderYPositions(section_y_pos.sizesSectionTopY);

                    if (glX >= size_slider_x && glX <= size_slider_x + size_slider_w) {
                        if (glY >= size_y_pos.brushSliderBottomY && glY <= size_y_pos.brushSliderBottomY + size_slider_h) {
                            float clamped_glX = std::max(size_slider_x, std::min(size_slider_x + size_slider_w, glX));
                            brushSize = 1.0f + ((clamped_glX - size_slider_x - SLIDER_THUMB_WIDTH_GL / 2.0f) / (size_slider_w - SLIDER_THUMB_WIDTH_GL)) * 19.0f;
                            brushSize = std::max(1.0f, std::min(20.0f, brushSize));
                            isDraggingBrushSlider = true;
                            handledClick = true;
                        } else if (glY >= size_y_pos.eraserSliderBottomY && glY <= size_y_pos.eraserSliderBottomY + size_slider_h) {
                            float clamped_glX = std::max(size_slider_x, std::min(size_slider_x + size_slider_w, glX));
                            eraserSize = 1.0f + ((clamped_glX - size_slider_x - SLIDER_THUMB_WIDTH_GL / 2.0f) / (size_slider_w - SLIDER_THUMB_WIDTH_GL)) * 19.0f;
                            eraserSize = std::max(1.0f, std::min(20.0f, eraserSize));
                            isDraggingEraserSlider = true;
                            handledClick = true;
                        }
                    }
                }
            }

            // Start drawing on canvas if the click was not handled by any UI element
            if (!handledClick && isInCanvas(glX, glY)) {
                // Ensure mouse coordinates are clamped to the canvas bounds before use
                float clampedGlX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, glX));
                float clampedGlY = std::max(DRAWING_AREA_BOTTOM_GL, std::min(CANVAS_TOP_GL, glY));

                if (currentTool == 5) { // Handle Fill tool specifically
                    bool filledExistingShape = false;
                    for (int i = strokes.size() - 1; i >= 0; --i) {
                        const Stroke& existingStroke = strokes[i];
                        if (existingStroke.tool == 2) { // Rectangle
                            float minX = std::min(existingStroke.points[0].x, existingStroke.points[2].x);
                            float maxX = std::max(existingStroke.points[0].x, existingStroke.points[2].x);
                            float minY = std::min(existingStroke.points[0].y, existingStroke.points[2].y);
                            float maxY = std::max(existingStroke.points[0].y, existingStroke.points[2].y);

                            if (clampedGlX >= minX && clampedGlX <= maxX && clampedGlY >= minY && clampedGlY <= maxY) {
                                Stroke fillStroke;
                                fillStroke.tool = 5;
                                std::memcpy(fillStroke.fillColor, currentColor, sizeof(fillStroke.fillColor));
                                fillStroke.rectStart = Point(minX, minY);
                                fillStroke.rectEnd = Point(maxX, maxY);
                                fillStroke.circleRadius = 0;
                                strokes.push_back(fillStroke);
                                filledExistingShape = true;
                                break;
                            }
                        } else if (existingStroke.tool == 3) { // Circle
                            if (existingStroke.circleRadius > 0) {
                                float dist_sq = std::pow(clampedGlX - existingStroke.circleCenter.x, 2) + std::pow(clampedGlY - existingStroke.circleCenter.y, 2);
                                if (dist_sq <= std::pow(existingStroke.circleRadius, 2)) {
                                    Stroke fillStroke;
                                    fillStroke.tool = 5;
                                    std::memcpy(fillStroke.fillColor, currentColor, sizeof(fillStroke.fillColor));
                                    fillStroke.circleCenter = existingStroke.circleCenter;
                                    fillStroke.circleRadius = existingStroke.circleRadius;
                                    strokes.push_back(fillStroke);
                                    filledExistingShape = true;
                                    break;
                                }
                            }
                        }
                    }
                    isDrawing = false; 
                } else { // Other tools (Brush, Eraser, Shapes)
                    isDrawing = true;
                    // Initial point for drawing
                    if (currentTool < 2) { // Brush or Eraser
                        currentStroke.points.clear();
                        currentStroke.tool = currentTool;
                        currentStroke.size = (currentTool == 0) ? brushSize : eraserSize;
                        currentStroke.points.push_back(Point(clampedGlX, clampedGlY, currentColor[0], currentColor[1], currentColor[2]));
                    }
                    // Start point for shapes
                    shapeStart = Point(clampedGlX, clampedGlY, currentColor[0], currentColor[1], currentColor[2]);
                    shapeEnd = shapeStart; // Initialize shapeEnd to shapeStart
                }
            }
        }
    } else if (action == GLFW_RELEASE) {
        isDraggingBrushSlider = false;
        isDraggingEraserSlider = false;
        isDraggingColorSliderR = false; // Stop dragging color sliders
        isDraggingColorSliderG = false;
        isDraggingColorSliderB = false;

        if (isDrawing) {
            // Ensure final mouse release position is clamped
            float finalGlX, finalGlY;
            screenToGL(xpos, ypos, finalGlX, finalGlY);
            finalGlX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, finalGlX));
            finalGlY = std::max(DRAWING_AREA_BOTTOM_GL, std::min(CANVAS_TOP_GL, finalGlY));
            shapeEnd = Point(finalGlX, finalGlY, currentColor[0], currentColor[1], currentColor[2]);

            if (currentTool < 2) { // Brush or Eraser
                // Only add stroke if there are points
                if (!currentStroke.points.empty()) {
                    strokes.push_back(currentStroke);
                }
                currentStroke.points.clear();
            } else if (currentTool >= 2 && currentTool <= 4) { // Shapes
                Stroke newStroke;
                newStroke.tool = currentTool;
                newStroke.size = brushSize; // Shapes use brushSize for outline thickness
                
                // Only add shape if it's not a tiny point click
                if (std::abs(shapeStart.x - shapeEnd.x) > 0.001f || std::abs(shapeStart.y - shapeEnd.y) > 0.001f) {
                    switch (currentTool) {
                        case 2: // Rectangle
                            newStroke.points.push_back(Point(shapeStart.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2]));
                            newStroke.points.push_back(Point(shapeEnd.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2]));
                            newStroke.points.push_back(Point(shapeEnd.x, shapeEnd.y, currentColor[0], currentColor[1], currentColor[2]));
                            newStroke.points.push_back(Point(shapeStart.x, shapeEnd.y, currentColor[0], currentColor[1], currentColor[2]));
                            newStroke.points.push_back(Point(shapeStart.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2])); // Close the loop
                            break;
                        case 3: // Circle
                            {
                                float radius = std::sqrt(std::pow(shapeEnd.x - shapeStart.x, 2) + std::pow(shapeEnd.y - shapeStart.y, 2));
                                const float canvasMinX = SIDEBAR_RIGHT_GL;
                                const float canvasMaxX = 1.0f;
                                const float canvasMinY = DRAWING_AREA_BOTTOM_GL; // Use the new constant
                                const float canvasMaxY = CANVAS_TOP_GL;
                                // Clamp radius to prevent drawing outside canvas bounds if the circle center is near an edge
                                float maxRadiusX = std::min(shapeStart.x - canvasMinX, canvasMaxX - shapeStart.x);
                                float maxRadiusY = std::min(shapeStart.y - canvasMinY, canvasMaxY - shapeStart.y);
                                radius = std::min({radius, maxRadiusX, maxRadiusY});
                                radius = std::max(0.0f, radius); // Ensure radius is non-negative
                                newStroke.circleCenter = shapeStart;
                                newStroke.circleRadius = radius;
                                for (int i = 0; i <= 360; i += 5) { 
                                    float angle = i * M_PI / 180.0f;
                                    float x_pt = shapeStart.x + radius * std::cos(angle);
                                    float y_pt = shapeStart.y + radius * std::sin(angle);
                                    newStroke.points.push_back(Point(x_pt, y_pt, currentColor[0], currentColor[1], currentColor[2]));
                                }
                            }
                            break;
                        case 4: // Line
                            newStroke.points.push_back(Point(shapeStart.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2]));
                            newStroke.points.push_back(Point(shapeEnd.x, shapeEnd.y, currentColor[0], currentColor[1], currentColor[2]));
                            break;
                    }
                    if (!newStroke.points.empty()) {
                        strokes.push_back(newStroke);
                    }
                }
            }
            isDrawing = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);

    float slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float slider_w = uiWidth - 2 * PADDING_X_GL;
    float slider_h = SLIDER_HEIGHT_GL;

    SectionYPositions section_y_pos = getSectionYPositions();
    ColorSliderYPositions color_y_pos = getIndividualColorSliderYPositions(section_y_pos.colorsSectionTopY);
    SizeSliderYPositions size_y_pos = getIndividualSizeSliderYPositions(section_y_pos.sizesSectionTopY);

    // Handle color slider dragging
    if (isDraggingColorSliderR) {
        float clamped_glX = std::max(slider_x, std::min(slider_x + slider_w, glX));
        customColor[0] = (clamped_glX - slider_x - SLIDER_THUMB_WIDTH_GL / 2.0f) / (slider_w - SLIDER_THUMB_WIDTH_GL);
        customColor[0] = std::max(0.0f, std::min(1.0f, customColor[0]));
        currentColor[0] = customColor[0];
    } else if (isDraggingColorSliderG) {
        float clamped_glX = std::max(slider_x, std::min(slider_x + slider_w, glX));
        customColor[1] = (clamped_glX - slider_x - SLIDER_THUMB_WIDTH_GL / 2.0f) / (slider_w - SLIDER_THUMB_WIDTH_GL);
        customColor[1] = std::max(0.0f, std::min(1.0f, customColor[1]));
        currentColor[1] = customColor[1];
    } else if (isDraggingColorSliderB) {
        float clamped_glX = std::max(slider_x, std::min(slider_x + slider_w, glX));
        customColor[2] = (clamped_glX - slider_x - SLIDER_THUMB_WIDTH_GL / 2.0f) / (slider_w - SLIDER_THUMB_WIDTH_GL);
        customColor[2] = std::max(0.0f, std::min(1.0f, customColor[2]));
        currentColor[2] = customColor[2];
    }
    // Handle size slider dragging
    else if (isDraggingBrushSlider) {
        float clamped_glX = std::max(slider_x, std::min(slider_x + slider_w, glX));
        brushSize = 1.0f + ((clamped_glX - slider_x - SLIDER_THUMB_WIDTH_GL / 2.0f) / (slider_w - SLIDER_THUMB_WIDTH_GL)) * 19.0f;
        brushSize = std::max(1.0f, std::min(20.0f, brushSize));
    } 
    else if (isDraggingEraserSlider) {
        float clamped_glX = std::max(slider_x, std::min(slider_x + slider_w, glX));
        eraserSize = 1.0f + ((clamped_glX - slider_x - SLIDER_THUMB_WIDTH_GL / 2.0f) / (slider_w - SLIDER_THUMB_WIDTH_GL)) * 19.0f;
        eraserSize = std::max(1.0f, std::min(20.0f, eraserSize));
    }
    else if (isDrawing && !isInSidebar(xpos, ypos) && !isInTopBar(xpos,ypos)) { 
        // Clamp cursor position to canvas boundaries for drawing
        glX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, glX));
        glY = std::max(DRAWING_AREA_BOTTOM_GL, std::min(CANVAS_TOP_GL, glY)); // Use the new constant

        if (currentTool < 2) { // Brush or Eraser
            if (currentStroke.points.empty()) { 
                currentStroke.points.push_back(shapeStart); // Ensure starting point is added
            }
            currentStroke.points.push_back(Point(glX, glY, currentColor[0], currentColor[1], currentColor[2]));
        } else if (currentTool >= 2 && currentTool <= 5) { // Shapes or Fill
            shapeEnd = Point(glX, glY, currentColor[0], currentColor[1], currentColor[2]);
        }
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);

    float size_slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float size_slider_w = uiWidth - 2 * PADDING_X_GL;
    float size_slider_h = SLIDER_HEIGHT_GL;

    SectionYPositions section_y_pos = getSectionYPositions();
    SizeSliderYPositions size_y_pos = getIndividualSizeSliderYPositions(section_y_pos.sizesSectionTopY);

    if (glX >= size_slider_x && glX <= size_slider_x + size_slider_w &&
        glY >= size_y_pos.brushSliderBottomY && glY <= size_y_pos.brushSliderBottomY + size_slider_h) {
        brushSize += yoffset * 2.0f;
        brushSize = std::max(1.0f, std::min(20.0f, brushSize));
    }
    else if (glX >= size_slider_x && glX <= size_slider_x + size_slider_w &&
        glY >= size_y_pos.eraserSliderBottomY && glY <= size_y_pos.eraserSliderBottomY + size_slider_h) {
        eraserSize += yoffset * 2.0f;
        eraserSize = std::max(1.0f, std::min(20.0f, eraserSize));
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_Z && (mods & GLFW_MOD_CONTROL || mods & GLFW_MOD_SUPER)) {
            if (!strokes.empty()) {
                strokes.pop_back();
            }
        } else if (key == GLFW_KEY_C && (mods & GLFW_MOD_CONTROL || mods & GLFW_MOD_SUPER)) {
            strokes.clear();
        } else if (key == GLFW_KEY_B) {
            currentTool = 0; // Brush
        } else if (key == GLFW_KEY_E) {
            currentTool = 1; // Eraser
        } else if (key == GLFW_KEY_G) {
            showGrid = !showGrid; // Toggle grid
        }
    }
}

// --- Main Rendering Function ---
void render() {
    glClearColor(BG_R, BG_G, BG_B, 1.0f); // Set clear color to the new background
    glClear(GL_COLOR_BUFFER_BIT);

    double mouseX, mouseY;
    glfwGetCursorPos(glfwGetCurrentContext(), &mouseX, &mouseY);
    float mouseX_gl, mouseY_gl;
    screenToGL(mouseX, mouseY, mouseX_gl, mouseY_gl);

    // Draw UI backgrounds (panels and shadows)
    drawShadow(SIDEBAR_LEFT_GL, -1.0f, uiWidth, 1.0f - CANVAS_TOP_GL, SHADOW_R, SHADOW_G, SHADOW_B, SHADOW_ALPHA, 0.008f, 0.008f, CORNER_RADIUS_GL * 2.0f);
    drawRoundedRect(SIDEBAR_LEFT_GL, -1.0f, uiWidth, 1.0f - CANVAS_TOP_GL, PANEL_R, PANEL_G, PANEL_B, CORNER_RADIUS_GL * 2.0f);
    drawRoundedRectOutline(SIDEBAR_LEFT_GL, -1.0f, uiWidth, 1.0f - CANVAS_TOP_GL, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL * 2.0f, 1.0f);
    
    // Draw right border for the toolbar (separator between toolbar and canvas)
    glColor3f(BORDER_R, BORDER_G, BORDER_B);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(SIDEBAR_RIGHT_GL, -1.0f);
    glVertex2f(SIDEBAR_RIGHT_GL, CANVAS_TOP_GL);
    glEnd();

    drawRoundedRect(-1.0f, CANVAS_TOP_GL, 2.0f, TOP_BAR_HEIGHT_GL, ACCENT_R, ACCENT_G, ACCENT_B, CORNER_RADIUS_GL * 2.0f);
    drawRoundedRectOutline(-1.0f, CANVAS_TOP_GL, 2.0f, TOP_BAR_HEIGHT_GL, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL * 2.0f, 1.0f);

    // Draw canvas background and border
    drawShadow(SIDEBAR_RIGHT_GL, -1.0f, 2.0f - uiWidth, 1.0f - CANVAS_TOP_GL, SHADOW_R, SHADOW_G, SHADOW_B, SHADOW_ALPHA, 0.008f, 0.008f, CORNER_RADIUS_GL * 2.0f);
    drawRoundedRect(SIDEBAR_RIGHT_GL, -1.0f, 2.0f - uiWidth, 1.0f - CANVAS_TOP_GL, 1.0f, 1.0f, 1.0f, CORNER_RADIUS_GL * 2.0f);
    drawRoundedRectOutline(SIDEBAR_RIGHT_GL, -1.0f, 2.0f - uiWidth, 1.0f - CANVAS_TOP_GL, BORDER_R, BORDER_G, BORDER_B, CORNER_RADIUS_GL * 2.0f, 1.5f);
    
    // Draw Top Bar UI elements (includes Clear and Save buttons)
    drawPresetColorPalette(mouseX_gl, mouseY_gl);
    drawTopBarButtons(mouseX_gl, mouseY_gl); 

    // Get all section Y positions at once for consistent drawing and hit-testing
    SectionYPositions section_y_pos = getSectionYPositions();

    // Draw Sidebar UI elements
    drawToolButtons(section_y_pos.toolsSectionTopY, mouseX_gl, mouseY_gl);
    // Add separator after tools
    glColor3f(BORDER_R, BORDER_G, BORDER_B);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(SIDEBAR_LEFT_GL + PADDING_X_GL, section_y_pos.colorsSectionTopY + SECTION_PADDING_Y_GL / 2.0f);
    glVertex2f(SIDEBAR_RIGHT_GL - PADDING_X_GL, section_y_pos.colorsSectionTopY + SECTION_PADDING_Y_GL / 2.0f);
    glEnd();

    drawColorSlidersSidebar(section_y_pos.colorsSectionTopY, mouseX_gl, mouseY_gl);
    // Add separator after colors
    glColor3f(BORDER_R, BORDER_G, BORDER_B);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(SIDEBAR_LEFT_GL + PADDING_X_GL, section_y_pos.sizesSectionTopY + SECTION_PADDING_Y_GL / 2.0f);
    glVertex2f(SIDEBAR_RIGHT_GL - PADDING_X_GL, section_y_pos.sizesSectionTopY + SECTION_PADDING_Y_GL / 2.0f);
    glEnd();

    drawSizeSelectorsSidebar(section_y_pos.sizesSectionTopY, mouseX_gl, mouseY_gl);

    // --- Enable Scissor Test for Canvas Drawing ---
    // Scissor test defines a rectangular region to which all subsequent drawing is clipped.
    // x, y specify the lower-left corner of the scissor box in pixels.
    // width, height specify the width and height of the scissor box in pixels.
    int scissor_x_pixel = static_cast<int>((SIDEBAR_RIGHT_GL + 1.0f) / 2.0f * windowWidth);
    int scissor_y_pixel = static_cast<int>((DRAWING_AREA_BOTTOM_GL + 1.0f) / 2.0f * windowHeight); // Bottom of drawing area in pixels
    int scissor_width_pixel = static_cast<int>((1.0f - SIDEBAR_RIGHT_GL) / 2.0f * windowWidth);
    int scissor_height_pixel = static_cast<int>((CANVAS_TOP_GL - DRAWING_AREA_BOTTOM_GL) / 2.0f * windowHeight); // Height from DRAWING_AREA_BOTTOM_GL to CANVAS_TOP_GL

    glEnable(GL_SCISSOR_TEST);
    glScissor(scissor_x_pixel, scissor_y_pixel, scissor_width_pixel, scissor_height_pixel);

    // Draw Canvas elements
    drawGrid(); // Draw grid if enabled
    drawStrokes();
    drawCurrentStroke();
    drawShapePreview();

    glDisable(GL_SCISSOR_TEST);

    // Draw Status Bar (always on top of other elements)
    drawStatusBar();
}

int main() {
    // Initialize GLFW (Graphics Library Framework)
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "SketchMate", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate(); // Terminate GLFW if window creation fails
        return -1;
    }
    glfwMakeContextCurrent(window); // Make the window's context current on the calling thread

    // Initialize GLAD (OpenGL Loader-Generator) to load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set up callback functions for user input
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback); // For undo functionality
    
    // Set the clear color for the window background to white
    glClearColor(BG_R, BG_G, BG_B, 1.0f); 

    // Enable point and line smoothing for better visual quality of strokes
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST); // Hint for highest quality smoothing
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND); // Enable blending for transparency and anti-aliasing effects
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard blending function

    // Main application loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents(); // Process all pending events (input, window events)
        glfwGetWindowSize(window, &windowWidth, &windowHeight); // Get current window size
        glViewport(0, 0, windowWidth, windowHeight); // Set the viewport to match window size
        render(); // Call the rendering function to draw everything
        glfwSwapBuffers(window); // Swap the front and back buffers to display the rendered frame
    }

    glfwTerminate(); // Terminate GLFW when the loop ends (window closed)
    return 0;
}
