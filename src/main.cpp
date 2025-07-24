#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm> // For std::min, std::max, std::abs

// Structure to store drawing data
struct Point {
    float x, y;
    float r, g, b;
    Point(float x = 0, float y = 0, float r = 0, float g = 0, float b = 0)
        : x(x), y(y), r(r), g(g), b(b) {}
};

struct Stroke {
    std::vector<Point> points;
    int tool; // 0=brush, 1=eraser, 2=rectangle, 3=circle, 4=line, 5=fill
    float size;
    float fillColor[3] = {0, 0, 0}; // For fill tool
    // For fill tool, these store the bounds of the shape being filled
    Point rectStart, rectEnd; 
    Point circleCenter; 
    float circleRadius; 
};

// Global variables
std::vector<Stroke> strokes;
Stroke currentStroke;
float currentColor[3] = {0.0f, 0.0f, 0.0f}; // Black - this is the actual drawing color
float customColor[3] = {0.0f, 0.0f, 0.0f}; // For RGB sliders - keeps track of slider positions
float brushSize = 3.0f;
float eraserSize = 10.0f;
int currentTool = 0; // 0=brush, 1=eraser, 2=rectangle, 3=circle, 4=line, 5=fill
bool isDrawing = false;
bool isDraggingBrushSlider = false; // Flag for brush slider dragging
bool isDraggingEraserSlider = false; // Flag for eraser slider dragging

Point shapeStart, shapeEnd; // For shape drawing preview (and initial point for brush/eraser)

int windowWidth = 1000, windowHeight = 700;
float uiWidth = 0.15f; // FURTHER DECREASED: Adjusted sidebar width for better aesthetics

// Global variables for size slider visibility - NOW ALWAYS TRUE
bool showBrushSizeSlider = true;
bool showEraserSizeSlider = true;

// Global array for tool order
int tools_order[] = {0, 1, 2, 3, 4, 5};

// UI Layout Constants
const float SIDEBAR_LEFT_GL = -1.0f;
const float SIDEBAR_RIGHT_GL = -1.0f + uiWidth;
const float PADDING_X_GL = 0.02f; // Horizontal padding from sidebar edge
const float PADDING_Y_GL = 0.03f; // Vertical padding between elements/buttons

const float SLIDER_VERTICAL_SPACING_GL = 0.015f; // Tighter spacing for stacked sliders
const float SECTION_PADDING_Y_GL = 0.04f; // Larger padding between major UI sections

const float BUTTON_HEIGHT_GL = 0.08f; // Height for the overall button area (including padding around icon)
const float COLOR_SWATCH_SIZE_GL = 0.08f;
const float SLIDER_HEIGHT_GL = 0.03f;
const float SLIDER_THUMB_WIDTH_GL = 0.01f;

// Top Bar Constants (now simpler)
const float TOP_BAR_HEIGHT_GL = COLOR_SWATCH_SIZE_GL + 2 * PADDING_Y_GL;
const float CANVAS_TOP_GL = 1.0f - TOP_BAR_HEIGHT_GL; // Y-coordinate where canvas and sidebar start below top bar

// Icon specific constants
const float ICON_DRAW_SIZE_GL = 0.06f;


// Convert screen coordinates to OpenGL coordinates
void screenToGL(double x, double y, float& glX, float& glY) {
    glX = (x / windowWidth) * 2.0f - 1.0f;
    glY = 1.0f - (y / windowHeight) * 2.0f;
}

// Check if point is in sidebar UI area (below top bar)
bool isInSidebar(double xpos, double ypos) {
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);
    return glX >= SIDEBAR_LEFT_GL && glX < SIDEBAR_RIGHT_GL && glY < CANVAS_TOP_GL;
}

// Check if point is in top bar UI area
bool isInTopBar(double xpos, double ypos) {
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);
    return glY >= CANVAS_TOP_GL; // Top bar spans full width
}

// Restrict drawing to canvas area
bool isInCanvas(float glX, float glY) {
    return glX >= SIDEBAR_RIGHT_GL && glX <= 1.0f && glY >= -1.0f && glY < CANVAS_TOP_GL;
}

// Draw filled rectangle
void drawRect(float x, float y, float w, float h, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// Draw rectangle outline
void drawRectOutline(float x, float y, float w, float h, float r, float g, float b) {
    glColor3f(r, g, b);
    glLineWidth(1.0f); // Default outline width
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// Draw circle
void drawCircle(float cx, float cy, float radius, float r, float g, float b, bool filled = true) {
    glColor3f(r, g, b);
    if (filled) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
    } else {
        glBegin(GL_LINE_LOOP);
    }
    for (int i = 0; i <= 360; i += 10) {
        float angle = i * 3.1415926f / 180.0f;
        glVertex2f(cx + radius * std::cos(angle), cy + radius * std::sin(angle));
    }
    glEnd();
}

// Draw UI buttons (used for color swatches and clear button)
void drawButton(float x, float y, float w, float h, float r, float g, float b, bool selected = false) {
    if (selected) {
        drawRect(x, y, w, h, std::min(r + 0.15f, 1.0f), std::min(g + 0.15f, 1.0f), std::min(b + 0.15f, 1.0f));
    } else {
        drawRect(x, y, w, h, r, g, b);
    }
    drawRectOutline(x, y, w, h, 0.2f, 0.2f, 0.2f);
}

// Very basic text drawing using lines (for labels)
void drawText(float x, float y, const char* text, float r, float g, float b, float scale = 0.005f) {
    glColor3f(r, g, b);
    glLineWidth(1.5f);
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

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
            case ' ': glTranslatef(0.8f, 0, 0); break; // Space
        }
        glEnd();
        glTranslatef(1.2f, 0, 0); // Advance for next character
    }
    glPopMatrix();
}


