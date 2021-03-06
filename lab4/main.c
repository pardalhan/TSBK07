// Lab 4, terrain generation

#ifdef __APPLE__
	#include <OpenGL/gl3.h>
	#include "MicroGlut.h"
	// Linking hint for Lightweight IDE
	// uses framework Cocoa
#endif
#include "GL_utilities.h"
#include "VectorUtils3.h"
#include "loadobj.h"
#include "LoadTGA.h"
#include "main.h"
#include "math.h"

mat4 projectionMatrix;

// Octagon
Model *octagon;
mat4 transOctagon;
GLuint concrete;
GLuint dirt;
GLuint maskrosTexture;

Model* GenerateTerrain(TextureData *tex)
{
	int vertexCount = tex->width * tex->height;
	int triangleCount = (tex->width-1) * (tex->height-1) * 2;
	int x, z;

	GLfloat *vertexArray = malloc(sizeof(GLfloat) * 3 * vertexCount);
	GLfloat *normalArray = malloc(sizeof(GLfloat) * 3 * vertexCount);
	GLfloat *texCoordArray = malloc(sizeof(GLfloat) * 2 * vertexCount);
	GLuint *indexArray = malloc(sizeof(GLuint) * triangleCount*3);

	printf("bpp: %d\n", tex->bpp);
	printf("width: %d\n", tex->width);
	for (x = 0; x < tex->width; x++)
		for (z = 0; z < tex->height; z++)
		{
// Vertex array. You need to scale this properly
			vertexArray[(x + z * tex->width)*3 + 0] = x / 1.0;
			vertexArray[(x + z * tex->width)*3 + 1] = tex->imageData[(x + z * tex->width) * (tex->bpp/8)] / 10.0;
			vertexArray[(x + z * tex->width)*3 + 2] = z / 1.0;
// Normal vectors. You need to calculate these.
			vec3 left;
			vec3 us;
			vec3 top;
			us =(vec3) {vertexArray[(x + z * tex->width)*3 + 0],
					vertexArray[(x + z * tex->width)*3 + 1],
					vertexArray[(x + z * tex->width)*3 + 2]};
			if(x -1 < 0){
			left =(vec3){0, 1, 0};
			}
			else{
			left =(vec3) {vertexArray[(x-1 + z * tex->width)*3 + 0],
					vertexArray[(x-1 + z * tex->width)*3 + 1],
					vertexArray[(x-1 + z * tex->width)*3 + 2]};
			}
			if(z-1 < 0){
			top=(vec3){0, 1, 0};
			}
			else {
			top = (vec3){vertexArray[(x + (z-1) * tex->width)*3 + 0],
					 vertexArray[(x + (z-1) * tex->width)*3 + 1],
					 vertexArray[(x + (z-1) * tex->width)*3 + 2]};
			}
			vec3 leftV = VectorSub(left, us);
			vec3 topV = VectorSub(top, us);
			vec3 normal = Normalize(CrossProduct(leftV, topV));

			normalArray[(x + z * tex->width)*3 + 0] = normal.x;
			normalArray[(x + z * tex->width)*3 + 1] = normal.y;
			normalArray[(x + z * tex->width)*3 + 2] = normal.z;
// Texture coordinates. You may want to scale them.
			texCoordArray[(x + z * tex->width)*2 + 0] = (float)x / tex->width;
			texCoordArray[(x + z * tex->width)*2 + 1] = (float)z / tex->height;
		}
	for (x = 0; x < tex->width-1; x++)
		for (z = 0; z < tex->height-1; z++)
		{
		// Triangle 1
			indexArray[(x + z * (tex->width-1))*6 + 0] = x + z * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 1] = x + (z+1) * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 2] = x+1 + z * tex->width;
		// Triangle 2
			indexArray[(x + z * (tex->width-1))*6 + 3] = x+1 + z * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 4] = x + (z+1) * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 5] = x+1 + (z+1) * tex->width;
		}

	// End of terrain generation

	// Create Model and upload to GPU:

	Model* model = LoadDataToModel(
			vertexArray,
			normalArray,
			texCoordArray,
			NULL,
			indexArray,
			vertexCount,
			triangleCount*3);

	return model;
}


// vertex array object
Model *m, *m2, *tm;
// Reference to shader program
GLuint program;
TextureData ttex; // terrain

