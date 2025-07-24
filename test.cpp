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
    Point rectStart, rectEnd; // For fill tool (used for rectangle fill)
    Point circleCenter; // For circle fill
    float circleRadius; // For circle fill
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
bool showFillDialog = false; // This is a flag to trigger the fill action
Point shapeStart, shapeEnd; // For shape drawing preview
int windowWidth = 1000, windowHeight = 700;
float uiWidth = 0.22f; // Adjusted sidebar width for better aesthetics

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
    drawRect(x + customColor[0] * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_r, SLIDER_THUMB_WIDTH_GL, h, 0.0f, 0.0f, 0.0f);
    drawRectOutline(x, slider_top_y_r, w, h, 0.2f, 0.2f, 0.2f);
    current_y = slider_top_y_r - SLIDER_VERTICAL_SPACING_GL; // Update current_y for next slider

    // G Slider with gradient
    float slider_top_y_g = current_y - h;
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_top_y_g);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(x + w, slider_top_y_g);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(x + w, slider_top_y_g + h);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_top_y_g + h);
    glEnd();
    drawRect(x + customColor[1] * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_g, SLIDER_THUMB_WIDTH_GL, h, 0.0f, 0.0f, 0.0f);
    drawRectOutline(x, slider_top_y_g, w, h, 0.2f, 0.2f, 0.2f);
    current_y = slider_top_y_g - SLIDER_VERTICAL_SPACING_GL; // Update current_y for next slider

    // B Slider with gradient
    float slider_top_y_b = current_y - h;
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_top_y_b);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(x + w, slider_top_y_b);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(x + w, slider_top_y_b + h);
    glColor3f(0.0f, 0.0f, 0.0f); glVertex2f(x, slider_top_y_b + h);
    glEnd();
    drawRect(x + customColor[2] * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_b, SLIDER_THUMB_WIDTH_GL, h, 0.0f, 0.0f, 0.0f);
    drawRectOutline(x, slider_top_y_b, w, h, 0.2f, 0.2f, 0.2f);
    
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

