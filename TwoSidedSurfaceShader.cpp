/***********************************************************************
TwoSidedSurfaceShader - Class to simulate OpenGL two-sided lighting
without the ridiculous and arbitrary performance penalty it incurs on
newer Nvidia Geforce graphics cards (shame on you, Nvidia!).
Copyright (c) 2012-2023 Oliver Kreylos

This file is part of the 3D Data Visualizer (Visualizer).

The 3D Data Visualizer is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The 3D Data Visualizer is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the 3D Data Visualizer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <TwoSidedSurfaceShader.h>

#include <Misc/PrintInteger.h>
#include <GL/gl.h>
#include <GL/GLLightTracker.h>
#include <GL/GLClipPlaneTracker.h>
#include <GL/GLContextData.h>
#include <SceneGraph/GLRenderState.h>

/************************************************
Methods of class TwoSidedSurfaceShader::DataItem:
************************************************/

TwoSidedSurfaceShader::DataItem::DataItem(void)
	:lightTrackerVersion(0),clipPlaneTrackerVersion(0)
	{
	}

TwoSidedSurfaceShader::DataItem::~DataItem(void)
	{
	}

void TwoSidedSurfaceShader::DataItem::buildShader(GLLightTracker& lt,GLClipPlaneTracker& cpt)
	{
	/* Reset the shader: */
	shader.reset();
	
	/* Start the vertex shader's main function: */
	std::string vertexShaderMain="\
		varying vec4 frontColor;\n\
		varying vec4 backColor;\n\
		\n\
		void main()\n\
			{\n\
			/* Compute the vertex position and normal vector in eye space: */\n\
			vec4 vertexEc=gl_ModelViewMatrix*gl_Vertex;\n\
			vec3 normalEc=normalize(gl_NormalMatrix*gl_Normal);\n\
			\n\
			/* Initialize the color accumulators: */\n\
			vec4 ambientDiffuseAccumulator=gl_LightModel.ambient*gl_FrontMaterial.ambient;\n\
			vec4 specularAccumulator=vec4(0.0,0.0,0.0,0.0);\n\
			\n";
	
	/* Call the appropriate light accumulation function for every enabled light source: */
	std::string vertexShaderFunctions;
	for(int lightIndex=0;lightIndex<lt.getMaxNumLights();++lightIndex)
		if(lt.getLightState(lightIndex).isEnabled())
			{
			/* Create the light accumulation function: */
			vertexShaderFunctions+=lt.createAccumulateLightFunction(lightIndex);
			
			/* Call the light accumulation function from the vertex shader's main function: */
			vertexShaderMain+="\
				accumulateLight";
			char liBuffer[12];
			vertexShaderMain.append(Misc::print(lightIndex,liBuffer+11));
			vertexShaderMain+="(vertexEc,normalEc,gl_FrontMaterial.ambient,gl_FrontMaterial.diffuse,gl_FrontMaterial.specular,gl_FrontMaterial.shininess,ambientDiffuseAccumulator,specularAccumulator);\n";
			}
	
	/* Continue the vertex shader's main function: */
	vertexShaderMain+="\
			\n\
			/* Assign the final accumulated vertex color: */\n\
			frontColor=ambientDiffuseAccumulator+specularAccumulator;\n\
			\n\
			/* Flip the normal vector to calculate back-face illumination: */\n\
			normalEc=-normalEc;\n\
			\n\
			/* Re-initialize the color accumulators: */\n\
			ambientDiffuseAccumulator=gl_LightModel.ambient*gl_BackMaterial.ambient;\n\
			specularAccumulator=vec4(0.0,0.0,0.0,0.0);\n\
			\n";
	
	/* Call the appropriate light accumulation function for every enabled light source: */
	for(int lightIndex=0;lightIndex<lt.getMaxNumLights();++lightIndex)
		if(lt.getLightState(lightIndex).isEnabled())
			{
			/* Call the previously created light accumulation function from the vertex shader's main function: */
			vertexShaderMain+="\
				accumulateLight";
			char liBuffer[12];
			vertexShaderMain.append(Misc::print(lightIndex,liBuffer+11));
			vertexShaderMain+="(vertexEc,normalEc,gl_BackMaterial.ambient,gl_BackMaterial.diffuse,gl_BackMaterial.specular,gl_BackMaterial.shininess,ambientDiffuseAccumulator,specularAccumulator);\n";
			}
	
	/* Continue the vertex shader's main function: */
	vertexShaderMain+="\
			\n\
			/* Assign the final accumulated vertex color: */\n\
			backColor=ambientDiffuseAccumulator+specularAccumulator;\n\
			\n";
	
	/* Insert code to calculate the vertex' position relative to all user-specified clipping planes: */
	vertexShaderMain+=cpt.createCalcClipDistances("vertexEc");
	
	/* Finish the vertex shader's main function: */
	vertexShaderMain+="\
			\n\
			/* Use standard vertex position: */\n\
			gl_Position=ftransform();\n\
			}\n";
	
	/* Assemble the full vertex shader source: */
	std::string vertexShader=vertexShaderFunctions+vertexShaderMain;
	
	/* Compile the vertex shader: */
	shader.compileVertexShaderFromString(vertexShader.c_str());
	
	/* Assemble the fragment shader's source: */
	std::string fragmentShaderMain="\
		varying vec4 frontColor;\n\
		varying vec4 backColor;\n\
		\n\
		void main()\n\
			{\n\
			if(gl_FrontFacing)\n\
				gl_FragColor=frontColor;\n\
			else\n\
				gl_FragColor=backColor;\n\
			}\n";
	
	/* Compile the fragment shader: */
	shader.compileFragmentShaderFromString(fragmentShaderMain.c_str());
	
	/* Link the shader: */
	shader.linkShader();
	}