// Draw preset color palette (in top bar, horizontal layout)
void drawPresetColorPalette() {
    float colors[][3] = {
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}
    };
    float swatch_y = 1.0f - PADDING_Y_GL - COLOR_SWATCH_SIZE_GL; // Fixed Y for this row
    float swatch_start_x = -1.0f + PADDING_X_GL;
    float swatch_spacing_x = COLOR_SWATCH_SIZE_GL + PADDING_X_GL / 2.0f;

    for (int i = 0; i < 8; i++) { // All 8 colors in one row
        float x = swatch_start_x + i * swatch_spacing_x;
        bool selected = (std::abs(currentColor[0] - colors[i][0]) < 0.01f &&
                         std::abs(currentColor[1] - colors[i][1]) < 0.01f &&
                         std::abs(currentColor[2] - colors[i][2]) < 0.01f);
        drawButton(x, swatch_y, COLOR_SWATCH_SIZE_GL, COLOR_SWATCH_SIZE_GL, colors[i][0], colors[i][1], colors[i][2], selected);
    }
}

// Draw improved RGB sliders (now in sidebar, stacked)
void drawColorSlidersSidebar(float& current_y) {
    float x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float w = uiWidth - 2 * PADDING_X_GL; // Sliders take full sidebar width
    float h = SLIDER_HEIGHT_GL;

    // Current Color Preview - at the top of the color sliders section
    float preview_h = SLIDER_HEIGHT_GL * 1.5f; // A bit larger for preview
    drawRect(x, current_y - preview_h, w, preview_h, customColor[0], customColor[1], customColor[2]);
    drawRectOutline(x, current_y - preview_h, w, preview_h, 0.2f, 0.2f, 0.2f);
    
    current_y -= (preview_h + PADDING_Y_GL); // Update y after drawing preview for the actual sliders

    // R Slider with gradient
    float slider_top_y_r = current_y - h;
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_top_y_r);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(x + w, slider_top_y_r);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(x + w, slider_top_y_r + h);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_top_y_r + h);
    glEnd();
    // Draw thumb in black with white outline
    drawRect(x + customColor[0] * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_r, SLIDER_THUMB_WIDTH_GL, h, 0.0f, 0.0f, 0.0f);
    drawRectOutline(x + customColor[0] * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_r, SLIDER_THUMB_WIDTH_GL, h, 1.0f, 1.0f, 1.0f);
    drawRectOutline(x, slider_top_y_r, w, h, 0.2f, 0.2f, 0.2f); // Outline on top of gradient
    current_y = slider_top_y_r - SLIDER_VERTICAL_SPACING_GL; // Update current_y for next slider

    // G Slider with gradient
    float slider_top_y_g = current_y - h;
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_top_y_g);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(x + w, slider_top_y_g);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(x + w, slider_top_y_g + h);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_top_y_g + h);
    glEnd();
    // Draw thumb in black with white outline
    drawRect(x + customColor[1] * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_g, SLIDER_THUMB_WIDTH_GL, h, 0.0f, 0.0f, 0.0f);
    drawRectOutline(x + customColor[1] * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_g, SLIDER_THUMB_WIDTH_GL, h, 1.0f, 1.0f, 1.0f);
    drawRectOutline(x, slider_top_y_g, w, h, 0.2f, 0.2f, 0.2f); // Outline on top of gradient
    current_y = slider_top_y_g - SLIDER_VERTICAL_SPACING_GL; // Update current_y for next slider

    // B Slider with gradient
    float slider_top_y_b = current_y - h;
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_top_y_b);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(x + w, slider_top_y_b);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(x + w, slider_top_y_b + h);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_top_y_b + h);
    glEnd();
    // Draw thumb in black with white outline
    drawRect(x + customColor[2] * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_b, SLIDER_THUMB_WIDTH_GL, h, 0.0f, 0.0f, 0.0f);
    drawRectOutline(x + customColor[2] * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_b, SLIDER_THUMB_WIDTH_GL, h, 1.0f, 1.0f, 1.0f);
    drawRectOutline(x, slider_top_y_b, w, h, 0.2f, 0.2f, 0.2f); // Outline on top of gradient
    
    current_y = slider_top_y_b - h - SECTION_PADDING_Y_GL; // Update current_y for next section
}

// Helper to draw a pencil icon
void drawPencilIcon(float cx, float cy, float size, bool selected) {
    float dim_factor = selected ? 0.7f : 1.0f; // Dim if selected

    float half_size = size / 2.0f;
    float body_width = half_size * 0.4f;
    float body_height = half_size * 0.8f;
    float tip_height = half_size * 0.4f;
    float eraser_height = half_size * 0.2f;

    // Pencil body (yellowish)
    glColor3f(0.9f * dim_factor, 0.8f * dim_factor, 0.2f * dim_factor);
    drawRect(cx - body_width/2, cy - body_height/2 + eraser_height/2, body_width, body_height, 0.9f, 0.8f, 0.2f);

    // Pencil tip (dark grey)
    glColor3f(0.3f * dim_factor, 0.3f * dim_factor, 0.3f * dim_factor);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - body_width/2, cy + body_height/2 + eraser_height/2);
    glVertex2f(cx + body_width/2, cy + body_height/2 + eraser_height/2);
    glVertex2f(cx, cy + body_height/2 + tip_height + eraser_height/2);
    glEnd();

    // Pencil eraser (pink)
    glColor3f(0.9f * dim_factor, 0.6f * dim_factor, 0.7f * dim_factor);
    drawRect(cx - body_width/2, cy - body_height/2 - eraser_height/2, body_width, eraser_height, 0.9f, 0.6f, 0.7f);

    // Outline (darker)
    glColor3f(0.2f * dim_factor, 0.2f * dim_factor, 0.2f * dim_factor);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx - body_width/2, cy - body_height/2 - eraser_height/2);
    glVertex2f(cx + body_width/2, cy - body_height/2 - eraser_height/2);
    glVertex2f(cx + body_width/2, cy + body_height/2 + eraser_height/2);
    glVertex2f(cx, cy + body_height/2 + tip_height + eraser_height/2); // Tip top
    glVertex2f(cx - body_width/2, cy + body_height/2 + eraser_height/2);
    glEnd();
}

