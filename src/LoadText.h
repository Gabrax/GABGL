#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLT_IMPLEMENTATION
#include "gltext.h"
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>


struct TextRenderer {

    std::vector<GLTtext*> texts;

    // Constructor to initialize a specified number of GLTtext objects
    TextRenderer(size_t count = 1) {
        for (size_t i = 0; i < count; ++i) {
            GLTtext* text = gltCreateText();
            gltSetText(text, "");
            texts.push_back(text);
        }
    }

    // Destructor to free allocated GLTtext objects
    ~TextRenderer() {
        for (auto text : texts) {
            gltDeleteText(text);
        }
    }

    // Template function to set and render text with optional arguments
    template<typename... Args>
    void renderText(size_t index, const char* prefix, Args... args) {
        if (index >= texts.size()) return;  // Ensure index is in range

        std::ostringstream oss;
        oss << prefix;
        
        // Using fold expression to insert each argument into the stream
        ((oss << std::fixed << std::setprecision(2) << args << " "), ...);

        gltSetText(texts[index], oss.str().c_str());
    }

    // Function to render all stored GLTtext objects on screen
    void drawTexts() {
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        gltBeginDraw();
        float yOffset = 0.0f;
        for (GLTtext* text : texts) {
            gltDrawText2D(text, 0.0f, yOffset, 2.0f);
            yOffset += 40.0f;  // Adjust vertical spacing as needed
        }
        gltEndDraw();

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
};