// Draw brush/eraser size selectors (in sidebar, stacked)
void drawSizeSelectorsSidebar(float& current_y) {
    float x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float w = uiWidth - 2 * PADDING_X_GL;
    float h = SLIDER_HEIGHT_GL;
    float label_text_height = 0.003f * 1.0f;

    // Brush size label
    drawText(x + PADDING_X_GL, current_y - (label_text_height + PADDING_Y_GL), "BRUSH SIZE", 0.2f, 0.2f, 0.2f, 0.003f);
    current_y -= (label_text_height + PADDING_Y_GL);

    // Brush size slider
    float slider_top_y_brush = current_y - h;
    drawRect(x, slider_top_y_brush, w, h, 0.8f, 0.8f, 0.8f);
    drawRectOutline(x, slider_top_y_brush, w, h, 0.2f, 0.2f, 0.2f);
    drawRect(x + (brushSize - 1.0f) / 19.0f * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_brush, SLIDER_THUMB_WIDTH_GL, h, 0.0f, 0.0f, 0.0f);
    current_y = slider_top_y_brush - SLIDER_VERTICAL_SPACING_GL;

    // Eraser size label
    drawText(x + PADDING_X_GL, current_y - (label_text_height + PADDING_Y_GL), "ERASER SIZE", 0.2f, 0.2f, 0.2f, 0.003f);
    current_y -= (label_text_height + PADDING_Y_GL);

    // Eraser size slider
    float slider_top_y_eraser = current_y - h;
    drawRect(x, slider_top_y_eraser, w, h, 0.9f, 0.9f, 0.9f);
    drawRectOutline(x, slider_top_y_eraser, w, h, 0.2f, 0.2f, 0.2f);
    drawRect(x + (eraserSize - 1.0f) / 19.0f * (w - SLIDER_THUMB_WIDTH_GL), slider_top_y_eraser, SLIDER_THUMB_WIDTH_GL, h, 0.0f, 0.0f, 0.0f);
    
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
        if (stroke.tool == 5) { // Fill tool
            glColor3f(stroke.fillColor[0], stroke.fillColor[1], stroke.fillColor[2]);
            if (stroke.rectEnd.x != stroke.rectStart.x || stroke.rectEnd.y != stroke.rectStart.y) { // Filled rectangle
                drawRect(stroke.rectStart.x, stroke.rectStart.y,
                         stroke.rectEnd.x - stroke.rectStart.x,
                         stroke.rectEnd.y - stroke.rectStart.y,
                         stroke.fillColor[0], stroke.fillColor[1], stroke.fillColor[2]);
            } else if (stroke.circleRadius > 0) { // Filled circle
                drawCircle(stroke.circleCenter.x, stroke.circleCenter.y, stroke.circleRadius,
                           stroke.fillColor[0], stroke.fillColor[1], stroke.fillColor[2], true);
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

// Draw current stroke being drawn
void drawCurrentStroke() {
    if (currentStroke.points.empty()) return;
    
    if (currentStroke.tool == 1) { // Eraser
        glColor3f(1.0f, 1.0f, 1.0f); // Eraser always draws white (canvas background)
    } else { // Brush or Shape preview
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

// Draw preview shapes
void drawShapePreview() {
    if (!isDrawing || currentTool < 2 || currentTool > 4) return;
    glColor3f(currentColor[0], currentColor[1], currentColor[2]); // Shapes always use the selected currentColor
    glLineWidth(brushSize / 2.0f);
    switch (currentTool) {
        case 2: // Rectangle
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

// Fill callback: fills within the last drawn shape (rectangle or circle)
void fillLastShapeWithColor() {
    // Find the last actual shape (rectangle or circle)
    for (int i = strokes.size() - 1; i >= 0; --i) {
        const Stroke& last = strokes[i];
        if (last.tool == 2 || last.tool == 3) { // Found a rectangle or circle
            Stroke fillStroke;
            fillStroke.tool = 5; // Set tool to fill
            fillStroke.size = 1.0f; // Size doesn't matter for fill
            fillStroke.fillColor[0] = currentColor[0];
            fillStroke.fillColor[1] = currentColor[1];
            fillStroke.fillColor[2] = currentColor[2];

            if (last.tool == 2) { // Rectangle
                fillStroke.rectStart = Point(std::min(last.points[0].x, last.points[2].x), std::min(last.points[0].y, last.points[2].y));
                fillStroke.rectEnd = Point(std::max(last.points[0].x, last.points[2].x), std::max(last.points[0].y, last.points[2].y));
                fillStroke.points.clear(); // No points needed for filled rect
                strokes.push_back(fillStroke);
            } else if (last.tool == 3) { // Circle
                // For a drawn circle, points[0] is center, and distance to any other point is radius
                if (!last.points.empty()) {
                    fillStroke.circleCenter = last.points[0];
                    if (last.points.size() > 1) {
                        fillStroke.circleRadius = std::sqrt(std::pow(last.points[1].x - last.points[0].x, 2) + std::pow(last.points[1].y - last.points[0].y, 2));
                    } else {
                        fillStroke.circleRadius = 0; // Or some default small radius if only center point exists
                    }
                    fillStroke.points.clear(); // No points needed for filled circle
                    strokes.push_back(fillStroke);
                }
            }
            return; // Only fill the most recent shape
        }
    }
}


// Mouse button callback
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        float glX, glY;
        screenToGL(xpos, ypos, glX, glY);

        if (action == GLFW_PRESS) {
            // Check Top Bar clicks
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
                        return; // Handled click
                    }
                }

                // Clear button click detection (top right)
                float clear_btn_w = BUTTON_HEIGHT_GL * 1.5f;
                float clear_btn_h = BUTTON_HEIGHT_GL;
                float clear_btn_x = 1.0f - PADDING_X_GL - clear_btn_w;
                float clear_btn_y = 1.0f - PADDING_Y_GL - clear_btn_h;
                if (glX >= clear_btn_x && glX <= clear_btn_x + clear_btn_w && glY >= clear_btn_y && glY <= clear_btn_y + clear_btn_h) {
                    strokes.clear(); return;
                }

            } else if (isInSidebar(xpos, ypos)) { // Click in Sidebar (below top bar)
                // Track current_y_click to match UI layout in render()
                float current_y_click_tracker = CANVAS_TOP_GL - PADDING_Y_GL;

                // Tool buttons click detection (single column)
                float tool_start_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
                float tool_btn_w = uiWidth - 2 * PADDING_X_GL; // Full width of the button area
                float tool_btn_h = BUTTON_HEIGHT_GL; // Height of the button area
                float tool_row_spacing = tool_btn_h + PADDING_Y_GL;

                current_y_click_tracker -= tool_row_spacing; // Initial offset for first tool button

                for (int i = 0; i < 6; i++) {
                    int tool_idx = tools_order[i];
                    float x_button_area = tool_start_x;
                    float y_button_area = current_y_click_tracker - i * tool_row_spacing;

                    // Check if click is within the entire button's rectangular area
                    if (glX >= x_button_area && glX <= x_button_area + tool_btn_w &&
                        glY >= y_button_area && glY <= y_button_area + tool_btn_h) {
                        currentTool = tool_idx;
                        if (currentTool == 5) { // Fill tool
                            showFillDialog = true; // Set flag to perform fill on next render cycle
                        }
                        return;
                    }
                }
                current_y_click_tracker = current_y_click_tracker - 5 * tool_row_spacing - SECTION_PADDING_Y_GL; // After tools section

                // Color sliders click detection (now in sidebar, stacked)
                float color_slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
                float color_slider_w = uiWidth - 2 * PADDING_X_GL;
                float color_slider_h = SLIDER_HEIGHT_GL;
                float preview_h = SLIDER_HEIGHT_GL * 1.5f;

                float color_section_start_y = current_y_click_tracker; // This is the 'current_y_position' from render() after tools
                
                // Check preview swatch
                float preview_y_check = color_section_start_y - preview_h;
                if (glX >= color_slider_x && glX <= color_slider_x + color_slider_w &&
                    glY >= preview_y_check && glY <= preview_y_check + preview_h) {
                    // Clicked on preview, no action needed, just don't fall through to sliders
                    return;
                }

                color_section_start_y -= (preview_h + PADDING_Y_GL); // Adjust for first actual slider
                
                float slider_top_y_r = color_section_start_y - color_slider_h;
                float slider_top_y_g = slider_top_y_r - (color_slider_h + SLIDER_VERTICAL_SPACING_GL);
                float slider_top_y_b = slider_top_y_g - (color_slider_h + SLIDER_VERTICAL_SPACING_GL);

                // Check Red Slider
                if (glX >= color_slider_x && glX <= color_slider_x + color_slider_w &&
                    glY >= slider_top_y_r && glY <= slider_top_y_r + color_slider_h) {
                    float clamped_glX = std::max(color_slider_x, std::min(color_slider_x + color_slider_w, glX));
                    customColor[0] = (clamped_glX - color_slider_x) / (color_slider_w - SLIDER_THUMB_WIDTH_GL);
                    currentColor[0] = customColor[0]; return;
                }
                // Check Green Slider
                if (glX >= color_slider_x && glX <= color_slider_x + color_slider_w &&
                    glY >= slider_top_y_g && glY <= slider_top_y_g + color_slider_h) {
                    float clamped_glX = std::max(color_slider_x, std::min(color_slider_x + color_slider_w, glX));
                    customColor[1] = (clamped_glX - color_slider_x) / (color_slider_w - SLIDER_THUMB_WIDTH_GL);
                    currentColor[1] = customColor[1]; return;
                }
                // Check Blue Slider
                if (glX >= color_slider_x && glX <= color_slider_x + color_slider_w &&
                    glY >= slider_top_y_b && glY <= slider_top_y_b + color_slider_h) {
                    float clamped_glX = std::max(color_slider_x, std::min(color_slider_x + color_slider_w, glX));
                    customColor[2] = (clamped_glX - color_slider_x) / (color_slider_w - SLIDER_THUMB_WIDTH_GL);
                    currentColor[2] = customColor[2]; return;
                }

                // Size sliders click detection (now in sidebar, stacked)
                float size_slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
                float size_slider_w = uiWidth - 2 * PADDING_X_GL;
                float size_slider_h = SLIDER_HEIGHT_GL;
                float label_text_height = 0.003f * 1.0f;

                float size_section_start_y = slider_top_y_b - color_slider_h - SECTION_PADDING_Y_GL; // This is the 'current_y_position' from render() after color sliders

                size_section_start_y -= (label_text_height + PADDING_Y_GL); // Adjust for brush label
                float size_top_y_brush = size_section_start_y - size_slider_h;
                size_section_start_y = size_top_y_brush - SLIDER_VERTICAL_SPACING_GL; // After brush slider
                float size_top_y_eraser = size_section_start_y - (label_text_height + PADDING_Y_GL) - size_slider_h;

                // Check Brush Size Slider
                if (glX >= size_slider_x && glX <= size_slider_x + size_slider_w &&
                    glY >= size_top_y_brush && glY <= size_top_y_brush + size_slider_h) {
                    float clamped_glX = std::max(size_slider_x, std::min(size_slider_x + size_slider_w, glX));
                    brushSize = ((clamped_glX - size_slider_x) / (size_slider_w - SLIDER_THUMB_WIDTH_GL)) * 19.0f + 1.0f;
                    brushSize = std::max(1.0f, std::min(20.0f, brushSize)); return;
                }
                // Check Eraser Size Slider
                if (glX >= size_slider_x && glX <= size_slider_x + size_slider_w &&
                    glY >= size_top_y_eraser && glY <= size_top_y_eraser + size_slider_h) {
                    float clamped_glX = std::max(size_slider_x, std::min(size_slider_x + size_slider_w, glX));
                    eraserSize = ((clamped_glX - size_slider_x) / (size_slider_w - SLIDER_THUMB_WIDTH_GL)) * 19.0f + 1.0f;
                    eraserSize = std::max(1.0f, std::min(20.0f, eraserSize)); return;
                }

            } else { // Click on Canvas (area below top bar, to the right of sidebar)
                // Start drawing
                isDrawing = true;
                if (currentTool < 2) { // Brush or Eraser
                    currentStroke.points.clear();
                    currentStroke.tool = currentTool;
                    currentStroke.size = (currentTool == 0) ? brushSize : eraserSize;
                    // Ensure the first point is within the canvas
                    float clampedGlX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, glX));
                    float clampedGlY = std::max(-1.0f, std::min(CANVAS_TOP_GL, glY));
                    currentStroke.points.push_back(Point(clampedGlX, clampedGlY, currentColor[0], currentColor[1], currentColor[2]));
                } else if (currentTool < 5) { // Shapes (Rectangle, Circle, Line)
                    // Ensure shapeStart is within canvas
                    float clampedGlX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, glX));
                    float clampedGlY = std::max(-1.0f, std::min(CANVAS_TOP_GL, glY));
                    shapeStart = Point(clampedGlX, clampedGlY, currentColor[0], currentColor[1], currentColor[2]);
                    shapeEnd = shapeStart; // Initialize end to start for preview
                }
            }
        } else if (action == GLFW_RELEASE) {
            if (isDrawing) {
                if (currentTool < 2) { // Brush or Eraser
                    if (!currentStroke.points.empty()) {
                        strokes.push_back(currentStroke);
                        currentStroke.points.clear();
                    }
                } else if (currentTool < 5) { // Shapes - create stroke from final shape
                    Stroke shapeStroke;
                    shapeStroke.tool = currentTool;
                    shapeStroke.size = brushSize; // Shapes use brush size for outline
                    
                    // Ensure shapeEnd is within canvas if drawing ended outside
                    float clampedGlX, clampedGlY;
                    screenToGL(xpos, ypos, clampedGlX, clampedGlY);
                    
                    // Clamp shapeEnd to canvas boundary if released outside
                    clampedGlX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, clampedGlX));
                    clampedGlY = std::max(-1.0f, std::min(CANVAS_TOP_GL, clampedGlY)); // Clamped by TOP_BAR
                    shapeEnd = Point(clampedGlX, clampedGlY, currentColor[0], currentColor[1], currentColor[2]);
                    

                    switch (currentTool) {
                        case 2: // Rectangle
                            shapeStroke.points.push_back(Point(shapeStart.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2]));
                            shapeStroke.points.push_back(Point(shapeEnd.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2]));
                            shapeStroke.points.push_back(Point(shapeEnd.x, shapeEnd.y, currentColor[0], currentColor[1], currentColor[2]));
                            shapeStroke.points.push_back(Point(shapeStart.x, shapeEnd.y, currentColor[0], currentColor[1], currentColor[2]));
                            shapeStroke.points.push_back(Point(shapeStart.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2])); // Close loop
                            break;
                        case 3: // Circle
                            {
                                float radius = std::sqrt(std::pow(shapeEnd.x - shapeStart.x, 2) + std::pow(shapeEnd.y - shapeStart.y, 2));
                                shapeStroke.circleCenter = shapeStart;
                                shapeStroke.circleRadius = radius;
                                // Generate points for circle outline
                                for (int i = 0; i <= 360; i += 5) { // More steps for smoother circle
                                    float angle = i * 3.1415926f / 180.0f;
                                    float x_pt = shapeStart.x + radius * std::cos(angle);
                                    float y_pt = shapeStart.y + radius * std::sin(angle);
                                    if (isInCanvas(x_pt, y_pt)) // Only add points within canvas
                                        shapeStroke.points.push_back(Point(x_pt, y_pt, currentColor[0], currentColor[1], currentColor[2]));
                                }
                            }
                            break;
                        case 4: // Line
                            shapeStroke.points.push_back(Point(shapeStart.x, shapeStart.y, currentColor[0], currentColor[1], currentColor[2]));
                            shapeStroke.points.push_back(Point(shapeEnd.x, shapeEnd.y, currentColor[0], currentColor[1], currentColor[2]));
                            break;
                    }
                    if (!shapeStroke.points.empty()) {
                        strokes.push_back(shapeStroke);
                    }
                }
                isDrawing = false;
            }
        }
    }
}