// Helper to draw an eraser icon
void drawEraserIcon(float cx, float cy, float size, bool selected) {
    float dim_factor = selected ? 0.7f : 1.0f; // Dim if selected

    float half_size = size / 2.0f;
    float eraser_width = half_size * 0.8f;
    float eraser_height = half_size * 0.5f;
    float tip_height = half_size * 0.2f;

    // Main eraser body
    glColor3f(0.7f * dim_factor, 0.7f * dim_factor, 0.7f * dim_factor); // light grey
    drawRect(cx - eraser_width/2, cy - eraser_height/2, eraser_width, eraser_height, 0.7f, 0.7f, 0.7f);

    // Small contrasting tip
    glColor3f(0.9f * dim_factor, 0.5f * dim_factor, 0.5f * dim_factor); // Pinkish tip
    drawRect(cx - eraser_width/2, cy + eraser_height/2, eraser_width, tip_height, 0.9f, 0.5f, 0.5f);


    // Outline (darker grey)
    glColor3f(0.2f * dim_factor, 0.2f * dim_factor, 0.2f * dim_factor);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx - eraser_width/2, cy - eraser_height/2);
    glVertex2f(cx + eraser_width/2, cy - eraser_height/2);
    glVertex2f(cx + eraser_width/2, cy + eraser_height/2 + tip_height);
    glVertex2f(cx - eraser_width/2, cy + eraser_height/2 + tip_height);
    glEnd();
}


// Draw tool icons (in sidebar, single vertical column)
void drawToolButtons(float& current_y) {
    float start_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float btn_w = uiWidth - 2 * PADDING_X_GL; // Full width of sidebar content area
    float btn_h = BUTTON_HEIGHT_GL;
    float row_spacing = btn_h + PADDING_Y_GL; // Spacing between buttons

    current_y -= row_spacing; // Initial offset for first tool button

    // Tools: 0=brush, 1=eraser, 2=rectangle, 3=circle, 4=line, 5=fill
    // tools_order is global

    for (int i = 0; i < 6; i++) {
        int tool_idx = tools_order[i];
        float x_button_area = start_x;
        float y_button_area = current_y - i * row_spacing;

        bool selected = (currentTool == tool_idx);
        float bg_r = 0.9f, bg_g = 0.9f, bg_b = 0.9f; // Default background color for button
        float outline_r = 0.2f, outline_g = 0.2f, outline_b = 0.2f;
        if (selected) {
            bg_r = 0.75f; bg_g = 0.75f; bg_b = 0.75f; // Darker if selected
            outline_r = 0.0f; outline_g = 0.0f; outline_b = 0.0f; // Black outline when selected
        }
        drawRect(x_button_area, y_button_area, btn_w, btn_h, bg_r, bg_g, bg_b); // Draw button background
        drawRectOutline(x_button_area, y_button_area, btn_w, btn_h, outline_r, outline_g, outline_b); // Draw button outline

        float iconX = x_button_area + btn_w / 2.0f; // Center icon within the button
        float iconY = y_button_area + btn_h / 2.0f;

        float icon_r = 0.0f, icon_g = 0.0f, icon_b = 0.0f; // Default icon color for generic shapes
        float dim_factor = selected ? 0.7f : 1.0f; // Dim icon if selected

        switch (tool_idx) {
            case 0: // Brush
                drawPencilIcon(iconX, iconY, ICON_DRAW_SIZE_GL, selected);
                break;
            case 1: // Eraser
                drawEraserIcon(iconX, iconY, ICON_DRAW_SIZE_GL, selected);
                break;
            case 2: // Rectangle
                icon_r = 0.0f * dim_factor; icon_g = 0.0f * dim_factor; icon_b = 0.0f * dim_factor;
                glColor3f(icon_r, icon_g, icon_b);
                glLineWidth(2.0f);
                glBegin(GL_LINE_LOOP);
                glVertex2f(iconX - ICON_DRAW_SIZE_GL/2, iconY - ICON_DRAW_SIZE_GL/2);
                glVertex2f(iconX + ICON_DRAW_SIZE_GL/2, iconY - ICON_DRAW_SIZE_GL/2);
                glVertex2f(iconX + ICON_DRAW_SIZE_GL/2, iconY + ICON_DRAW_SIZE_GL/2);
                glVertex2f(iconX - ICON_DRAW_SIZE_GL/2, iconY + ICON_DRAW_SIZE_GL/2);
                glEnd();
                break;
            case 3: // Circle
                icon_r = 0.0f * dim_factor; icon_g = 0.0f * dim_factor; icon_b = 0.0f * dim_factor;
                drawCircle(iconX, iconY, ICON_DRAW_SIZE_GL/2, icon_r, icon_g, icon_b, false);
                break;
            case 4: // Line
                icon_r = 0.0f * dim_factor; icon_g = 0.0f * dim_factor; icon_b = 0.0f * dim_factor;
                glColor3f(icon_r, icon_g, icon_b);
                glLineWidth(2.0f);
                glBegin(GL_LINES);
                glVertex2f(iconX - ICON_DRAW_SIZE_GL/2, iconY - ICON_DRAW_SIZE_GL/2);
                glVertex2f(iconX + ICON_DRAW_SIZE_GL/2, iconY + ICON_DRAW_SIZE_GL/2);
                glEnd();
                break;
            case 5: // Fill
                icon_r = currentColor[0] * dim_factor; icon_g = currentColor[1] * dim_factor; icon_b = currentColor[2] * dim_factor;
                drawRect(iconX - ICON_DRAW_SIZE_GL/2, iconY - ICON_DRAW_SIZE_GL/2, ICON_DRAW_SIZE_GL, ICON_DRAW_SIZE_GL, icon_r, icon_g, icon_b);
                drawRectOutline(iconX - ICON_DRAW_SIZE_GL/2, iconY - ICON_DRAW_SIZE_GL/2, ICON_DRAW_SIZE_GL, ICON_DRAW_SIZE_GL, 0.2f, 0.2f, 0.2f);
                break;
        }
    }
    current_y = current_y - 5 * row_spacing - SECTION_PADDING_Y_GL; // Update y for next section (6 rows total)
}

