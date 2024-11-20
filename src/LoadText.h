#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stdexcept>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLT_IMPLEMENTATION
#include "gltext.h"
#include <sstream>
#include <iomanip>
#include <vector>


struct TextRenderer {
    std::vector<GLTtext*> texts; // Store all text objects
    size_t nextIndex = 0;        // Tracks the next available index

    // Constructor: Initialize GLT
    TextRenderer() {
        if (!gltInit()) {
            throw std::runtime_error("Failed to initialize glText!");
        }
    }

    // Destructor: Free allocated GLTtext objects
    ~TextRenderer() {
        for (auto text : texts) {
            if (text) {
                gltDeleteText(text);
            }
        }
        gltTerminate();
    }

    // Template function to add or update text dynamically
    template<typename... Args>
    void renderText(const char* prefix, Args... args) {
        // Ensure the vector has enough room for the current index
        if (nextIndex >= texts.size()) {
            texts.push_back(gltCreateText()); // Create new text object if needed
        }

        // Construct the text
        std::ostringstream oss;
        oss << prefix;
        ((oss << std::fixed << std::setprecision(2) << args << " "), ...);

        // Update the text at the current index
        gltSetText(texts[nextIndex], oss.str().c_str());

        // Increment the index for the next text
        nextIndex++;
    }

    // Function to render all stored GLTtext objects on screen
    void drawTexts() {
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        gltBeginDraw();
        gltColor(1.0f, 1.0f, 1.0f, 1.0f);  // Set text color to white

        float yOffset = 0.0f;
        for (size_t i = 0; i < nextIndex; ++i) { // Only render up to `nextIndex`
            if (texts[i]) {
                gltDrawText2D(texts[i], 0.0f, yOffset, 2.0f);
                yOffset += 40.0f;  // Adjust vertical spacing as needed
            }
        }
        gltEndDraw();

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        // Reset index for the next frame
        nextIndex = 0;
    }
};
