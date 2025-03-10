/***********************************************************************
VariableManager - Helper class to manage the scalar and vector variables
that can be extracted from a data set.
Copyright (c) 2008-2023 Oliver Kreylos

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

#include <Abstract/VariableManager.h>

#include <string.h>
#include <stdio.h>
#include <stdexcept>
#include <Misc/CreateNumberedFileName.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLColorMap.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupWindow.h>
#include <SceneGraph/GLRenderState.h>
#include <Vrui/Vrui.h>

#include <Abstract/ScalarExtractor.h>
#include <Abstract/VectorExtractor.h>

#include <ColorBar.h>
#include <ColorMap.h>

namespace Visualization {

namespace Abstract {

/************************************************
Methods of class VariableManager::ScalarVariable:
************************************************/

VariableManager::ScalarVariable::ScalarVariable(void)
	:scalarExtractor(0),
	 colorMap(0),
	 colorMapVersion(0),
	 palette(0)
	{
	}

VariableManager::ScalarVariable::~ScalarVariable(void)
	{
	delete scalarExtractor;
	delete colorMap;
	delete palette;
	}

/************************************************
Methods of class VariableManager::VectorVariable:
************************************************/

VariableManager::VectorVariable::VectorVariable(void)
	:vectorExtractor(0)
	{
	}

VariableManager::VectorVariable::~VectorVariable(void)
	{
	delete vectorExtractor;
	}

/******************************************
Methods of class VariableManager::DataItem:
******************************************/

VariableManager::DataItem::DataItem(unsigned int sNumScalarVariables)
	:numScalarVariables(sNumScalarVariables),
	 colorMapTextureIds(new GLuint[numScalarVariables]),
	 colorMapVersions(new unsigned int[numScalarVariables])
	{
	/* Initialize the color map texture array: */
	glGenTextures(numScalarVariables,colorMapTextureIds);
	for(unsigned int i=0;i<numScalarVariables;++i)
		colorMapVersions[i]=0;
	}

VariableManager::DataItem::~DataItem(void)
	{
	/* Delete the color map texture array: */
	glDeleteTextures(numScalarVariables,colorMapTextureIds);
	delete[] colorMapTextureIds;
	delete[] colorMapVersions;
	}

/********************************
Methods of class VariableManager:
********************************/

void VariableManager::prepareScalarVariable(int scalarVariableIndex)
	{
	ScalarVariable& sv=scalarVariables[scalarVariableIndex];
	
	/* Get a new scalar extractor: */
	sv.scalarExtractor=dataSet->getScalarExtractor(scalarVariableIndex);
	
	/* Calculate the scalar extractor's value range: */
	sv.valueRange=dataSet->calcScalarValueRange(sv.scalarExtractor);
	
	/* Check for and correct an empty value range: */
	if(sv.valueRange.first==sv.valueRange.second)
		{
		sv.valueRange.first-=1.0;
		sv.valueRange.second+=1.0;
		}
	
	/* Create a 256-entry OpenGL color map for rendering: */
	sv.colorMap=new GLColorMap(GLColorMap::GREYSCALE|GLColorMap::RAMP_ALPHA,1.0f,1.0f,sv.valueRange.first,sv.valueRange.second);
	++sv.colorMapVersion;
	
	/* Initialize the color map range to the variable's full scalar range: */
	sv.colorMapRange=sv.valueRange;
	}

void VariableManager::colorMapChangedCallback(Misc::CallbackData* cbData)
	{
	/* Export the changed palette to the current color map: */
	paletteEditor->exportColorMap(*scalarVariables[currentScalarVariableIndex].colorMap);
	++scalarVariables[currentScalarVariableIndex].colorMapVersion;
	
	Vrui::requestUpdate();
	}

void VariableManager::savePaletteCallback(Misc::CallbackData* cbData)
	{
	if(Vrui::isHeadNode())
		{
		try
			{
			char numberedFileName[40];
			paletteEditor->savePalette(Misc::createNumberedFileName("SavedPalette.pal",4,numberedFileName));
			}
		catch(const std::runtime_error&)
			{
			/* Ignore errors and carry on: */
			}
		}
	}