// Helper function to calculate slider Y positions
struct SliderYPositions {
    float brushSliderBottomY;
    float eraserSliderBottomY;
};

SliderYPositions getSliderYPositions() {
    float current_y_tracker = CANVAS_TOP_GL - PADDING_Y_GL;

    // Skip tool buttons
    current_y_tracker -= (BUTTON_HEIGHT_GL + PADDING_Y_GL) * 6;
    current_y_tracker -= SECTION_PADDING_Y_GL;

    // Skip color preview
    current_y_tracker -= (SLIDER_HEIGHT_GL * 1.5f + PADDING_Y_GL);

    // Skip RGB sliders
    current_y_tracker -= (SLIDER_HEIGHT_GL * 3 + SLIDER_VERTICAL_SPACING_GL * 2 + SECTION_PADDING_Y_GL);

    // Now, current_y_tracker is the top of the size section (before labels)

    float label_text_height = 0.003f * 1.0f; // Estimated height of text
    float size_slider_h = SLIDER_HEIGHT_GL;

    // Brush Size Slider
    float brush_label_top_y = current_y_tracker - (label_text_height + PADDING_Y_GL);
    float brush_slider_bottom_y = brush_label_top_y - size_slider_h;

    // Eraser Size Slider
    float eraser_label_top_y = brush_slider_bottom_y - SLIDER_VERTICAL_SPACING_GL - (label_text_height + PADDING_Y_GL);
    float eraser_slider_bottom_y = eraser_label_top_y - size_slider_h;

    return {brush_slider_bottom_y, eraser_slider_bottom_y};
}


// Draw brush/eraser size selectors (in sidebar, stacked)
void drawSizeSelectorsSidebar(float& current_y) {
    float x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float w = uiWidth - 2 * PADDING_X_GL;
    float h = SLIDER_HEIGHT_GL;
    float label_text_height = 0.003f * 1.0f; // Estimated height of text

    SliderYPositions y_pos = getSliderYPositions(); // Get precise Y positions

    // --- Brush Size Slider ---
    // Brush size label
    drawText(x + PADDING_X_GL, current_y - (label_text_height + PADDING_Y_GL), "PENCIL SIZE", 0.2f, 0.2f, 0.2f, 0.003f);
    current_y -= (label_text_height + PADDING_Y_GL);

    // Brush size slider
    float slider_top_y_brush = y_pos.brushSliderBottomY; // Use the calculated bottom Y
    drawRect(x, slider_top_y_brush, w, h, 0.8f, 0.8f, 0.8f);
    drawRectOutline(x, slider_top_y_brush, w, h, 0.2f, 0.2f, 0.2f);
    // Draw thumb in black with white outline
    drawRect(x + (brushSize - 1.0f) / 19.0f * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_brush, SLIDER_THUMB_WIDTH_GL, h, 0.0f, 0.0f, 0.0f);
    drawRectOutline(x + (brushSize - 1.0f) / 19.0f * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_brush, SLIDER_THUMB_WIDTH_GL, h, 1.0f, 1.0f, 1.0f);
    current_y = slider_top_y_brush - SLIDER_VERTICAL_SPACING_GL; // Update for next element

    // --- Eraser Size Slider ---
    // Eraser size label
    drawText(x + PADDING_X_GL, current_y - (label_text_height + PADDING_Y_GL), "ERASER SIZE", 0.2f, 0.2f, 0.2f, 0.003f);
    current_y -= (label_text_height + PADDING_Y_GL);

    // Eraser size slider
    float slider_top_y_eraser = y_pos.eraserSliderBottomY; // Use the calculated bottom Y
    drawRect(x, slider_top_y_eraser, w, h, 0.9f, 0.9f, 0.9f);
    drawRectOutline(x, slider_top_y_eraser, w, h, 0.2f, 0.2f, 0.2f);
    // Draw thumb in black with white outline
    drawRect(x + (eraserSize - 1.0f) / 19.0f * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_eraser, SLIDER_THUMB_WIDTH_GL, h, 0.0f, 0.0f, 0.0f);
    drawRectOutline(x + (eraserSize - 1.0f) / 19.0f * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_eraser, SLIDER_THUMB_WIDTH_GL, h, 1.0f, 1.0f, 1.0f);
    
    current_y = slider_top_y_eraser - h - PADDING_Y_GL; // Update y for next section
}