// Mouse movement callback
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (isDrawing && !isInSidebar(xpos, ypos) && !isInTopBar(xpos,ypos)) { // Ensure not in sidebar or top bar
        float glX, glY;
        screenToGL(xpos, ypos, glX, glY);
        
        // Clamp cursor position to canvas boundaries for drawing
        glX = std::max(SIDEBAR_RIGHT_GL, std::min(1.0f, glX));
        glY = std::max(-1.0f, std::min(CANVAS_TOP_GL, glY)); // Clamped by TOP_BAR

        if (currentTool < 2) { // Brush or Eraser
            currentStroke.points.push_back(Point(glX, glY, currentColor[0], currentColor[1], currentColor[2]));
        } else if (currentTool < 5) { // Shapes
            shapeEnd = Point(glX, glY, currentColor[0], currentColor[1], currentColor[2]);
        }
    }
}

// Scroll callback for brush size
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Check if mouse is over the size selectors area in the sidebar
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float glX, glY;
    screenToGL(xpos, ypos, glX, glY);

    float size_slider_x = SIDEBAR_LEFT_GL + PADDING_X_GL;
    float size_slider_w = uiWidth - 2 * PADDING_X_GL;
    float size_slider_h = SLIDER_HEIGHT_GL;
    float label_text_height = 0.003f * 1.0f;

    // Calculate y positions for size sliders (relative to top of sidebar)
    float current_y_for_scroll_tracker = CANVAS_TOP_GL - PADDING_Y_GL; // Start of sidebar elements

    // Skip Tool Buttons (6 buttons in a single column)
    float tool_btn_h = BUTTON_HEIGHT_GL;
    float tool_row_spacing = tool_btn_h + PADDING_Y_GL;
    current_y_for_scroll_tracker -= (tool_row_spacing * 6); // 6 rows of tools
    current_y_for_scroll_tracker -= SECTION_PADDING_Y_GL; // After tools section

    // Skip Color Sliders and their preview
    float preview_h = SLIDER_HEIGHT_GL * 1.5f;
    float color_slider_h = SLIDER_HEIGHT_GL;
    current_y_for_scroll_tracker -= (preview_h + PADDING_Y_GL); // preview swatch and its padding
    current_y_for_scroll_tracker -= (color_slider_h * 3 + SLIDER_VERTICAL_SPACING_GL * 2 + SECTION_PADDING_Y_GL); // 3 sliders + 2 paddings + section padding

    current_y_for_scroll_tracker -= (label_text_height + PADDING_Y_GL); // Adjust for brush label
    float size_top_y_brush = current_y_for_scroll_tracker - size_slider_h;
    current_y_for_scroll_tracker = size_top_y_brush - SLIDER_VERTICAL_SPACING_GL; // After brush slider
    float size_top_y_eraser = current_y_for_scroll_tracker - (label_text_height + PADDING_Y_GL) - size_slider_h;

    // Check Brush Size Slider
    if (glX >= size_slider_x && glX <= size_slider_x + size_slider_w &&
        glY >= size_top_y_brush && glY <= size_top_y_brush + size_slider_h) {
        brushSize += yoffset * 2.0f;
        brushSize = std::max(1.0f, std::min(20.0f, brushSize));
    }
    // Check Eraser Size Slider
    if (glX >= size_slider_x && glX <= size_slider_x + size_slider_w &&
        glY >= size_top_y_eraser && glY <= size_top_y_eraser + size_slider_h) {
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
    drawRect(SIDEBAR_RIGHT_GL, -1.0f, 2.0f - uiWidth, 1.0f - CANVAS_TOP_GL, 1.0f, 1.0f, 1.0f);
    drawRectOutline(SIDEBAR_RIGHT_GL, -1.0f, 2.0f - uiWidth, 1.0f - CANVAS_TOP_GL, 0.7f, 0.7f, 0.7f);
    
    // Draw Top Bar UI elements (Preset Colors, Clear Button)
    drawPresetColorPalette();
    drawClearButtonTopBar();

    // Draw Sidebar UI elements (Tools, Color Sliders, Size Sliders)
    current_y_position = CANVAS_TOP_GL - PADDING_Y_GL; // Starting Y for sidebar elements
    drawToolButtons(current_y_position);
    drawColorSlidersSidebar(current_y_position); // Now drawn in the sidebar
    drawSizeSelectorsSidebar(current_y_position); // Now drawn in the sidebar

    // Perform fill action if flag is set
    if (showFillDialog) {
        fillLastShapeWithColor();
        showFillDialog = false; // Reset flag
    }

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
