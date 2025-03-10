/***********************************************************************
Raycaster - Base class for volume renderers for Cartesian gridded data
using GLSL shaders.
Copyright (c) 2007-2024 Oliver Kreylos

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

#include <Raycaster.h>

#include <Misc/StdError.h>
#include <Math/Math.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLMiscTemplates.h>
#include <GL/GLClipPlaneTracker.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBDepthTexture.h>
#include <GL/Extensions/GLARBMultitexture.h>
#include <GL/Extensions/GLARBShadow.h>
#include <GL/Extensions/GLARBTextureNonPowerOfTwo.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <GL/Extensions/GLEXTTexture3D.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <SceneGraph/GLRenderState.h>
#include <Vrui/Vrui.h>
#include <Vrui/DisplayState.h>

/************************************
Methods of class Raycaster::DataItem:
************************************/

Raycaster::DataItem::DataItem(void)
	:hasNPOTDTextures(GLARBTextureNonPowerOfTwo::isSupported()),
	 textureSize(0,0,0),
	 depthTextureID(0),depthFramebufferID(0),depthTextureSize(1,1),
	 mcScaleLoc(-1),mcOffsetLoc(-1),
	 depthSamplerLoc(-1),depthMatrixLoc(-1),depthSizeLoc(-1),
	 eyePositionLoc(-1),stepSizeLoc(-1)
	{
	/* Check for the required OpenGL extensions: */
	if(!GLShader::isSupported())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Shader objects not supported by local OpenGL");
	//if(!GLARBMultitexture::isSupported()||!GLEXTTexture3D::isSupported())
	//	throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Multitexture or 3D texture extension not supported by local OpenGL");
	if(!GLEXTFramebufferObject::isSupported()||!GLARBDepthTexture::isSupported()||!GLARBShadow::isSupported())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Framebuffer object extension or depth/shadow texture extension not supported by local OpenGL");
	
	/* Initialize all required OpenGL extensions: */
	GLARBDepthTexture::initExtension();
	GLARBMultitexture::initExtension();
	GLARBShadow::initExtension();
	if(hasNPOTDTextures)
		GLARBTextureNonPowerOfTwo::initExtension();
	GLEXTFramebufferObject::initExtension();
	GLEXTTexture3D::initExtension();
	
	/* Create the depth texture: */
	glGenTextures(1,&depthTextureID);
	glBindTexture(GL_TEXTURE_2D,depthTextureID);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE_ARB,GL_NONE);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24_ARB,depthTextureSize,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,0);
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Create the depth framebuffer and attach the depth texture to it: */
	glGenFramebuffersEXT(1,&depthFramebufferID);
	GLint currentFramebuffer;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&currentFramebuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,depthFramebufferID);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,depthTextureID,0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,currentFramebuffer);
	}

Raycaster::DataItem::~DataItem(void)
	{
	/* Destroy the depth texture and framebuffer: */
	glDeleteFramebuffersEXT(1,&depthFramebufferID);
	glDeleteTextures(1,&depthTextureID);
	}

void Raycaster::DataItem::initDepthBuffer(const Size2& maxFrameSize,SceneGraph::GLRenderState& renderState)
	{
	/* Calculate the new depth texture size: */
	Size2 newDepthTextureSize;
	if(hasNPOTDTextures)
		{
		/* Use the maximum frame buffer size in the current window group: */
		newDepthTextureSize=maxFrameSize;
		}
	else
		{
		/* Pad the maximum frame buffer size to the next power of two: */
		for(int i=0;i<2;++i)
			for(newDepthTextureSize[i]=1;newDepthTextureSize[i]<maxFrameSize[i];newDepthTextureSize[i]<<=1)
				;
		}
	
	/* Bind the depth texture: */
	renderState.bindTexture2D(depthTextureID);
	
	/* Check if the depth texture size needs to change: */
	if(depthTextureSize!=newDepthTextureSize)
		{
		/* Re-allocate the depth texture: */
		glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24_ARB,newDepthTextureSize,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,0);
		depthTextureSize=newDepthTextureSize;
		}
	
	/* Copy the current depth buffer from the current viewport into the depth texture: */
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,renderState.getViewport().offset,renderState.getViewport());
	
	/* Unbind the depth texture so it can be used as a frame buffer attachment: */
	renderState.bindTexture2D(0);
	}