// Draw clear button (in top bar, rightmost side)
void drawClearButtonTopBar() {
    float btn_w = BUTTON_HEIGHT_GL * 1.5f; // Make it a bit wider
    float btn_h = BUTTON_HEIGHT_GL;
    float x = 1.0f - PADDING_X_GL - btn_w; // Rightmost side
    float y = 1.0f - PADDING_Y_GL - btn_h; // Top bar, aligned with color swatches

    drawButton(x, y, btn_w, btn_h, 0.9f, 0.3f, 0.3f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2f(x + btn_w * 0.25f, y + btn_h * 0.25f);
    glVertex2f(x + btn_w * 0.75f, y + btn_h * 0.75f);
    glVertex2f(x + btn_w * 0.25f, y + btn_h * 0.75f);
    glVertex2f(x + btn_w * 0.75f, y + btn_h * 0.25f);
    glEnd();
}


// Draw all strokes
void drawStrokes() {
    for (const auto& stroke : strokes) {
        if (stroke.tool == 5) { // Fill tool (can be rectangle or circle fill)
            glColor3f(stroke.fillColor[0], stroke.fillColor[1], stroke.fillColor[2]);
            if (stroke.circleRadius > 0) { // Filled circle
                drawCircle(stroke.circleCenter.x, stroke.circleCenter.y, stroke.circleRadius,
                           stroke.fillColor[0], stroke.fillColor[1], stroke.fillColor[2], true);
            } else { // Filled rectangle (default for tool 5 if not a circle)
                // Ensure the rectangle is drawn correctly regardless of start/end point order
                float minX = std::min(stroke.rectStart.x, stroke.rectEnd.x);
                float maxX = std::max(stroke.rectStart.x, stroke.rectEnd.x);
                float minY = std::min(stroke.rectStart.y, stroke.rectEnd.y);
                float maxY = std::max(stroke.rectStart.y, stroke.rectEnd.y);
                drawRect(minX, minY, maxX - minX, maxY - minY,
                         stroke.fillColor[0], stroke.fillColor[1], stroke.fillColor[2]);
            }
            continue; // Skip point/line drawing for fill strokes
        }

        // Determine color for non-fill strokes (brush, eraser, shapes)
        if (stroke.tool == 1) { // Eraser
            glColor3f(1.0f, 1.0f, 1.0f); // Eraser always draws white (canvas background)
        } else { // Brush or Shapes (tool 0, 2, 3, 4)
            // For brush, points store their color. For shapes, all points in the stroke
            // were added with the same currentColor at creation.
            // So, we use the color from the first point.
            if (!stroke.points.empty()) {
                glColor3f(stroke.points[0].r, stroke.points[0].g, stroke.points[0].b);
            } else {
                glColor3f(0.0f, 0.0f, 0.0f); // Fallback to black if no points (shouldn't happen for shapes)
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
                glBegin(GL_LINE_LOOP); // Circles are drawn as line loops
            } else if (stroke.tool == 4) { // Line
                glBegin(GL_LINES);
            } else { // Brush/Eraser (line strip)
                glBegin(GL_LINE_STRIP);
            }
            for (const auto& point : stroke.points) {
                glVertex2f(point.x, point.y);
            }
            glEnd();
        }
    }
}

// Draw current stroke being drawn (for brush/eraser preview)
void drawCurrentStroke() {
    if (currentStroke.points.empty()) return;
    
    if (currentStroke.tool == 1) { // Eraser
        glColor3f(1.0f, 1.0f, 1.0f); // Eraser always draws white (canvas background)
    } else { // Brush preview
        glColor3f(currentColor[0], currentColor[1], currentColor[2]); // Use current selected color
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

// Draw preview shapes (for rectangle, circle, line, and fill tool)
void drawShapePreview() {
    // Only draw preview if drawing is active and it's a shape or fill tool
    if (!isDrawing || (currentTool < 2 && currentTool != 5) || currentTool > 5) return; 
    
    // Only draw preview if there's actual movement from the start point
    if (std::abs(shapeStart.x - shapeEnd.x) < 0.001f && std::abs(shapeStart.y - shapeEnd.y) < 0.001f) {
        return; // No movement, no preview
    }

    glColor3f(currentColor[0], currentColor[1], currentColor[2]); // Shapes always use the selected currentColor
    glLineWidth(brushSize / 2.0f); // Use brush size for shape outlines

    switch (currentTool) {
        case 2: // Rectangle
        case 5: // Fill (Rectangle Preview) - Note: Fill tool no longer draws new rectangles, but this case is kept for consistency if it were to revert.
            glBegin(GL_LINE_LOOP);
            glVertex2f(shapeStart.x, shapeStart.y);
            glVertex2f(shapeEnd.x, shapeStart.y);
            glVertex2f(shapeEnd.x, shapeEnd.y);
            glVertex2f(shapeStart.x, shapeEnd.y);
            glEnd();
            break;
        case 3: // Circle
            {
                float radius = std::sqrt(std::pow(shapeEnd.x - shapeStart.x, 2) + std::pow(shapeEnd.y - shapeStart.y, 2));
                drawCircle(shapeStart.x, shapeStart.y, radius, currentColor[0], currentColor[1], currentColor[2], false);
            }
            break;
        case 4: // Line
            glBegin(GL_LINES);
            glVertex2f(shapeStart.x, shapeStart.y);
            glVertex2f(shapeEnd.x, shapeEnd.y);
            glEnd();
            break;
    }
}

// Mouse button callback
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);

    if (action == GLFW_PRESS) {
        bool handledClick = false; // Flag to indicate if any UI element was clicked

        // --- Handle Left-Click ---
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            // Check Top Bar clicks first
            if (isInTopBar(xpos, ypos)) {
                // Preset color palette click detection
                float colors[][3] = {
                    {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                    {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}
                };
                float swatch_y = 1.0f - PADDING_Y_GL - COLOR_SWATCH_SIZE_GL; // Y for the row of swatches
                float swatch_start_x = -1.0f + PADDING_X_GL;
                float swatch_spacing_x = COLOR_SWATCH_SIZE_GL + PADDING_X_GL / 2.0f;

                for (int i = 0; i < 8; i++) {
                    float x = swatch_start_x + i * swatch_spacing_x;
                    if (glX >= x && glX <= x + COLOR_SWATCH_SIZE_GL && glY >= swatch_y && glY <= swatch_y + COLOR_SWATCH_SIZE_GL) {
                        currentColor[0] = colors[i][0]; currentColor[1] = colors[i][1]; currentColor[2] = colors[i][2];
                        std::memcpy(customColor, currentColor, sizeof(customColor)); // Keep custom color in sync
                        handledClick = true;
                        break;
                    }
                }

                // Clear button click detection
                float clear_btn_w = BUTTON_HEIGHT_GL * 1.5f;
                float clear_btn_h = BUTTON_HEIGHT_GL;
                float clear_btn_x = 1.0f - PADDING_X_GL - clear_btn_w;
                float clear_btn_y = 1.0f - PADDING_Y_GL - clear_btn_h;
                if (glX >= clear_btn_x && glX <= clear_btn_x + clear_btn_w && glY >= clear_btn_y && glY <= clear_btn_y + clear_btn_h) {
                    strokes.clear();
                    handledClick = true;
                }
            }

            // Check Sidebar clicks
            if (!handledClick && isInSidebar(xpos, ypos)) {
                // Tool buttons click detection (left click)
                float tool_start_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
                float tool_btn_w = uiWidth - 2 * PADDING_X_GL;
                float tool_btn_h = BUTTON_HEIGHT_GL;
                float tool_row_spacing = tool_btn_h + PADDING_Y_GL;

                float current_y_click_tracker_for_tools = CANVAS_TOP_GL - PADDING_Y_GL - tool_row_spacing;

                for (int i = 0; i < 6; i++) {
                    int tool_idx = tools_order[i];
                    float x_button_area = tool_start_x;
                    float y_button_area = current_y_click_tracker_for_tools - i * tool_row_spacing;

                    if (glX >= x_button_area && glX <= x_button_area + tool_btn_w &&
                        glY >= y_button_area && glY <= y_button_area + tool_btn_h) {
                        currentTool = tool_idx;
                        handledClick = true;
                        break;
                    }
                }

                // Color sliders click detection (sidebar)
                if (!handledClick) {
                    float color_slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
                    float color_slider_w = uiWidth - 2 * PADDING_X_GL;
                    float color_slider_h = SLIDER_HEIGHT_GL;
                    float preview_h = SLIDER_HEIGHT_GL * 1.5f;

                    float color_section_start_y = CANVAS_TOP_GL - PADDING_Y_GL; // Start of sidebar elements
                    color_section_start_y -= (tool_row_spacing * 6); // Skip tool buttons
                    color_section_start_y -= SECTION_PADDING_Y_GL; // After tools section

                    float slider_top_y_r = color_section_start_y - preview_h - PADDING_Y_GL - color_slider_h;
                    float slider_top_y_g = slider_top_y_r - (color_slider_h + SLIDER_VERTICAL_SPACING_GL);
                    float slider_top_y_b = slider_top_y_g - (color_slider_h + SLIDER_VERTICAL_SPACING_GL);

                    if (glX >= color_slider_x && glX <= color_slider_x + color_slider_w) {
                        if (glY >= slider_top_y_r && glY <= slider_top_y_r + color_slider_h) {
                            float clamped_glX = std::max(color_slider_x, std::min(color_slider_x + color_slider_w, glX));
                            customColor[0] = (clamped_glX - color_slider_x) / (color_slider_w - SLIDER_THUMB_WIDTH_GL);
                            currentColor[0] = customColor[0]; handledClick = true;
                        } else if (glY >= slider_top_y_g && glY <= slider_top_y_g + color_slider_h) {
                            float clamped_glX = std::max(color_slider_x, std::min(color_slider_x + color_slider_w, glX));
                            customColor[1] = (clamped_glX - color_slider_x) / (color_slider_w - SLIDER_THUMB_WIDTH_GL);
                            currentColor[1] = customColor[1]; handledClick = true;
                        } else if (glY >= slider_top_y_b && glY <= slider_top_y_b + color_slider_h) {
                            float clamped_glX = std::max(color_slider_x, std::min(color_slider_x + color_slider_w, glX));
                            customColor[2] = (clamped_glX - color_slider_x) / (color_slider_w - SLIDER_THUMB_WIDTH_GL);
                            currentColor[2] = customColor[2]; handledClick = true;
                        }
                    }
                }

                // Size sliders click detection (sidebar)
                if (!handledClick) {
                    float size_slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
                    float size_slider_w = uiWidth - 2 * PADDING_X_GL;
                    float size_slider_h = SLIDER_HEIGHT_GL;

                    SliderYPositions y_pos = getSliderYPositions(); // Use the helper function

                    if (glX >= size_slider_x && glX <= size_slider_x + size_slider_w) {
                        if (glY >= y_pos.brushSliderBottomY && glY <= y_pos.brushSliderBottomY + size_slider_h) {
                            float clamped_glX = std::max(size_slider_x, std::min(size_slider_x + size_slider_w, glX));
                            brushSize = 1.0f + ((clamped_glX - size_slider_x) / (size_slider_w)) * 19.0f;
                            brushSize = std::max(1.0f, std::min(20.0f, brushSize));
                            isDraggingBrushSlider = true; // Start dragging
                            handledClick = true;
                        } else if (glY >= y_pos.eraserSliderBottomY && glY <= y_pos.eraserSliderBottomY + size_slider_h) {
                            float clamped_glX = std::max(size_slider_x, std::min(size_slider_x + size_slider_w, glX));
                            eraserSize = 1.0f + ((clamped_glX - size_slider_x) / (size_slider_w)) * 19.0f;
                            eraserSize = std::max(1.0f, std::min(20.0f, eraserSize));
                            isDraggingEraserSlider = true; // Start dragging
                            handledClick = true;
                        }
                    }
                }
            }

            // Start drawing on canvas if click was on canvas and not handled by UI
            if (!handledClick && isInCanvas(glX, glY)) {
                if (currentTool == 5) { // Handle Fill tool specifically
                    bool filledExistingShape = false;
                    // Iterate strokes in reverse to fill the topmost shape
                    for (int i = strokes.size() - 1; i >= 0; --i) {
                        const Stroke& existingStroke = strokes[i];
                        if (existingStroke.tool == 2) { // Check if it's a rectangle
                            // Normalize rectangle coordinates for easier checking
                            // The points vector for a rectangle stroke contains 5 points forming a loop:
                            // [start, top-right, end, bottom-left, start]
                            // So, existingStroke.points[0] is one corner, and existingStroke.points[2] is the opposite corner.
                            float minX = std::min(existingStroke.points[0].x, existingStroke.points[2].x);
                            float maxX = std::max(existingStroke.points[0].x, existingStroke.points[2].x);
                            float minY = std::min(existingStroke.points[0].y, existingStroke.points[2].y);
                            float maxY = std::max(existingStroke.points[0].y, existingStroke.points[2].y);

                            if (glX >= minX && glX <= maxX && glY >= minY && glY <= maxY) {
                                // Click is inside this rectangle, create a fill stroke for it
                                Stroke fillStroke;
                                fillStroke.tool = 5;
                                fillStroke.fillColor[0] = currentColor[0];
                                fillStroke.fillColor[1] = currentColor[1];
                                fillStroke.fillColor[2] = currentColor[2];
                                fillStroke.rectStart = Point(minX, minY);
                                fillStroke.rectEnd = Point(maxX, maxY);
                                fillStroke.circleRadius = 0; // Indicate it's a rectangle fill
                                strokes.push_back(fillStroke);
                                filledExistingShape = true;
                                handledClick = true;
                                break; // Only fill one shape
                            }
                        } else if (existingStroke.tool == 3) { // Check if it's a circle
                            if (existingStroke.circleRadius > 0) { // Ensure it's a valid circle stroke
                                float dist_sq = std::pow(glX - existingStroke.circleCenter.x, 2) + std::pow(glY - existingStroke.circleCenter.y, 2);
                                if (dist_sq <= std::pow(existingStroke.circleRadius, 2)) {
                                    // Click is inside this circle, create a fill stroke for it
                                    Stroke fillStroke;
                                    fillStroke.tool = 5;
                                    fillStroke.fillColor[0] = currentColor[0];
                                    fillStroke.fillColor[1] = currentColor[1];
                                    fillStroke.fillColor[2] = currentColor[2];
                                    fillStroke.circleCenter = existingStroke.circleCenter;
                                    fillStroke.circleRadius = existingStroke.circleRadius;
                                    // rectStart/End are not used for circles, but initialize to safe values
                                    fillStroke.rectStart = Point(0,0);
                                    fillStroke.rectEnd = Point(0,0);
                                    strokes.push_back(fillStroke);
                                    filledExistingShape = true;
                                    handledClick = true;
                                    break; // Only fill one shape
                                }
                            }
                        }
                    }
                    // If fill tool was active but no existing shape was clicked, it does nothing.
                    // This prevents the fill tool from starting a brush-like stroke on canvas click.
                    isDrawing = false; // Ensure isDrawing is false if fill tool is active and no shape is clicked
                } else { // Handle other tools (brush, eraser, new shapes)
                    isDrawing = true;
                    if (currentTool < 2) { // Brush or Eraser
                        currentStroke.points.clear();
                        currentStroke.tool = currentTool;
                        currentStroke.size = (currentTool == 0) ? brushSize : eraserSize;
                        float clampedGlX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, glX));
                        float clampedGlY = std::max(-1.0f, std::min(CANVAS_TOP_GL, glY));
                        shapeStart = Point(clampedGlX, clampedGlY, currentColor[0], currentColor[1], currentColor[2]); // Use shapeStart as initial point for brush/eraser too
                        shapeEnd = shapeStart; // Initialize shapeEnd to shapeStart for all drawing types
                    } else if (currentTool >= 2 && currentTool <= 4) { // Shapes (Rectangle, Circle, Line)
                        float clampedGlX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, glX));
                        float clampedGlY = std::max(-1.0f, std::min(CANVAS_TOP_GL, glY));
                        shapeStart = Point(clampedGlX, clampedGlY, currentColor[0], currentColor[1], currentColor[2]);
                        shapeEnd = shapeStart;
                    }
                }
            }
        }
    } else if (action == GLFW_RELEASE) {
        // Stop dragging any slider
        isDraggingBrushSlider = false;
        isDraggingEraserSlider = false;

        if (isDrawing) {
            if (currentTool < 2) { // Brush or Eraser
                // Only add stroke if there was actual movement (more than 0 points after initial press)
                if (currentStroke.points.size() > 0) { 
                    strokes.push_back(currentStroke);
                }
                currentStroke.points.clear(); // Clear current stroke points regardless
            } else if (currentTool >= 2 && currentTool <= 4) { // Shapes (Rectangle, Circle, Line)
                Stroke newStroke;
                newStroke.tool = currentTool;
                newStroke.size = brushSize; // Shapes use brush size for outline
                
                // Ensure shapeEnd is within canvas if drawing ended outside
                float finalGlX, finalGlY;
                screenToGL(xpos, ypos, finalGlX, finalGlY);

                // Clamp final coordinates to canvas boundary
                finalGlX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, finalGlX));
                finalGlY = std::max(-1.0f, std::min(CANVAS_TOP_GL, finalGlY)); // Clamped by TOP_BAR
                shapeEnd = Point(finalGlX, finalGlY, currentColor[0], currentColor[1], currentColor[2]);
                
                // Only add shape stroke if there was significant drag
                if (std::abs(shapeStart.x - shapeEnd.x) > 0.001f || std::abs(shapeStart.y - shapeEnd.y) > 0.001f) {
                    switch (currentTool) {
                        case 2: // Rectangle
                            newStroke.points.push_back(Point(shapeStart.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2]));
                            newStroke.points.push_back(Point(shapeEnd.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2]));
                            newStroke.points.push_back(Point(shapeEnd.x, shapeEnd.y, currentColor[0], currentColor[1], currentColor[2]));
                            newStroke.points.push_back(Point(shapeStart.x, shapeEnd.y, currentColor[0], currentColor[1], currentColor[2]));
                            newStroke.points.push_back(Point(shapeStart.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2])); // Close loop
                            break;
                        case 3: // Circle
                            {
                                float radius = std::sqrt(std::pow(shapeEnd.x - shapeStart.x, 2) + std::pow(shapeEnd.y - shapeStart.y, 2));
                                newStroke.circleCenter = shapeStart;
                                newStroke.circleRadius = radius;
                                // Generate points for circle outline
                                for (int i = 0; i <= 360; i += 5) { // More steps for smoother circle
                                    float angle = i * 3.1415926f / 180.0f;
                                    float x_pt = shapeStart.x + radius * std::cos(angle);
                                    float y_pt = shapeStart.y + radius * std::sin(angle);
                                    if (isInCanvas(x_pt, y_pt)) // Only add points within canvas
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

// Mouse movement callback
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);

    float size_slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float size_slider_w = uiWidth - 2 * PADDING_X_GL;
    float size_slider_h = SLIDER_HEIGHT_GL;

    SliderYPositions y_pos = getSliderYPositions(); // Use the helper function

    // Handle slider dragging
    if (isDraggingBrushSlider) {
        float clamped_glX = std::max(size_slider_x, std::min(size_slider_x + size_slider_w, glX));
        brushSize = 1.0f + ((clamped_glX - size_slider_x) / (size_slider_w)) * 19.0f;
        brushSize = std::max(1.0f, std::min(20.0f, brushSize));
    } else if (isDraggingEraserSlider) {
        float clamped_glX = std::max(size_slider_x, std::min(size_slider_x + size_slider_w, glX));
        eraserSize = 1.0f + ((clamped_glX - size_slider_x) / (size_slider_w)) * 19.0f;
        eraserSize = std::max(1.0f, std::min(20.0f, eraserSize));
    }
    // Handle drawing on canvas
    else if (isDrawing && !isInSidebar(xpos, ypos) && !isInTopBar(xpos,ypos)) { // Ensure not in sidebar or top bar
        // Clamp cursor position to canvas boundaries for drawing
        glX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, glX));
        glY = std::max(-1.0f, std::min(CANVAS_TOP_GL, glY)); // Clamped by TOP_BAR

        if (currentTool < 2) { // Brush or Eraser
            // For brush/eraser, add points only when moving
            if (currentStroke.points.empty()) { // If this is the very first point after a press (shapeStart is the press point)
                currentStroke.points.push_back(shapeStart); // Add the initial click point
            }
            currentStroke.points.push_back(Point(glX, glY, currentColor[0], currentColor[1], currentColor[2]));
        } else if (currentTool >= 2 && currentTool <= 5) { // Shapes or Fill
            shapeEnd = Point(glX, glY, currentColor[0], currentColor[1], currentColor[2]);
        }
    }
}

