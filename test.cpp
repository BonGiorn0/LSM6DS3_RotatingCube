#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <chrono>
#include <thread>


#include "shader.hpp"
#include "com_port.h"

using namespace glm;

int main()
{
    glewExperimental = true; // Needed for core profile
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4);               // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

    // Open a window and create its OpenGL context
    GLFWwindow *window; // (In the accompanying source code, this variable is global for simplicity)
    window = glfwCreateWindow(1024, 768, "Tutorial 01", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // Initialize GLEW
    glewExperimental = true;        // Needed in core profile
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.3f, 0.0f);

    // Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it is closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "TransformVertexShader.vert", "ColorFragmentShader.frag" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	// Camera matrix
	glm::mat4 View       = glm::lookAt(
								glm::vec3(0,0,-5), // Camera is at (4,3,-3), in World Space
								glm::vec3(0,0,0), // and looks at the origin
								glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
						   );
	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model      = glm::mat4(1.0f);
    
    static const GLfloat g_vertex_buffer_data[] = { 
		-1.5f,+0.15f,+1.0f,
		-1.5f,+0.15f,-1.0f,
		-1.5f,-0.15f,+1.0f,

		-1.5f,+0.15f,-1.0f,
		-1.5f,-0.15f,+1.0f,
		-1.5f,-0.15f,-1.0f,

		+1.5f,+0.15f,+1.0f,
		+1.5f,+0.15f,-1.0f,
		+1.5f,-0.15f,+1.0f,

		+1.5f,+0.15f,-1.0f,
		+1.5f,-0.15f,+1.0f,
		+1.5f,-0.15f,-1.0f, //RED

		-1.5f,+0.15f,-1.0f,
		+1.5f,+0.15f,-1.0f,
		-1.5f,-0.15f,-1.0f,

		+1.5f,+0.15f,-1.0f,
		-1.5f,-0.15f,-1.0f,
		+1.5f,-0.15f,-1.0f,

		-1.5f,+0.15f,+1.0f,
		+1.5f,+0.15f,+1.0f,
		-1.5f,-0.15f,+1.0f,

		+1.5f,+0.15f,+1.0f,
		-1.5f,-0.15f,+1.0f,
		+1.5f,-0.15f,+1.0f, //GREEN

		-1.5f,+0.15f,-1.0f,
		-1.5f,+0.15f,+1.0f,
		+1.5f,+0.15f,-1.0f,

		-1.5f,+0.15f,+1.0f,
		+1.5f,+0.15f,-1.0f,
		+1.5f,+0.15f,+1.0f,

		-1.5f,-0.15f,-1.0f,
		-1.5f,-0.15f,+1.0f,
		+1.5f,-0.15f,-1.0f,

		-1.5f,-0.15f,+1.0f,
		+1.5f,-0.15f,-1.0f,
		+1.5f,-0.15f,+1.0f,
	};

	// One color for each vertex. They were generated randomly.
	static GLfloat g_color_buffer_data[12*3*3];
	for (int v = 0; v < 12 ; v++){
		g_color_buffer_data[3*v+0] = 1;
		g_color_buffer_data[3*v+1] = 0;
		g_color_buffer_data[3*v+2] = 0;
	}
	for (int v = 12; v < 24 ; v++){
		g_color_buffer_data[3*v+0] = 0;
		g_color_buffer_data[3*v+1] = 1;
		g_color_buffer_data[3*v+2] = 0;
	}
	for (int v = 24; v < 36 ; v++){
		g_color_buffer_data[3*v+0] = 0.5;
		g_color_buffer_data[3*v+1] = 0;
		g_color_buffer_data[3*v+2] = 1.f;
	}

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint colorbuffer;
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

	int fd = open (PORTNAME, O_RDONLY | O_NOCTTY | O_SYNC | O_NONBLOCK);
		if (fd < 0)
		{
				//error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
				//return;
		}
	set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
	set_blocking (fd, 0);                // set no blocking
	double read_delay = 1;
    do
    {
        // Clear the screen. It's not mentioned before Tutorial 02, but it can cause flickering, so it's there nonetheless.
		double start = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		int16_t buf[3];
		
		
		int n = read (fd, buf, 3 * 2);
		if(n != 0){
			printf("X: %d\nY: %d\nZ: %d\n", buf[0], buf[1], buf[2]);
			read_delay = glfwGetTime();
			Model =  glm::mat4(1);
			float x = (float)buf[0] / INT16_MAX * 2;
			float y = (float)buf[1] / INT16_MAX * 2;
			float z = (float)buf[2] / INT16_MAX * 2;
			float pitch = -atan(x / sqrt(y * y + z * z));
			float roll = -atan(y / sqrt(x * x + z * z));
			float yaw = atan(sqrt(x * x + y * y) / z);
			Model = glm::rotate(Model, pitch, glm::vec3(0,0,1));
			Model = glm::rotate(Model, roll, glm::vec3(1,0,0));
			//Model = glm::rotate(Model, yaw, glm::vec3(0,1,0));
		}
		
		

		// Use our shader
		glUseProgram(programID);

		//Model = glm::rotate(Model, (float)-M_PI / 180 , glm::vec3(0,0,1));
		//Model = glm::rotate(Model, (float)-M_PI / 360 , glm::vec3(0,1,0));
		// Our ModelViewProjection : multiplication of our 3 matrices
		 // Remember, matrix multiplication is the other way around
		glm::mat4 MVP        = Projection * View * Model;


		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : colors
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 12*3); // 12*3 indices starting at 0 -> 12 triangles

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
		double end = glfwGetTime();
		double tmp = 1.0 / 60 - (end - start);
		if(tmp > 0){
			std::this_thread::sleep_for(std::chrono::duration<double>(tmp));;
		}
			
    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);
	close(fd);
}