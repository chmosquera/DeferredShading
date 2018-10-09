/* Lab 6 base code - transforms using local matrix functions
	to be written by students -
	based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
	& Ian Dunn, Christian Eckhardt
*/
#include <iostream>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "camera.h"
// used for helper in perspective
#include "glm/glm.hpp"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;
const int DEFERRED = 0;





class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	int size = 5;

	// Our shader program
	std::shared_ptr<Program> prog,prog2, prog_nondeferred;

	// Shape to be used (from obj file)
	shared_ptr<Shape> shape;
	
	//camera
	camera mycam;

	//texture for sim
	GLuint TextureEarth;
	GLuint TextureMoon,depth_rb;
	GLuint gBuffer, gColor, gViewpos, gNormal;

	GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex;
	
	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our triangle to OpenGL
	GLuint VertexBufferID;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			mycam.w = 1;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			mycam.s = 1;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.a = 1;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{


		GLSL::checkVersion();

		
		// Set background color.
		glClearColor(0.12f, 0.34f, 0.56f, 1.0f);

		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		//culling:
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		//transparency
		glEnable(GL_BLEND);
		//next function defines how to mix the background color with the transparent pixel in the foreground. 
		//This is the standard:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		if (! prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("campos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");


		prog2 = make_shared<Program>();
		prog2->setVerbose(true);
		prog2->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/frag_nolight.glsl");
		if (!prog2->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog2->init();
		prog2->addUniform("P");
		prog2->addUniform("V");
		prog2->addUniform("M");
		prog2->addUniform("campos");
		prog2->addAttribute("vertPos");
		prog2->addAttribute("vertTex");
		prog2->addAttribute("vertNor");

		prog_nondeferred = make_shared<Program>();
		prog_nondeferred->setVerbose(true);
		prog_nondeferred->setShaderNames(resourceDirectory + "/nondeferred_vert.glsl", resourceDirectory + "/nondeferred_frag.glsl");
		if (!prog_nondeferred->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog_nondeferred->init();
		prog_nondeferred->addUniform("P");
		prog_nondeferred->addUniform("V");
		prog_nondeferred->addUniform("M");
		prog_nondeferred->addUniform("campos");
		prog_nondeferred->addAttribute("vertPos");
		prog_nondeferred->addAttribute("vertNor");
		prog_nondeferred->addAttribute("vertTex");

	}

	void initGeom(const std::string& resourceDirectory)
	{
		//init rectangle mesh (2 triangles) for the post processing
		glGenVertexArrays(1, &VertexArrayIDBox);
		glBindVertexArray(VertexArrayIDBox);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox);

		GLfloat *rectangle_vertices = new GLfloat[18];
		// front
		int verccount = 0;

		rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
		rectangle_vertices[verccount++] = 0.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;


		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), rectangle_vertices, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferTex);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex);

		float t = 1. / 100.;
		GLfloat *rectangle_texture_coords = new GLfloat[12];
		int texccount = 0;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
		rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 1;
		rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;

		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), rectangle_texture_coords, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(2);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		// Initialize mesh.
		shape = make_shared<Shape>();
		shape->loadMesh(resourceDirectory + "/sphere.obj");
		shape->resize(); 
		shape->init();
			
		
		int width, height, channels;
		char filepath[1000];

		//texture earth diffuse
		string str = resourceDirectory + "/earth.jpg";
		strcpy(filepath, str.c_str());		
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &TextureEarth);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureEarth);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		
		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex");//tex, tex2... sampler in the fragment shader
		// Then bind the uniform samplers to texture units:
		glUseProgram(prog->pid);
		glUniform1i(Tex1Location, 0);

		Tex1Location = glGetUniformLocation(prog_nondeferred->pid, "tex");//tex, tex2... sampler in the fragment shader
		glUseProgram(prog_nondeferred->pid);
		glUniform1i(Tex1Location, 0);


		glUseProgram(prog2->pid);
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glGenFramebuffers(1, &gBuffer);
		glActiveTexture(GL_TEXTURE0);
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		//RGBA8 2D texture, 24 bit depth texture, 256x256
		glGenTextures(1, &gColor);
		glBindTexture(GL_TEXTURE_2D, gColor);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);
			

		glGenTextures(1, &gViewpos);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gViewpos);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGRA, GL_FLOAT, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);
	
		glGenTextures(1, &gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGRA, GL_FLOAT, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);
		
		//Attach 2D texture to this FBO
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gColor, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gViewpos, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gNormal, 0);
		//-------------------------
		glGenRenderbuffers(1, &depth_rb);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		//-------------------------
		//Attach depth buffer to FBO
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
		//-------------------------
		//Does the GPU support current FBO configuration?

		
		// Send all 3 textures to the second program
		int Tex1Loc = glGetUniformLocation(prog2->pid, "gColor");//tex, tex2... sampler in the fragment shader
		int Tex2Loc = glGetUniformLocation(prog2->pid, "gViewPos");
		int Tex3Loc = glGetUniformLocation(prog2->pid, "gNormal");
		glUniform1i(Tex1Loc, 0);
		glUniform1i(Tex2Loc, 1);
		glUniform1i(Tex3Loc, 2);

		GLenum status;
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			cout << "status framebuffer: good";
			break;
		default:
			cout << "status framebuffer: bad";
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	//*************************************
	double get_last_elapsed_time()
		{
		static double lasttime = glfwGetTime();
		double actualtime = glfwGetTime();
		double difference = actualtime - lasttime;
		lasttime = actualtime;
		return difference;
		}
	//*************************************
	void render_to_screen()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, gColor);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, gViewpos);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glGenerateMipmap(GL_TEXTURE_2D);


		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		auto P = std::make_shared<MatrixStack>();
		P->pushMatrix();	
		P->perspective(70., width, height, 0.1, 100.0f);
		glm::mat4 M,V,S,T;		
	
		V = glm::mat4(1);
		
		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		

		prog2->bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gColor);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gViewpos);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		M = glm::scale(glm::mat4(1),glm::vec3(1.2,1,1)) * glm::translate(glm::mat4(1), glm::vec3(-0.5, -0.5, -1));
		glUniform3fv(prog2->getUniform("campos"), 1, &mycam.pos.x);
		glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog2->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog2->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glBindVertexArray(VertexArrayIDBox);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		prog2->unbind();
		
	}
	void render_to_texture() // aka render to framebuffer
	{
		// Frame Buffer
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, buffers);


		double frametime = get_last_elapsed_time();
		static double totalTime = 0;
		static int count = 0;
		if (count >= 100) {
			std::cout << "Deferred - time: " <<  totalTime/100.0 << endl;
			std::cout << "Deferred - time: " << totalTime << endl;
		}
		else {
			count++;
			totalTime += frametime;
			std::cout << totalTime << endl;
		}

		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		glm::mat4 M, V, S, T, P;
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones
		V = mycam.process();


		//bind shader and copy matrices
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos.x);

		static float angle = 0;
		angle += 0.02*frametime;
		M = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -5));
		glm::mat4 Ry = glm::rotate(glm::mat4(1.f), angle, glm::vec3(0, 1, 0));
		float pih = -3.1415926 / 2.0;
		glm::mat4 Rx = glm::rotate(glm::mat4(1.f), pih, glm::vec3(1, 0, 0));
		M = M * Ry * Rx;
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureEarth);


		//int size = 3;

		for (int i = -size; i <=size; i++)
		for (int j = -size; j <= size; j++)
		for (int k = -size; k <= size; k++) {

			int padding = 3.0f;

			M = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -5));
			T = translate(mat4(1.0f), vec3(padding * i, padding * j, padding * k));

			M = M * T* Ry * Rx;
			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			shape->draw(prog, true);
		}
	
		
		//done, unbind stuff
		prog->unbind();
	}

	void render_nondeferred() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		double frametime = get_last_elapsed_time();
		static double totalTime = 0;
		static int count = 0;

		if (count >= 100) {
			std::cout << "Nondeferred - time: " << totalTime / 100.00 << endl;
			std::cout << "Nondeferred - time: " << totalTime  << endl;
		}
		else {
			count++;
			totalTime += frametime;
			std::cout << totalTime << endl;
		}



		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		glm::mat4 M, V, S, T, P;
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones
		V = mycam.process();


		//bind shader and copy matrices
		prog_nondeferred->bind();
		glUniformMatrix4fv(prog_nondeferred->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog_nondeferred->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(prog_nondeferred->getUniform("campos"), 1, &mycam.pos.x);

		static float angle = 0;
		angle += 0.02*frametime;
		M = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -5));
		glm::mat4 Ry = glm::rotate(glm::mat4(1.f), angle, glm::vec3(0, 1, 0));
		float pih = -3.1415926 / 2.0;
		glm::mat4 Rx = glm::rotate(glm::mat4(1.f), pih, glm::vec3(1, 0, 0));
		M = M * Ry * Rx;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureEarth);


		//int size = 3;

		for (int i = -size; i <= size; i++)
			for (int j = -size; j <= size; j++)
				for (int k = -size; k <= size; k++) {

					int padding = 3.0f;

					M = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -5));
					T = translate(mat4(1.0f), vec3(padding * i, padding * j, padding * k));

					M = M * T* Ry * Rx;
					glUniformMatrix4fv(prog_nondeferred->getUniform("M"), 1, GL_FALSE, &M[0][0]);
					shape->draw(prog_nondeferred, true);
				}


		//done, unbind stuff
		prog_nondeferred->unbind();
	}
};
//*********************************************************************************************************
int main(int argc, char **argv)
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.

		if (DEFERRED == 1) {
			application->render_to_texture();
			application->render_to_screen();
		}
		else {
			application->render_nondeferred();
		}
		
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
