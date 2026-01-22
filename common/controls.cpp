// Include GLFW
#include <GLFW/glfw3.h>
extern GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "controls.hpp"

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix(){
	return ViewMatrix;
}
glm::mat4 getProjectionMatrix(){
	return ProjectionMatrix;
}


// Initial position : on +Z
glm::vec3 position = glm::vec3( -32, 30, -48 ); 
// Initial horizontal angle : toward -Z
float horizontalAngle = 0.59;
// Initial vertical angle : none
float verticalAngle = -0.48;
// Initial Field of View
float initialFoV = 45.0f;

float speed = 10.0f; 
float mouseSpeed = 0.005f;



// This is the callback function that GLFW will call when the mouse wheel is scrolled.
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	
	initialFoV -= (float)yoffset * 2.0f;

	// Clamp the Field of View (FoV) 
	if (initialFoV < 20.0f)
		initialFoV = 20.0f;
	if (initialFoV > 60.0f) // You can set this max higher
		initialFoV = 60.0f;
}



void computeMatricesFromInputs(){

	
	static double lastTime = glfwGetTime();

	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	// Get mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Reset mouse position for next frame
	glfwSetCursorPos(window, 1024/2, 768/2);

	// Compute new orientation
	horizontalAngle += mouseSpeed * float(1024/2 - xpos );
	verticalAngle   += mouseSpeed * float( 768/2 - ypos );

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 direction(
		cos(verticalAngle) * sin(horizontalAngle), 
		sin(verticalAngle),
		cos(verticalAngle) * cos(horizontalAngle)
	);
	
	// Right vector
	glm::vec3 right = glm::vec3(
		sin(horizontalAngle - 3.14f/2.0f), 
		0,
		cos(horizontalAngle - 3.14f/2.0f)
	);
	
	// Up vector
	glm::vec3 up = glm::cross( right, direction );

	
	float currentSpeed = speed; 
	if (glfwGetKey( window, GLFW_KEY_LEFT_SHIFT ) == GLFW_PRESS){
		currentSpeed = speed * 2.5f; // Sprint speed is 2.5x faster
	}


	// Move forward
	if (glfwGetKey( window, GLFW_KEY_UP ) == GLFW_PRESS || glfwGetKey( window, GLFW_KEY_W ) == GLFW_PRESS){
		position += direction * deltaTime * currentSpeed;
	}
	// Move backward
	if (glfwGetKey( window, GLFW_KEY_DOWN ) == GLFW_PRESS || glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS){
		position -= direction * deltaTime * currentSpeed;
	}
	// Strafe right

	if (glfwGetKey( window, GLFW_KEY_RIGHT ) == GLFW_PRESS || glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS){
		position += right * deltaTime * currentSpeed;
	}
	// Strafe left
	if (glfwGetKey( window, GLFW_KEY_LEFT ) == GLFW_PRESS || glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS){
		position -= right * deltaTime * currentSpeed;
	}
	// Move up
	if (glfwGetKey( window, GLFW_KEY_Q ) == GLFW_PRESS){
		position += up * deltaTime * currentSpeed;
	}
	// Move down
	if (glfwGetKey( window, GLFW_KEY_E ) == GLFW_PRESS){
		position -= up * deltaTime * currentSpeed;
	}


	
	
	float FoV = initialFoV; 

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	ProjectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 200.0f);
	// Camera matrix
	ViewMatrix       = glm::lookAt(
								position,           // Camera is here
								position+direction, // and looks here : at the same position, plus "direction"
								up                  // Head is up (set to 0,-1,0 to look upside-down)
						   );

	// For the next frame, the "last time" will be "now"
	lastTime = currentTime;
}
