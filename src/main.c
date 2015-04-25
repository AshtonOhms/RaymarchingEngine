#define GLFW_INCLUDE_GLCOREARB

#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <math.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768


char* loadFile(char* fileName) {
	char* fileContents;
	long inputFileSize;

	FILE* inputFile = fopen(fileName, "rb");
	fseek(inputFile, 0, SEEK_END);
	inputFileSize = ftell(inputFile);
	rewind(inputFile);

	fileContents = malloc(inputFileSize * sizeof(char));
	fread(fileContents, sizeof(char), inputFileSize, inputFile);
	fclose(inputFile);

	return fileContents;
}

int initOpenCL(cl_device_id* device_id, cl_context* context, cl_command_queue* commands) {
	int err;                            // error code returned from api calls

    // Get number of devices
    cl_uint num_devices;
    clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);

    // Load all of the devices in the list
    cl_device_id* devices = calloc(sizeof(cl_device_id), num_devices);
    err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, num_devices, devices, NULL);
    if (err != CL_SUCCESS) return err;

    // Specify for the second device (GeForce GT 750M on my machine)
    *device_id = devices[1];

    // Print out the device name
    char buf[128];
    clGetDeviceInfo(*device_id, CL_DEVICE_NAME, 128, buf, NULL);
    fprintf(stdout, "Device: %s \n", buf);

    // Create the context
    *context = clCreateContext(0, 1, device_id, NULL, NULL, &err);
    if (!*context) return err;

    // Create the command queue
    *commands = clCreateCommandQueue(*context, *device_id, 0, &err);
    if (!*commands) return err;

    return err;
}

int createAndBuildProgram(char* kernelSource, cl_device_id* device_id, cl_context* context, cl_program* program) {
	int err;

	// Create program from loaded source
    *program = clCreateProgramWithSource(*context, 1, (const char **) &kernelSource, NULL, &err);
    if ( !*program ) return err;

    // Build the program
    err = clBuildProgram(*program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
    	size_t len;
    	char buffer[2048];

    	clGetProgramBuildInfo(*program, *device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
    	printf("%s\n", buffer);

    	return err;
    }

	return err;
}

static void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argc, char* args[]) {
	//OpenCL types and initialization
	/*int err;
	size_t global[2];                      // global domain size for our calculation
    size_t local;                       // local domain size for our calculation

	cl_device_id device_id;             // compute device id 
    cl_context context;                 // compute context
    cl_command_queue commands;          // compute command queue
    cl_program program;                 // compute program
    cl_kernel kernel;                   // compute kernel

    // Initialize OpenCL
    err = initOpenCL(&device_id, &context, &commands);
    if ( err != CL_SUCCESS ) {
    	printf("Failed to initialize OpenCL!\n");
    	return EXIT_FAILURE;
    }

    // Load the source code for the render kernel
    char* kernelSource = loadFile("OpenCL/render.cl");
    err = createAndBuildProgram(kernelSource, &device_id, &context, &program);
    if (err != CL_SUCCESS) {
    	printf("Failed to create/build program!\n");
    	return EXIT_FAILURE;
    }

    kernel = clCreateKernel(program, "render", &err);
    if(!kernel || err != CL_SUCCESS) {
    	printf("Failed to create compute kernel!\n");
    	return EXIT_FAILURE;
    }*/

    // GLFW Initialization
    if(!glfwInit()) {
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed

    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);
    
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Raytrace Demo", NULL, NULL);

    if(!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);


    // Set the arguments to the compute kernel
    int time = 0;
    int width = WINDOW_WIDTH;
    int height = WINDOW_HEIGHT;

    //err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &screenMem);
    //err |= clSetKernelArg(kernel, 1, sizeof(Uint32), &width);
    //err |= clSetKernelArg(kernel, 2, sizeof(Uint32), &height);
    //err |= clSetKernelArg(kernel, 3, sizeof(Uint32), &time);

    /*if (err != CL_SUCCESS) {
    	printf("Error: Failed to set kernel arguments! %d\n", err);
    	return EXIT_FAILURE;
    }*/

    // Get the maximum work group size for executing the kernel on the device
    /*err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
    if(err != CL_SUCCESS) {
    	printf("Failed to retrieve kernel work group info! %d\n", err);
    	return EXIT_FAILURE;
    }*/

    /*GLuint vertexArrayId;
    glGenVertexArrays(1, &vertexArrayId);
    glBindVertexArray(vertexArrayId);*/

    printf("%s\n", glGetString(GL_VERSION));

    // Shader declarations
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint programId = glCreateProgram();
    GLint result = GL_FALSE;
    int infoLogLength;

    // Create & compile vertex shader
    const char* vertexShaderSource = loadFile("shaders/vertex.glsl");
    glShaderSource(vertexShaderId, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShaderId);

    // Fetch vertex shader errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertexShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
    char* vertexShaderErrorMessage = malloc(infoLogLength * sizeof(char));
    glGetShaderInfoLog(vertexShaderId, infoLogLength, NULL, vertexShaderErrorMessage);
    printf("Vertex Errors: \n%s\n", vertexShaderErrorMessage);

    // Create and compile fragment shader
    const char* fragmentShaderSource = loadFile("shaders/fragment.glsl");
    glShaderSource(fragmentShaderId, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShaderId);

    // Fetch fragment shader error
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragmentShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
    char* fragmentShaderErrorMessage = malloc(infoLogLength * sizeof(char));
    glGetShaderInfoLog(fragmentShaderId, infoLogLength, NULL, fragmentShaderErrorMessage);
    printf("Fragment Errors:\n%s\n", fragmentShaderErrorMessage);

    // Create shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    glLinkProgram(programId);

    float ratio = width / (float) height;

    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    int iwidth = 512;
    int iheight = 512;

    char* data = malloc(iwidth * iheight * 3 * sizeof(char));

    for(int x = 0; x < iwidth; x++) {
        for(int y = 0; y < iheight; y++) {
            int index = y * iwidth + x;

            data[index * 3] = 0;
            data[index * 3 + 1] = 128 * (y%2);
            data[index * 3 + 2] = 128 * (x%2);
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, iwidth, iheight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    GLfloat vertexBufferData[] = {
        1.0f, -1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,  
    };

    GLfloat uvBufferData[] = {
        1.0f, 0.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
    };

    GLuint vertexArrayId;
    glGenVertexArrays(1, &vertexArrayId);
    glBindVertexArray(vertexArrayId);

    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * 4, vertexBufferData, GL_STATIC_DRAW);

    GLuint uvBuffer;
    glGenBuffers(1, &uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * 4, uvBufferData, GL_STATIC_DRAW);

    while(!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(programId);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(
           0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
           3,                  // size
           GL_FLOAT,           // type
           GL_FALSE,           // normalized?
           0,                  // stride
           0            // array buffer offset
        );

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glVertexAttribPointer(
           1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
           2,                  // size
           GL_FLOAT,           // type
           GL_FALSE,           // normalized?
           0,                  // stride
           0            // array buffer offset
        );
         
        // Draw the triangle !
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); 
         
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        
        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}