VariableManager::VariableManager(const DataSet* sDataSet,const char* sDefaultColorMapName)
	:GLObject(false),
	 dataSet(sDataSet),
	 defaultColorMapName(0),
	 scalarVariables(0),
	 colorBarDialogPopup(0),colorBar(0),
	 paletteEditor(0),
	 vectorVariables(0),
	 currentScalarVariableIndex(-1),currentVectorVariableIndex(-1)
	{
	if(sDefaultColorMapName!=0)
		{
		/* Store the default color map name: */
		int nameLength=strlen(sDefaultColorMapName);
		defaultColorMapName=new char[nameLength+1];
		memcpy(defaultColorMapName,sDefaultColorMapName,nameLength+1);
		}
	
	/* Initialize the scalar variable array: */
	numScalarVariables=dataSet->getNumScalarVariables();
	if(numScalarVariables>0)
		scalarVariables=new ScalarVariable[numScalarVariables];
	
	/* Get the style sheet: */
	const GLMotif::StyleSheet& ss=*Vrui::getUiStyleSheet();
	
	/* Create the color bar dialog: */
	colorBarDialogPopup=new GLMotif::PopupWindow("ColorBarDialogPopup",Vrui::getWidgetManager(),"Color Bar");
	
	/* Create the color bar widget: */
	colorBar=new GLMotif::ColorBar("ColorBar",colorBarDialogPopup,ss.fontHeight*5.0f,6,5);
	
	/* Create the palette editor: */
	paletteEditor=new PaletteEditor;
	paletteEditor->getColorMapChangedCallbacks().add(this,&VariableManager::colorMapChangedCallback);
	paletteEditor->getSavePaletteCallbacks().add(this,&VariableManager::savePaletteCallback);
	
	/* Initialize the vector variable array: */
	numVectorVariables=dataSet->getNumVectorVariables();
	if(numVectorVariables>0)
		vectorVariables=new VectorVariable[numVectorVariables];
	
	/* Initialize the current variable state: */
	setCurrentScalarVariable(0);
	setCurrentVectorVariable(0);
	
	GLObject::init();
	}

VariableManager::~VariableManager(void)
	{
	delete[] defaultColorMapName;
	delete[] scalarVariables;
	delete[] vectorVariables;
	delete colorBarDialogPopup;
	delete paletteEditor;
	}

void VariableManager::initContext(GLContextData& contextData) const
	{
	/* Create a data item and register it with the OpenGL context: */
	DataItem* dataItem=new DataItem(numScalarVariables);
	contextData.addDataItem(this,dataItem);
	}

const DataSet* VariableManager::getDataSetByScalarVariable(int scalarVariableIndex) const
	{
	if(scalarVariableIndex<0||scalarVariableIndex>=numScalarVariables)
		return 0;
	
	return dataSet;
	}

const DataSet* VariableManager::getDataSetByVectorVariable(int vectorVariableIndex) const
	{
	if(vectorVariableIndex<0||vectorVariableIndex>=numVectorVariables)
		return 0;
	
	return dataSet;
	}

int VariableManager::getScalarVariable(const char* scalarVariableName) const
	{
	for(int i=0;i<numScalarVariables;++i)
		if(strcmp(getScalarVariableName(i),scalarVariableName)==0)
			return i;
	
	return -1;
	}

int VariableManager::getVectorVariable(const char* vectorVariableName) const
	{
	for(int i=0;i<numVectorVariables;++i)
		if(strcmp(getVectorVariableName(i),vectorVariableName)==0)
			return i;
	
	return -1;
	}

