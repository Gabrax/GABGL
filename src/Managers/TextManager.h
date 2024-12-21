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


struct TextManager {

    std::vector<GLTtext*> texts; 
    size_t nextIndex = 0;        

    TextManager() {
        if (!gltInit()) {
            throw std::runtime_error("Failed to initialize glText!");
        }
    }

    ~TextManager() {
        for (auto text : texts) {
            if (text) {
                gltDeleteText(text);
            }
        }
        gltTerminate();
    }

    template<typename... Args>
    void AddText(const char* prefix, Args... args) {
        
        if (nextIndex >= texts.size()) {
            texts.emplace_back(gltCreateText()); 
        }

        std::ostringstream oss;
        oss << prefix;
        ((oss << std::fixed << std::setprecision(2) << args << " "), ...);

        gltSetText(texts[nextIndex], oss.str().c_str());

        nextIndex++;
    }

    void DrawTexts() {

        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        gltBeginDraw();
        gltColor(1.0f, 1.0f, 1.0f, 1.0f);  

        float yOffset = 0.0f;
        for (size_t i = 0; i < nextIndex; ++i) { 
            if (texts[i]) {
                gltDrawText2D(texts[i], 0.0f, yOffset, 2.0f);
                yOffset += 40.0f;  
            }
        }
        gltEndDraw();

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        nextIndex = 0;
    }
};