/**************************
Methods of class Raycaster:
**************************/

void Raycaster::initDataItem(Raycaster::DataItem* dataItem) const
	{
	/* Calculate the appropriate volume texture's size: */
	if(dataItem->hasNPOTDTextures)
		{
		/* Use the data size directly: */
		dataItem->textureSize=dataSize;
		}
	else
		{
		/* Pad to the next power of two: */
		for(int i=0;i<3;++i)
			for(dataItem->textureSize[i]=1;dataItem->textureSize[i]<dataSize[i];dataItem->textureSize[i]<<=1)
				;
		}
	
	/* Calculate the texture coordinate box for trilinear interpolation and the transformation from model space to data space: */
	Point tcMin,tcMax;
	for(int i=0;i<3;++i)
		{
		tcMin[i]=Scalar(0.5)/Scalar(dataItem->textureSize[i]);
		tcMax[i]=(Scalar(dataSize[i])-Scalar(0.5))/Scalar(dataItem->textureSize[i]);
		Scalar scale=(tcMax[i]-tcMin[i])/domain.getSize(i);
		dataItem->mcScale[i]=GLfloat(scale);
		dataItem->mcOffset[i]=GLfloat(tcMin[i]-domain.min[i]*scale);
		}
	dataItem->texCoords=Box(tcMin,tcMax);
	}

void Raycaster::initShader(Raycaster::DataItem* dataItem) const
	{
	/* Get the shader's uniform locations: */
	dataItem->mcScaleLoc=dataItem->shader.getUniformLocation("mcScale");
	dataItem->mcOffsetLoc=dataItem->shader.getUniformLocation("mcOffset");
	
	dataItem->depthSamplerLoc=dataItem->shader.getUniformLocation("depthSampler");
	dataItem->depthMatrixLoc=dataItem->shader.getUniformLocation("depthMatrix");
	dataItem->depthSizeLoc=dataItem->shader.getUniformLocation("depthSize");
	
	dataItem->eyePositionLoc=dataItem->shader.getUniformLocation("eyePosition");
	dataItem->stepSizeLoc=dataItem->shader.getUniformLocation("stepSize");
	}

void Raycaster::bindShader(const Raycaster::PTransform& pmv,const Raycaster::PTransform& mv,SceneGraph::GLRenderState& renderState,Raycaster::DataItem* dataItem) const
	{
	/* Set up the data space transformation: */
	glUniform3fvARB(dataItem->mcScaleLoc,1,dataItem->mcScale);
	glUniform3fvARB(dataItem->mcOffsetLoc,1,dataItem->mcOffset);
	
	/* Bind the ray termination texture: */
	glActiveTextureARB(GL_TEXTURE0_ARB);
	renderState.bindTexture2D(dataItem->depthTextureID);
	glUniform1iARB(dataItem->depthSamplerLoc,0);
	
	/* Set the termination matrix: */
	glUniformMatrix4fvARB(dataItem->depthMatrixLoc,1,GL_TRUE,pmv.getMatrix().getEntries());
	
	/* Set the depth texture size: */
	glUniform2fARB(dataItem->depthSizeLoc,float(dataItem->depthTextureSize[0]),float(dataItem->depthTextureSize[1]));
	
	/* Calculate the eye position in model coordinates: */
	Point eye=renderState.getEyePos();
	glUniform3fvARB(dataItem->eyePositionLoc,1,eye.getComponents());
	
	/* Set the sampling step size: */
	glUniform1fARB(dataItem->stepSizeLoc,stepSize*cellSize);
	}

void Raycaster::unbindShader(SceneGraph::GLRenderState& renderState,Raycaster::DataItem* dataItem) const
	{
	/* Reset the active texture unit: */
	glActiveTextureARB(GL_TEXTURE0_ARB);
	}