void VariableManager::setCurrentScalarVariable(int newCurrentScalarVariableIndex)
	{
	if(currentScalarVariableIndex==newCurrentScalarVariableIndex||newCurrentScalarVariableIndex<0||newCurrentScalarVariableIndex>=numScalarVariables)
		return;
	
	/* Check if the scalar variable has not been requested before: */
	ScalarVariable& sv=scalarVariables[newCurrentScalarVariableIndex];
	if(sv.scalarExtractor==0)
		prepareScalarVariable(newCurrentScalarVariableIndex);
	
	/* Save the palette editor's current palette: */
	if(currentScalarVariableIndex>=0)
		scalarVariables[currentScalarVariableIndex].palette=paletteEditor->getPalette();
	
	/* Update the current scalar variable: */
	currentScalarVariableIndex=newCurrentScalarVariableIndex;
	
	if(sv.palette==0)
		{
		if(defaultColorMapName!=0)
			{
			/* Load the default palette: */
			try
				{
				paletteEditor->loadPalette(defaultColorMapName,sv.valueRange);
				}
			catch(const std::runtime_error&)
				{
				/* Create a new palette: */
				paletteEditor->createPalette(GLMotif::ColorMap::GREYSCALE,sv.valueRange);
				}
			}
		else
			{
			/* Create a new palette: */
			paletteEditor->createPalette(GLMotif::ColorMap::GREYSCALE,sv.valueRange);
			}
		}
	else
		{
		/* Upload the previously stored palette: */
		paletteEditor->setPalette(sv.palette);
		delete sv.palette;
		sv.palette=0;
		}
	
	/* Update the palette editor's title: */
	char title[256];
	snprintf(title,sizeof(title),"Palette Editor - %s",dataSet->getScalarVariableName(newCurrentScalarVariableIndex));
	paletteEditor->setTitleString(title);

	/* Update the color bar dialog: */
	snprintf(title,sizeof(title),"Color Bar - %s",dataSet->getScalarVariableName(newCurrentScalarVariableIndex));
	colorBarDialogPopup->setTitleString(title);
	colorBar->setColorMap(sv.colorMap);
	colorBar->setValueRange(sv.valueRange.first,sv.valueRange.second);
	}

void VariableManager::setCurrentVectorVariable(int newCurrentVectorVariableIndex)
	{
	if(currentVectorVariableIndex==newCurrentVectorVariableIndex||newCurrentVectorVariableIndex<0||newCurrentVectorVariableIndex>=numVectorVariables)
		return;
	
	/* Check if the vector variable has not been requested before: */
	if(vectorVariables[newCurrentVectorVariableIndex].vectorExtractor==0)
		vectorVariables[newCurrentVectorVariableIndex].vectorExtractor=dataSet->getVectorExtractor(newCurrentVectorVariableIndex);
	
	/* Update the current vector variable: */
	currentVectorVariableIndex=newCurrentVectorVariableIndex;
	}

const ScalarExtractor* VariableManager::getScalarExtractor(int scalarVariableIndex)
	{
	if(scalarVariableIndex<0||scalarVariableIndex>=numScalarVariables)
		return 0;
	
	/* Check if the scalar variable has not been requested before: */
	if(scalarVariables[scalarVariableIndex].scalarExtractor==0)
		prepareScalarVariable(scalarVariableIndex);
	
	return scalarVariables[scalarVariableIndex].scalarExtractor;
	}

int VariableManager::getScalarVariable(const ScalarExtractor* scalarExtractor) const
	{
	/* Find the scalar extractor among the registered extractors: */
	for(int i=0;i<numScalarVariables;++i)
		if(scalarVariables[i].scalarExtractor==scalarExtractor)
			return i;
	
	return -1;
	}

const DataSet::VScalarRange& VariableManager::getScalarValueRange(int scalarVariableIndex)
	{
	if(scalarVariableIndex<0||scalarVariableIndex>=numScalarVariables)
		return scalarVariables[currentScalarVariableIndex].valueRange;
	
	/* Check if the scalar variable has not been requested before: */
	if(scalarVariables[scalarVariableIndex].scalarExtractor==0)
		prepareScalarVariable(scalarVariableIndex);
	
	return scalarVariables[scalarVariableIndex].valueRange;
	}

const GLColorMap* VariableManager::getColorMap(int scalarVariableIndex)
	{
	if(scalarVariableIndex<0||scalarVariableIndex>=numScalarVariables)
		return 0;
	
	/* Check if the scalar variable has not been requested before: */
	if(scalarVariables[scalarVariableIndex].scalarExtractor==0)
		prepareScalarVariable(scalarVariableIndex);
	
	return scalarVariables[scalarVariableIndex].colorMap;
	}

const DataSet::VScalarRange& VariableManager::getScalarColorMapRange(int scalarVariableIndex)
	{
	if(scalarVariableIndex<0||scalarVariableIndex>=numScalarVariables)
		return scalarVariables[currentScalarVariableIndex].colorMapRange;
	
	/* Check if the scalar variable has not been requested before: */
	if(scalarVariables[scalarVariableIndex].scalarExtractor==0)
		prepareScalarVariable(scalarVariableIndex);
	
	return scalarVariables[scalarVariableIndex].colorMapRange;
	}

