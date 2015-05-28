#define GLFW_INCLUDE_GLCOREARB

#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#include <GLFW/glfw3.h>
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <math.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable

char* loadFile(char* fileName) {
  char* fileContents;
  long inputFileSize;

  FILE* inputFile = fopen(fileName, "rb");
  fseek(inputFile, 0, SEEK_END);
  inputFileSize = ftell(inputFile);
  rewind(inputFile);

  fileContents = malloc(inputFileSize + 1);
  fread(fileContents, 1, inputFileSize, inputFile);
  fclose(inputFile);

  fileContents[inputFileSize] = 0;

  return fileContents;
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
    int err;
    size_t global[2];                      // global domain size for our calculation
    size_t local;                           // local domain size for our calculation

    cl_platform_id platform_id;
    cl_device_id devices[10];
    cl_device_id device_id;             // compute device id
    cl_context context;                 // compute context
    cl_command_queue commands;          // compute command queue
    cl_program program;                 // compute program
    cl_kernel kernel;                   // compute kernel

    // GLFW Initialization
    if(!glfwInit()) {
        return EXIT_FAILURE;
    }

    printf("GLFW Initialized\n");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make OS X happy; should not be needed

    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Raytrace Demo", NULL, NULL);

    if(!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    printf("Window created\n");

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // Initialize OpenCL
    CGLContextObj kCGLContext = CGLGetCurrentContext();
    CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);

    printf("OpenGL contexts retrieved\n");

    const GLubyte* openglVendor = glGetString(GL_VENDOR);
    printf("OpenGL vendor: %s\n", openglVendor);

    cl_context_properties properties[] = {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
        (cl_context_properties)kCGLShareGroup, 0
    };

    clGetPlatformIDs(1, &platform_id, NULL);
    clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 10, devices, NULL);
    device_id = devices[1];

    char device_name[256];
    clGetDeviceInfo(device_id, CL_DEVICE_NAME, 256, device_name, NULL);
    printf("Device: %s\n", device_name);

    context = clCreateContext(properties, 0, 0, NULL, 0, 0);
    commands = clCreateCommandQueue(context, device_id, 0, &err);

    printf("OpenCL context created\n");

    // Load the source code for the render kernel
    char* kernelSource = loadFile("OpenCL/render.cl");

    //printf("%s\n", kernelSource);

    program = clCreateProgramWithSource(context, 1, (const char **) &kernelSource, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Failed to create program\n");
        return EXIT_FAILURE;
    }
    printf("Program created\n");

    err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
      size_t len;
      clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);

      char* buffer = malloc(len + 1);
      buffer[len] = 0;

      clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
      printf("Failed to build program %d:\n%s\n", err, buffer);

      return EXIT_FAILURE;
    }
    printf("Program built\n");

    kernel = clCreateKernel(program, "render", &err);
    if(!kernel || err != CL_SUCCESS) {
        printf("Failed to create compute kernel %d\n", err);
        return EXIT_FAILURE;
    }

    printf("Kernel created\n");

    // Set the arguments to the compute kernel
    double time = 0.0;
    int width = WINDOW_WIDTH;
    int height = WINDOW_HEIGHT;

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

    int iwidth = 1024;
    int iheight = 768;

    char* data = calloc(iwidth * iheight * 4, sizeof(char));

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iwidth, iheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    cl_mem mTexture = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY,
                                            GL_TEXTURE_2D, 0, textureId, &err);

    if (err != CL_SUCCESS) {
        printf("Failed to bind texture! %d\n", err);
        return EXIT_FAILURE;
    }

    printf("Texture bound.\n");

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &mTexture);
    err |= clSetKernelArg(kernel, 1, sizeof(int), &iwidth);
    err |= clSetKernelArg(kernel, 2, sizeof(int), &iheight);
    err |= clSetKernelArg(kernel, 3, sizeof(double), &time);

    if (err != CL_SUCCESS) {
        printf("Error: Failed to set kernel arguments! %d\n", err);
        return EXIT_FAILURE;
    }

    printf("Kernel arguments initialized\n");

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

    global[0] = iwidth;
    global[1] = iheight;

    // Get the maximum work group size for executing the kernel on the device
    err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
    if(err != CL_SUCCESS) {
      printf("Failed to retrieve kernel work group info! %d\n", err);
      return EXIT_FAILURE;
    }

    glfwSetTime(0.0f);
    int frameCount = 0;

    while(!glfwWindowShouldClose(window)) {
        time = glfwGetTime();
        err = clSetKernelArg(kernel, 3, sizeof(double), &time);

        err = clEnqueueAcquireGLObjects(commands, 1, &mTexture, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            printf("Failed to acquire screen texture %d", err);
        }

        err = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, global, NULL, 0, NULL, NULL);
        if (err != CL_SUCCESS){
            printf("Failed to execute kernel %d\n", err);
        }

        err = clEnqueueReleaseGLObjects(commands, 1, &mTexture, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            printf("Failed to release screen texture %d", err);
        }

        clFinish(commands);


        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(programId);

        // Vertex attribute buffer
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // UV attribute buffer
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // Draw the screen
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glfwSwapBuffers(window);
        glfwPollEvents();

        frameCount += 1;
    }

    double fps = frameCount / glfwGetTime();
    printf("FPS: %f\n", fps);

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