Polyhedron<Raycaster::Scalar>* Raycaster::clipDomain(const Raycaster::PTransform& mv,SceneGraph::GLRenderState& renderState) const
	{
	typedef Polyhedron<Scalar> PH;
	
	/* Clip the render domain against the view frustum's front plane: */
	SceneGraph::GLRenderState::DPlane frontPlane=renderState.getFrustumPlane(4).flip();
	PH* clippedDomain=renderDomain.clip(frontPlane);
	
	GLClipPlaneTracker& cpt=*renderState.contextData.getClipPlaneTracker();
	for(int cpi=0;cpi<cpt.getMaxNumClipPlanes();++cpi)
		{
		const GLClipPlaneTracker::ClipPlaneState& cps=cpt.getClipPlaneState(cpi);
		if(cps.isEnabled())
			{
			/* Transform the clipping plane to current model coordinates: */
			Geometry::ComponentArray<double,4> cp;
			for(int i=0;i<3;++i)
				cp[i]=-cps.getPlane()[i];
			cp[3]=-cps.getPlane()[3];
			cp=mv.getMatrix().transposeMultiply(cp);
			
			/* Clip the render domain against the clipping plane: */
			PH* newClippedDomain=clippedDomain->clip(PH::Plane(PH::Plane::Vector(cp.getComponents()),-cp[3]));
			delete clippedDomain;
			clippedDomain=newClippedDomain;
			}
		}
	
	return clippedDomain;
	}

Raycaster::Raycaster(const Raycaster::Size3& sDataSize,const Raycaster::Box& sDomain)
	:GLObject(false),
	 dataSize(sDataSize),
	 domain(sDomain),domainExtent(0),cellSize(0),
	 renderDomain(Polyhedron<Scalar>::Point(domain.min),Polyhedron<Scalar>::Point(domain.max)),
	 stepSize(1)
	{
	/* Calculate the data strides and cell size: */
	ptrdiff_t stride=1;
	for(int i=0;i<3;++i)
		{
		dataStrides[i]=stride;
		stride*=ptrdiff_t(dataSize[i]);
		domainExtent+=Math::sqr(domain.max[i]-domain.min[i]);
		cellSize+=Math::sqr((domain.max[i]-domain.min[i])/Scalar(dataSize[i]-1));
		}
	domainExtent=Math::sqrt(domainExtent);
	cellSize=Math::sqrt(cellSize);
	
	GLObject::init();
	}

Raycaster::~Raycaster(void)
	{
	}

void Raycaster::setStepSize(Raycaster::Scalar newStepSize)
	{
	/* Set the new step size: */
	stepSize=newStepSize;
	}

void Raycaster::glRenderAction(SceneGraph::GLRenderState& renderState) const
	{
	/* Get the OpenGL-dependent application data from the GLContextData object: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	/* Bail out if shader is invalid: */
	if(!dataItem->shader.isValid())
		return;
	
	/* Retrieve Vrui's display state: */
	const Vrui::DisplayState& displayState=Vrui::getDisplayState(renderState.contextData);
	
	/* Initialize the ray termination depth frame buffer: */
	dataItem->initDepthBuffer(displayState.maxFrameSize,renderState);
	
	/* Bind the ray termination framebuffer: */
	GLint currentFramebuffer;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&currentFramebuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,dataItem->depthFramebufferID);
	
	/* Get the projection and modelview matrices: */
	PTransform pmv=renderState.getProjection();
	PTransform mv(renderState.getTransform());
	pmv*=mv;
	
	/* Clip the render domain against the view frustum's front plane and all clipping planes: */
	Polyhedron<Scalar>* clippedDomain=clipDomain(mv,renderState);
	
	/* Draw the clipped domain's back faces to the depth buffer as ray termination conditions: */
	renderState.setFrontFace(GL_CCW);
	renderState.enableCulling(GL_FRONT);
	glDepthMask(GL_TRUE);
	renderState.uploadModelview();
	clippedDomain->drawFaces();
	glDepthMask(GL_FALSE);
	
	/* Unbind the depth framebuffer: */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,currentFramebuffer);
	
	/* Install the GLSL shader program: */
	renderState.bindShader(dataItem->shader.getProgramObject());
	bindShader(pmv,mv,renderState,dataItem);
	
	/* Draw the clipped domain's front faces: */
	renderState.enableCulling(GL_BACK);
	clippedDomain->drawFaces();
	
	/* Uninstall the GLSL shader program: */
	unbindShader(renderState,dataItem);
	
	/* Clean up: */
	delete clippedDomain;
	}