const VectorExtractor* VariableManager::getVectorExtractor(int vectorVariableIndex)
	{
	if(vectorVariableIndex<0||vectorVariableIndex>=numVectorVariables)
		return 0;
	
	/* Check if the vector variable has not been requested before: */
	if(vectorVariables[vectorVariableIndex].vectorExtractor==0)
		vectorVariables[vectorVariableIndex].vectorExtractor=dataSet->getVectorExtractor(vectorVariableIndex);
	
	return vectorVariables[vectorVariableIndex].vectorExtractor;
	}

int VariableManager::getVectorVariable(const VectorExtractor* vectorExtractor) const
	{
	/* Find the vector extractor among the registered vector variables: */
	for(int i=0;i<numVectorVariables;++i)
		if(vectorVariables[i].vectorExtractor==vectorExtractor)
			return i;
	
	return -1;
	}

void VariableManager::showColorBar(bool show)
	{
	/* Hide or show color bar dialog based on parameter: */
	if(show)
		Vrui::popupPrimaryWidget(colorBarDialogPopup);
	else
		Vrui::popdownPrimaryWidget(colorBarDialogPopup);
	}

void VariableManager::showPaletteEditor(bool show)
	{
	/* Hide or show color palette editor dialog based on parameter: */
	if(show)
		Vrui::popupPrimaryWidget(paletteEditor);
	else
		Vrui::popdownPrimaryWidget(paletteEditor);
	}

void VariableManager::createPalette(int newPaletteType)
	{
	/* Define local types: */
	typedef GLMotif::ColorMap ColorMap;
	typedef ColorMap::ValueRange ValueRange;
	typedef ColorMap::ColorMapValue Color;
	typedef ColorMap::ControlPoint ControlPoint;
	
	/* Get the current color map's value range: */
	const ValueRange& valueRange=paletteEditor->getColorMap()->getValueRange();
	double o=valueRange.first;
	double f=valueRange.second-o;
	
	/* Create a new control point vector for the current color map: */
	std::vector<ControlPoint> controlPoints;
	bool createPalette=true;
	switch(newPaletteType)
		{
		case LUMINANCE_GREY:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.0f,0.0f,0.0f,0.0f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(1.0f,1.0f,1.0f,1.0f)));
			break;
		
		case LUMINANCE_RED:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.0f,0.0f,0.0f,0.0f)));
			controlPoints.push_back(ControlPoint(o+f*0.5,Color(1.0f,0.287f,0.287f,0.5f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(1.0f,1.0f,1.0f,1.0f)));
			break;
		
		case LUMINANCE_YELLOW:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.0f,0.0f,0.0f,0.0f)));
			controlPoints.push_back(ControlPoint(o+f*0.5,Color(0.564f,0.564f,0.0f,0.5f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(1.0f,1.0f,1.0f,1.0f)));
			break;
		
		case LUMINANCE_GREEN:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.0f,0.0f,0.0f,0.0f)));
			controlPoints.push_back(ControlPoint(o+f*0.5,Color(0.0f,0.852f,0.0f,0.5f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(1.0f,1.0f,1.0f,1.0f)));
			break;
		
		case LUMINANCE_CYAN:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.0f,0.0f,0.0f,0.0f)));
			controlPoints.push_back(ControlPoint(o+f*0.5,Color(0.0f,0.713f,0.713f,0.5f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(1.0f,1.0f,1.0f,1.0f)));
			break;
		
		case LUMINANCE_BLUE:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.0f,0.0f,0.0f,0.0f)));
			controlPoints.push_back(ControlPoint(o+f*0.5,Color(0.436f,0.436f,1.0f,0.5f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(1.0f,1.0f,1.0f,1.0f)));
			break;
		
		case LUMINANCE_MAGENTA:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.0f,0.0f,0.0f,0.0f)));
			controlPoints.push_back(ControlPoint(o+f*0.5,Color(1.0f,0.148f,1.0f,0.5f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(1.0f,1.0f,1.0f,1.0f)));
			break;
		
		case SATURATION_RED_CYAN:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(1.0f,0.287f,0.287f,0.0f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(0.0f,0.713f,0.713f,1.0f)));
			break;
		
		case SATURATION_YELLOW_BLUE:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.564f,0.564f,0.0f,0.0f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(0.436f,0.436f,1.0f,1.0f)));
			break;
		
		case SATURATION_GREEN_MAGENTA:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.0f,0.852f,0.0f,0.0f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(1.0f,0.148f,1.0f,1.0f)));
			break;
		
		case SATURATION_CYAN_RED:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.0f,0.713f,0.713f,0.0f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(1.0f,0.287f,0.287f,1.0f)));
			break;
		
		case SATURATION_BLUE_YELLOW:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(0.436f,0.436f,1.0f,0.0f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(0.564f,0.564f,0.0f,1.0f)));
			break;
		
		case SATURATION_MAGENTA_GREEN:
			controlPoints.push_back(ControlPoint(valueRange.first,Color(1.0f,0.148f,1.0f,0.0f)));
			controlPoints.push_back(ControlPoint(valueRange.second,Color(0.0f,0.852f,0.0f,1.0f)));
			break;
		
		case RAINBOW:
			{
			double o=valueRange.first;
			double f=valueRange.second-o;
			controlPoints.push_back(ControlPoint(o+f*(0.0/5.0),Color(1.0f,0.287f,0.287f,(0.0f/5.0f))));
			controlPoints.push_back(ControlPoint(o+f*(1.0/5.0),Color(0.564f,0.564f,0.0f,(1.0f/5.0f))));
			controlPoints.push_back(ControlPoint(o+f*(2.0/5.0),Color(0.0f,0.852f,0.0f,(2.0f/5.0f))));
			controlPoints.push_back(ControlPoint(o+f*(3.0/5.0),Color(0.0f,0.713f,0.713f,(3.0f/5.0f))));
			controlPoints.push_back(ControlPoint(o+f*(4.0/5.0),Color(0.436f,0.436f,1.0f,(4.0f/5.0f))));
			controlPoints.push_back(ControlPoint(o+f*(5.0/5.0),Color(1.0f,0.148f,1.0f,(5.0f/5.0f))));
			break;
			}
		
		default:
			createPalette=false;
		}
	
	if(createPalette)
		{
		/* Create the new color map: */
		paletteEditor->createPalette(controlPoints);
		
		Vrui::requestUpdate();
		}
	}

