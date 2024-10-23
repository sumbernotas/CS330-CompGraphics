///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/cork.jpg",
		"bottle-cork");

	bReturn = CreateGLTexture(
		"textures/draught-living-death.jpg",
		"draught-potion");

	bReturn = CreateGLTexture(
		"textures/twine-black.png",
		"black-twine");

	bReturn = CreateGLTexture(
		"textures/wood-seamless.jpg",
		"table");

	bReturn = CreateGLTexture(
		"textures/twine-brown.png",
		"brown-twine");

	bReturn = CreateGLTexture(
		"textures/wall.jpg",
		"background");

	bReturn = CreateGLTexture(
		"textures/amortentia.jpg",
		"love-potion");

	bReturn = CreateGLTexture(
		"textures/felix.jpg",
		"lucky-potion");

	bReturn = CreateGLTexture(
		"textures/thunderbrew.jpg",
		"stun-potion");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

void SceneManager::DefineObjectMaterials()
{

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	woodMaterial.shininess = 0.1;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.478f, 0.478f, 0.478f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 98.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL wallMaterial;
	wallMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.9f);
	wallMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	wallMaterial.shininess = 2.0;
	wallMaterial.tag = "wall";

	m_objectMaterials.push_back(wallMaterial);

	OBJECT_MATERIAL twineMaterial;
	twineMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	twineMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	twineMaterial.shininess = 0.2;
	twineMaterial.tag = "twine";

	m_objectMaterials.push_back(twineMaterial);

	OBJECT_MATERIAL liquidMaterial;
	liquidMaterial.diffuseColor = glm::vec3(0.329f, 0.212f, 0.4f);
	liquidMaterial.specularColor = glm::vec3(0.1f, 0.05f, 0.1f);
	liquidMaterial.shininess = 0.50;
	liquidMaterial.tag = "liquid";

	m_objectMaterials.push_back(liquidMaterial);

	OBJECT_MATERIAL GlowingFMaterial;
	GlowingFMaterial.diffuseColor = glm::vec3(0.929f, 0.961f, 0.424f);
	GlowingFMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	GlowingFMaterial.shininess = 0.70;
	GlowingFMaterial.tag = "felixGlow";

	m_objectMaterials.push_back(GlowingFMaterial);

	OBJECT_MATERIAL GlowingLMaterial;
	GlowingLMaterial.diffuseColor = glm::vec3(0.922f, 0.435f, 0.773f);
	GlowingLMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	GlowingLMaterial.shininess = 0.70;
	GlowingLMaterial.tag = "loveGlow";

	m_objectMaterials.push_back(GlowingLMaterial);

}

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// directional light to emulate sunlight coming into scene
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.05f, -0.3f, -0.1f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	// point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
	// point light 3
	m_pShaderManager->setVec3Value("pointLights[2].position", 3.8f, 5.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
	// point light 4
	m_pShaderManager->setVec3Value("pointLights[3].position", 5.0f, 6.5f, 6.0f);
	m_pShaderManager->setVec3Value("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[3].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);


}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	LoadSceneTextures(); // load texture image files to scene
	DefineObjectMaterials();
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderBackground();
	RenderTable();
	RenderDraughtLivingDeath();
	RenderAmortentia();
	RenderThunderbrew();
	RenderFelix();
	RenderFlooPowder();
}