// Scroll callback for brush size
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Get current mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);

    float size_slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float size_slider_w = uiWidth - 2 * PADDING_X_GL;
    float size_slider_h = SLIDER_HEIGHT_GL;

    SliderYPositions y_pos = getSliderYPositions(); // Use the helper function

    // Check if mouse is over Brush Size Slider
    if (glX >= size_slider_x && glX <= size_slider_x + size_slider_w &&
        glY >= y_pos.brushSliderBottomY && glY <= y_pos.brushSliderBottomY + size_slider_h) {
        brushSize += yoffset * 2.0f;
        brushSize = std::max(1.0f, std::min(20.0f, brushSize));
    }
    // Check if mouse is over Eraser Size Slider
    else if (glX >= size_slider_x && glX <= size_slider_x + size_slider_w &&
        glY >= y_pos.eraserSliderBottomY && glY <= y_pos.eraserSliderBottomY + size_slider_h) {
        eraserSize += yoffset * 2.0f;
        eraserSize = std::max(1.0f, std::min(20.0f, eraserSize));
    }
}

// Keyboard callback for undo
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        // Check for Ctrl (Windows/Linux) or Command (macOS)
        if (mods & GLFW_MOD_CONTROL || mods & GLFW_MOD_SUPER) {
            if (!strokes.empty()) {
                strokes.pop_back(); // Remove the last stroke
            }
        }
    }
}