void init(void)
{
	// GL inits
	glClearColor(0.2,0.2,0.5,0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	printError("GL inits");

	projectionMatrix = frustum(-0.1, 0.1, -0.1, 0.1, 0.2, 200.0);

	// Load and compile shader
	program = loadShaders("terrain.vert", "terrain.frag");
	glUseProgram(program);
	printError("init shader");

	glUniformMatrix4fv(glGetUniformLocation(program, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);

	// Load textures
	LoadTGATextureSimple("maskros512.tga", &maskrosTexture);
	LoadTGATextureSimple("dirt.tga", &dirt);

	glUniform1i(glGetUniformLocation(program, "maskrosen"), 0); 
	glUniform1i(glGetUniformLocation(program, "dirt"), 1);

	// Load object

	octagon = LoadModelPlus("octagon.obj");
	transOctagon = T(84, 1, 90);
	LoadTGATextureSimple("conc.tga", &concrete); 

// Load terrain data

	LoadTGATextureData("fft-terrain.tga", &ttex);
	tm = GenerateTerrain(&ttex);
	printError("init terrain");
}
vec3 cameraPos = {70.0, 1.0, 74.0};
vec3 cameraTarget = {0, 5, 8};
vec3 cameraNormal = {0, 1, 0};
mat4 camMatrix;

void drawObject(mat4 transform, Model* model, GLuint p)
{
	glUniformMatrix4fv(glGetUniformLocation(p, "mdlMatrix"), 1, GL_TRUE, transform.m);
	DrawModel(model, p, "inPosition", "inNormal", "inTexCoord");
	printError("drawObject()");
}


float findHeight(float xpos, float zpos)
{
	GLint cornerX = floor(xpos);
	GLint cornerZ = floor(zpos);

	if ((xpos - cornerX + zpos - cornerZ) > 1) {
		cornerX += 1;
		cornerZ += 1;
	}

	vec3 normal = (vec3)
		{tm->normalArray[(cornerX + cornerZ * ttex.width)*3 + 0],
		 tm->normalArray[(cornerX + cornerZ * ttex.width)*3 + 1],
		 tm->normalArray[(cornerX + cornerZ * ttex.width)*3 + 2]};

	vec3 position = (vec3)
		{tm->vertexArray[(cornerX + cornerZ * ttex.width)*3 + 0],
		 tm->vertexArray[(cornerX + cornerZ * ttex.width)*3 + 1],
		 tm->vertexArray[(cornerX + cornerZ * ttex.width)*3 + 2]};

	GLfloat D = normal.x * position.x + normal.y * position.y +
		normal.z * position.z;

	return -(D - normal.x * xpos - normal.z * zpos);
}


void display(void)
{
	printError("pre display");
	cameraPos = moveOnKeyInputRelativeCamera(cameraPos);
	cameraTarget = moveOnKeyInputRelativeCamera(cameraTarget);
	//cameraPos.y = findHeight(cameraPos.x, cameraPos.z) +2;
	//cameraTarget.y = findHeight(cameraTarget.x, cameraTarget.z) +2;
	camMatrix = lookAtv(cameraPos, cameraTarget, cameraNormal);

	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 total, modelView;
	glUseProgram(program);


	// Draw terrain
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, maskrosTexture);
	modelView = IdentityMatrix();
	total = Mult(camMatrix, modelView);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, total.m);	
	drawObject(total, tm, program);

	// Draw octagon
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, concrete);
	transOctagon = T(cameraPos.x - 5, findHeight(cameraPos.x - 5, cameraPos.z), cameraPos.z);
	total = Mult(camMatrix, transOctagon);
	drawObject(total, octagon, program);
	
	glActiveTexture(GL_TEXTURE0);

	printError("display 2");

	glutSwapBuffers();
}

void timer(int i)
{
	glutTimerFunc(20, &timer, i);
	glutPostRedisplay();
}

vec3 cameraDirection;

void mouse(int x, int y)
{
  cameraTarget = (vec3)
		{cos((float)x / 400 * M_PI * 2) * sin((float)y / 300 * M_PI),
		 -(float)y / 300 + 0.5,
		 sin((float)x / 400 * M_PI * 2) * sin((float)y / 300 * M_PI)};
	cameraTarget = VectorAdd(cameraTarget, cameraPos);

	camMatrix = lookAtv(cameraPos, cameraTarget, cameraNormal);
	cameraDirection = Normalize(VectorSub(cameraTarget, cameraPos));
	printError("handleMouse()");
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
	glutInitContextVersion(3, 2);
	glutInitWindowSize (400, 300);
	glutCreateWindow ("TSBK07 Lab 4");
	glutDisplayFunc(display);
	init ();
	initKeymapManager();
	glutTimerFunc(20, &timer, 0);

	glutPassiveMotionFunc(mouse);

	glutMainLoop();
	exit(0);
}

vec3 moveOnKeyInputRelativeCamera(vec3 in)
{
	vec3 forward;
	vec3 leftV;
	forward = ScalarMult(cameraDirection, 0.1f);
	leftV = ScalarMult(CrossProduct(cameraDirection, cameraNormal), 0.1f);

  if(keyIsDown('w')) {
    in.x += forward.x;
    in.y += forward.y;
    in.z += forward.z;
	}
  else if (keyIsDown('s')) {
    in.x -= forward.x;
    in.y -= forward.y;
    in.z -= forward.z;
	}

  if(keyIsDown('a')){
		in.x -= leftV.x;
    in.y -= leftV.y;
    in.z -= leftV.z;
	}
  else if(keyIsDown('d')){
		in.x += leftV.x;
    in.y += leftV.y;
    in.z += leftV.z;
	}

  if(keyIsDown('p')){
    printf("(e%f, ", cameraPos.x);
    printf("%f, ", cameraPos.y);
    printf("%f)\n", cameraPos.z);
	}
  if(keyIsDown('q'))
    in.z += 0.1;
  else if(keyIsDown('e'))
    in.z -= 0.1;

	printError("moveonkeyinputrelativecamera()");

  return in;
}
