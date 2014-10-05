//
//  Arcball.h
//  Arcball
//
//  Created by Saburo Okita on 12/03/14.
//  Copyright (c) 2014 Saburo Okita. All rights reserved.
//

#ifndef __Arcball__Arcball__
#define __Arcball__Arcball__

namespace citygen
{
  class Arcball {
  private:
    int windowWidth;
    int windowHeight;
    int mouseEvent;
    GLfloat rollSpeed;
    GLfloat angle ;
    glm::vec3 prevPos;
    glm::vec3 currPos;
    glm::vec3 camAxis;

    bool xAxis;
    bool yAxis;

  public:
    Arcball( int window_width, int window_height, GLfloat roll_speed = 1.0f, bool x_axis = true, bool y_axis = true );
    glm::vec3 toScreenCoord( float x, float y );

    void mouseButtonCallback( GLFWwindow * window, int button, int action, int mods );
    void cursorCallback( GLFWwindow *window, float x, float y );

    glm::mat4 createViewRotationMatrix();
    glm::mat4 createModelRotationMatrix( glm::mat4& view_matrix );

  };
}

#endif /* defined(__Arcball__Arcball__) */