/**********************************************
Static elements of class TwoSidedSurfaceShader:
**********************************************/

Threads::Spinlock TwoSidedSurfaceShader::theShaderMutex;
unsigned int TwoSidedSurfaceShader::theShaderRefCount(0);
TwoSidedSurfaceShader* TwoSidedSurfaceShader::theShader=0;

/**************************************
Methods of class TwoSidedSurfaceShader:
**************************************/

bool TwoSidedSurfaceShader::isSupported(GLContextData& contextData)
	{
	/* Return true if shaders are supported: */
	return GLShader::isSupported();
	}

TwoSidedSurfaceShader* TwoSidedSurfaceShader::acquireShader(void)
	{
	Threads::Spinlock::Lock theShaderLock(theShaderMutex);
	
	/* Check if the shared shader has no references yet: */
	if(theShaderRefCount==0)
		{
		/* Allocate a new shared shader object: */
		theShader=new TwoSidedSurfaceShader;
		}
	
	/* Increment the reference counter and return the shared shader object: */
	++theShaderRefCount;
	return theShader;
	}

void TwoSidedSurfaceShader::releaseShader(TwoSidedSurfaceShader* shader)
	{
	Threads::Spinlock::Lock theShaderLock(theShaderMutex);
	
	/* Check if the to-be-released shader is really the shared shader: */
	if(shader!=0&&shader==theShader)
		{
		/* Decrement the reference counter: */
		--theShaderRefCount;
		
		/* Check if the shared shader can be deleted: */
		if(theShaderRefCount==0)
			{
			delete theShader;
			theShader=0;
			}
		}
	}

TwoSidedSurfaceShader::TwoSidedSurfaceShader(void)
	{
	}

TwoSidedSurfaceShader::~TwoSidedSurfaceShader(void)
	{
	}

void TwoSidedSurfaceShader::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

void TwoSidedSurfaceShader::set(SceneGraph::GLRenderState& renderState) const
	{
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	/* Check if the shader needs to be updated: */
	GLLightTracker& lt=*renderState.contextData.getLightTracker();
	GLClipPlaneTracker& cpt=*renderState.contextData.getClipPlaneTracker();
	if(dataItem->lightTrackerVersion!=lt.getVersion()||dataItem->clipPlaneTrackerVersion!=cpt.getVersion())
		{
		/* Rebuild the shader: */
		dataItem->buildShader(lt,cpt);
		
		/* Mark the shader as up-to-date: */
		dataItem->lightTrackerVersion=lt.getVersion();
		dataItem->clipPlaneTrackerVersion=cpt.getVersion();
		}
	
	/* Install the shader: */
	renderState.bindShader(dataItem->shader.getProgramObject());
	}