void VariableManager::loadPalette(const char* paletteFileName)
	{
	/* Load the given palette file: */
	paletteEditor->loadPalette(paletteFileName,scalarVariables[currentScalarVariableIndex].valueRange);
	}

void VariableManager::insertPaletteEditorControlPoint(double newControlPoint)
	{
	paletteEditor->getColorMap()->insertControlPoint(newControlPoint);
	}

void VariableManager::bindColorMap(int scalarVariableIndex,SceneGraph::GLRenderState& renderState) const
	{
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	/* Bind the color texture object: */
	renderState.bindTexture1D(dataItem->colorMapTextureIds[scalarVariableIndex]);
	
	/* Check if the texture object is outdated: */
	const ScalarVariable& sv=scalarVariables[scalarVariableIndex];
	if(dataItem->colorMapVersions[scalarVariableIndex]!=sv.colorMapVersion)
		{
		/* Set the texture object's parameters: */
		glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_BASE_LEVEL,0);
		glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAX_LEVEL,0);
		glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		
		/* Upload the changed color map into the texture object: */
		glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA8,256,0,GL_RGBA,GL_FLOAT,sv.colorMap->getColors());
		
		/* Mark the texture object as up-to-date: */
		dataItem->colorMapVersions[scalarVariableIndex]=sv.colorMapVersion;
		}
	
	/* Set up the texture matrix to convert scalar variable values to color map indices: */
	typedef SceneGraph::GLRenderState::TextureTransform TTransform;
	typedef typename TTransform::Scalar TScalar;
	TTransform tt=TTransform::identity;
	TScalar mapMin=TScalar(sv.colorMapRange.first);
	TScalar mapRange=TScalar(sv.colorMapRange.second-sv.colorMapRange.first);
	tt.getMatrix()(0,0)=TScalar(1)/mapRange;
	tt.getMatrix()(0,3)=-mapMin/mapRange;
	renderState.setTextureTransform(tt);
	}

}

}