void SceneManager::RenderBackground()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 0.5f, -10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -9.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.929, 0.835, 0.784, 1); // changed the colors values to match the wall
	SetShaderTexture("background");
	SetTextureUVScale(5.0f, 5.0f);

	SetShaderMaterial("wall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}

void SceneManager::RenderTable()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	//scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	scaleXYZ = glm::vec3(20.0f, 0.6f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.3f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.435, 0.224, 0.102, 1); // changed the colors values to vaguely match that of a wooden table !to be changed to texture later
	SetShaderTexture("table");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::RenderDraughtLivingDeath()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Adds main bottle shape to "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 3.0f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.153, 0.125, 0.153, 1); // changes the color to a deep purple
	SetShaderTexture("draught-potion");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("liquid");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/*** Adds tapered neck to bottle shape of "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.5f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.0f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/
	/*** Adds main neck shape to "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 1.2f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.0f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/*** Adds lip to bottle neck of "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates tours 90 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 4.2f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
	/*** Adds twine to bottle neck of "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates torus 90 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 4.0f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("black-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
	/*** Adds twine to bottle neck of "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -92.0f; // rotates torus 90 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.9f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("black-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
	/*** Adds twine to bottle neck of "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 89.0f; // rotates torus 89 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.8f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("black-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to bottle neck of "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -95.0f; // rotates torus -95 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.7f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("black-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to bottle neck of "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates torus -95 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.6f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("black-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds cork in neck to "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 4.0f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.569, 0.392, 0.286, 1); // changes the color to a "corkish" brown
	SetShaderTexture("bottle-cork");
	SetTextureUVScale(1.0f,1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}

void SceneManager::RenderAmortentia()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Adds main bottle shape to "Draught of Living Death" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 1.8f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -5.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.6f, 0.0f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.153, 0.120, 0.153, 1); // changes the color to a deep purple
	SetShaderTexture("love-potion");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("loveGlow");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Adds tapered neck to bottle shape of "Amortentia" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.6f, 1.8f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	/*** Adds main neck shape to "Amortentia" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.8f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.6f, 2.0f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Adds lip to bottle neck of "Amortentia" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates tours 90 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.6f, 2.8f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds cork in neck to "Amortentia" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.2f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.6f, 2.8f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.569, 0.392, 0.286, 1); // changes the color to a "corkish" brown
	SetShaderTexture("bottle-cork");

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Adds twine to bottle neck of "Amortentia" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates torus -95 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.6f, 2.4f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to bottle neck of "Amortentia" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 93.0f; // rotates torus -95 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.6f, 2.3f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to bottle neck of "Amortentia" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -94.0f; // rotates torus -95 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.6f, 2.5f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to bottle neck of "Amortentia" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -91.0f; // rotates torus -95 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.6f, 2.6f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
}

void SceneManager::RenderThunderbrew() 
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Adds main bottle shape to "Thunderbew" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 2.5f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.2f, 0.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.153, 0.125, 0.153, 1); // changes the color to a deep purple
	SetShaderTexture("stun-potion");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("liquid");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Adds tapered neck to bottle shape of "Thunderbew" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.5f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.2f, 2.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	/*** Adds main neck shape to "Thunderbew" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 1.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.2f, 2.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Adds lip to bottle neck of "Thunderbew" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates tours 90 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.2f, 3.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to bottle neck of "Thunderbew" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates torus 90 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.2f, 3.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to bottle neck of "Thunderbew" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -92.0f; // rotates torus 90 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.2f, 3.3f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to bottle neck of "Thunderbew" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 89.0f; // rotates torus 89 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.2f, 3.1f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to bottle neck of "Thunderbew" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -95.0f; // rotates torus -95 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.2f, 3.2f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds cork in neck to "Thunderbew" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.2f, 3.3f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.569, 0.392, 0.286, 1); // changes the color to a "corkish" brown
	SetShaderTexture("bottle-cork");

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
}

void SceneManager::RenderFelix() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Adds main bottle shape to "Felix" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.3f, 1.4f, 1.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 1.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.153, 0.125, 0.153, 1); // changes the color to a deep purple
	SetShaderTexture("lucky-potion");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("felixGlow");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/
	/*** Adds main neck shape to "Felix" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 1.3f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 1.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.153, 0.125, 0.153, 1); // changes the color to a deep purple
	SetShaderTexture("lucky-potion");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("felixGlow");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	/*** Adds main neck shape to "Felix" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 1.0f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 2.6f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Adds lip to bottle neck of "Felix" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.4f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 3.6f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds cork in neck to "Felix" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 3.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.569, 0.392, 0.286, 1); // changes the color to a "corkish" brown
	SetShaderTexture("bottle-cork");

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Adds handle to "Felix" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.2f, 3.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to "Felix" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.4f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates torus 89 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 3.3f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to "Felix" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.4f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates torus 89 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 3.4f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to "Felix" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.4f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates torus 89 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 3.45f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds twine to "Felix" potion ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.4f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates torus 89 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 3.5f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.067, 0.067, 0.071, 1); // changes the color to a black
	SetShaderTexture("brown-twine");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("twine");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
}

void SceneManager::RenderFlooPowder() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Adds main bottle shape to Floo Powder ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 1.8f, 1.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.5f, 0.0f, -3.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Adds lid to Floo Powder ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 0.5f, 1.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f; // rotates torus 89 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.5f, 1.8f, -3.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();

	/****************************************************************/
	/*** Adds lid siding to Floo Powder ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.1f, 1.1f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // rotates torus 90 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.5f, 1.8f, -3.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Adds lid handle to Floo Powder ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.6f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f; // rotates torus 90 degrees
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.5f, 2.0f, -3.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.827, 0.824, 0.902, 0.8); // changes the color to a blue with slight translucency to represent a bottle "glass" look
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
}