// Render function
void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    float current_y_position; // Tracks the current y position for drawing UI elements from top to bottom

    // Draw top bar background (full width)
    drawRect(-1.0f, CANVAS_TOP_GL, 2.0f, TOP_BAR_HEIGHT_GL, 0.85f, 0.85f, 0.85f);
    drawRectOutline(-1.0f, CANVAS_TOP_GL, 2.0f, TOP_BAR_HEIGHT_GL, 0.7f, 0.7f, 0.7f);

    // Draw sidebar background (below top bar)
    drawRect(SIDEBAR_LEFT_GL, -1.0f, uiWidth, 1.0f - CANVAS_TOP_GL, 0.93f, 0.93f, 0.93f);
    drawRectOutline(SIDEBAR_LEFT_GL, -1.0f, uiWidth, 1.0f - CANVAS_TOP_GL, 0.7f, 0.7f, 0.7f);

    // Draw canvas area (below top bar, right of sidebar)
    drawRect(SIDEBAR_RIGHT_GL, -1.0f, 2.0f - uiWidth, 1.0f - CANVAS_TOP_GL, 1.0f, 1.0f, 1.0f); // White background
    // Draw a proper black border for the canvas
    glColor3f(0.0f, 0.0f, 0.0f); // Black color for the border
    glLineWidth(2.0f); // Make the border thicker
    glBegin(GL_LINE_LOOP);
    glVertex2f(SIDEBAR_RIGHT_GL, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, CANVAS_TOP_GL);
    glVertex2f(SIDEBAR_RIGHT_GL, CANVAS_TOP_GL);
    glEnd();
    glLineWidth(1.0f); // Reset line width for other drawings (important!)
    
    // Draw Top Bar UI elements (Preset Colors, Clear Button)
    drawPresetColorPalette();
    drawClearButtonTopBar();

    // Draw Sidebar UI elements (Tools, Color Sliders, Size Sliders)
    current_y_position = CANVAS_TOP_GL - PADDING_Y_GL; // Starting Y for sidebar elements
    drawToolButtons(current_y_position);
    drawColorSlidersSidebar(current_y_position); // Now drawn in the sidebar
    drawSizeSelectorsSidebar(current_y_position); // Now drawn in the sidebar

    drawStrokes();
    drawCurrentStroke();
    drawShapePreview();
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    // Create window
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "SketchMate", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    // Set callbacks
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback); // Register the new key callback
    
    // Set clear color to white (for the entire window background)
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 

    // Enable point and line smoothing for better drawing quality
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND); // Enable blending for smooth lines/points
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard blending function

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        glViewport(0, 0, windowWidth, windowHeight);
        render();
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}
