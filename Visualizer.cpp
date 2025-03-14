/***********************************************************************
Visualizer - Test application for the new visualization component
framework.
Copyright (c) 2005-2024 Oliver Kreylos

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

#include "Visualizer.h"

#include <ctype.h>
#include <string.h>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <string>
#include <Misc/StdError.h>
#include <Misc/Timer.h>
#include <Misc/StandardMarshallers.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/CreateNumberedFileName.h>
#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/File.h>
#include <IO/Directory.h>
#include <IO/OpenFile.h>
#include <IO/ValueSource.h>
#include <Cluster/MulticastPipe.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Separator.h>
#include <GLMotif/Label.h>
#include <GLMotif/TextField.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <SceneGraph/GLRenderState.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/VRMLFile.h>
#include <Vrui/Vrui.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/SceneGraphManager.h>

#include <Abstract/DataSetRenderer.h>
#include <Abstract/CoordinateTransformer.h>
#include <Abstract/VariableManager.h>
#include <Abstract/Parameters.h>
#include <Abstract/BinaryParametersSink.h>
#include <Abstract/BinaryParametersSource.h>
#include <Abstract/FileParametersSource.h>
#include <Abstract/ConfigurationFileParametersSource.h>
#include <Abstract/Algorithm.h>
#include <Abstract/Element.h>
#include <Abstract/Module.h>

#include "CuttingPlane.h"
#include "BaseLocator.h"
#include "CuttingPlaneLocator.h"
#include "ScalarEvaluationLocator.h"
#include "VectorEvaluationLocator.h"
#include "ExtractorLocator.h"
#include "ElementList.h"
#if VISUALIZATION_CONFIG_USE_COLLABORATION
#include "SharedVisualizationClient.h"
#endif

/***************************
Methods of class Visualizer:
***************************/

GLMotif::PopupMenu* Visualizer::createRenderingModesMenu(void)
	{
	GLMotif::PopupMenu* renderingModesMenuPopup=new GLMotif::PopupMenu("RenderingModesMenuPopup",Vrui::getWidgetManager());
	
	GLMotif::Menu* renderingModesMenu=new GLMotif::Menu("RenderingModesMenu",renderingModesMenuPopup,false);
	
	GLMotif::RadioBox* renderingModes=new GLMotif::RadioBox("RenderingModes",renderingModesMenu,false);
	renderingModes->setSelectionMode(GLMotif::RadioBox::ATMOST_ONE);
	
	int numRenderingModes=dataSetRenderer->getNumRenderingModes();
	for(int i=0;i<numRenderingModes;++i)
		renderingModes->addToggle(dataSetRenderer->getRenderingModeName(i));
	
	if(renderDataSet)
		renderingModes->setSelectedToggle(dataSetRenderer->getRenderingMode());
	renderingModes->getValueChangedCallbacks().add(this,&Visualizer::changeRenderingModeCallback);
	
	renderingModes->manageChild();
	
	if(!sceneGraphs.empty())
		{
		new GLMotif::Separator("SceneGraphsSeparator",renderingModesMenu,GLMotif::Separator::HORIZONTAL,0.0f,GLMotif::Separator::LOWERED);
		
		/* Create a set of toggle buttons to enable/disable individual additional scene graphs: */
		int i=0;
		for(std::vector<SG>::iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt,++i)
			{
			char sgName[40];
			snprintf(sgName,sizeof(sgName),"SceneGraph%d",i+1);
			GLMotif::ToggleButton* sgToggle=new GLMotif::ToggleButton(sgName,renderingModesMenu,sgIt->name.c_str());
			sgToggle->setToggle(sgIt->render);
			sgToggle->getValueChangedCallbacks().add(this,&Visualizer::toggleSceneGraphCallback,i);
			}
		}
	
	renderingModesMenu->manageChild();
	
	return renderingModesMenuPopup;
	}

GLMotif::PopupMenu* Visualizer::createScalarVariablesMenu(void)
	{
	GLMotif::PopupMenu* scalarVariablesMenuPopup=new GLMotif::PopupMenu("ScalarVariablesMenuPopup",Vrui::getWidgetManager());
	
	GLMotif::Menu* scalarVariablesMenu=new GLMotif::Menu("ScalarVariablesMenu",scalarVariablesMenuPopup,false);
	
	GLMotif::RadioBox* scalarVariables=new GLMotif::RadioBox("ScalarVariables",scalarVariablesMenu,false);
	scalarVariables->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	for(int i=0;i<variableManager->getNumScalarVariables();++i)
		scalarVariables->addToggle(variableManager->getScalarVariableName(i));
	
	scalarVariables->setSelectedToggle(variableManager->getCurrentScalarVariable());
	scalarVariables->getValueChangedCallbacks().add(this,&Visualizer::changeScalarVariableCallback);
	
	scalarVariables->manageChild();
	
	scalarVariablesMenu->manageChild();
	
	return scalarVariablesMenuPopup;
	}

GLMotif::PopupMenu* Visualizer::createVectorVariablesMenu(void)
	{
	GLMotif::PopupMenu* vectorVariablesMenuPopup=new GLMotif::PopupMenu("VectorVariablesMenuPopup",Vrui::getWidgetManager());
	
	GLMotif::Menu* vectorVariablesMenu=new GLMotif::Menu("VectorVariablesMenu",vectorVariablesMenuPopup,false);
	
	GLMotif::RadioBox* vectorVariables=new GLMotif::RadioBox("VectorVariables",vectorVariablesMenu,false);
	vectorVariables->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	for(int i=0;i<variableManager->getNumVectorVariables();++i)
		vectorVariables->addToggle(variableManager->getVectorVariableName(i));
	
	vectorVariables->setSelectedToggle(variableManager->getCurrentVectorVariable());
	vectorVariables->getValueChangedCallbacks().add(this,&Visualizer::changeVectorVariableCallback);
	
	vectorVariables->manageChild();
	
	vectorVariablesMenu->manageChild();
	
	return vectorVariablesMenuPopup;
	}

GLMotif::PopupMenu* Visualizer::createAlgorithmsMenu(void)
	{
	GLMotif::PopupMenu* algorithmsMenuPopup=new GLMotif::PopupMenu("AlgorithmsMenuPopup",Vrui::getWidgetManager());
	
	GLMotif::Menu* algorithmsMenu=new GLMotif::Menu("AlgorithmsMenu",algorithmsMenuPopup,false);
	
	GLMotif::RadioBox* algorithms=new GLMotif::RadioBox("Algorithms",algorithmsMenu,false);
	algorithms->setSelectionMode(GLMotif::RadioBox::ALWAYS_ONE);
	
	/* Add the cutting plane algorithm: */
	int algorithmIndex=0;
	algorithms->addToggle("Cutting Plane");
	++algorithmIndex;
	
	if(variableManager->getNumScalarVariables()>0)
		{
		/* Add the scalar evaluator algorithm: */
		algorithms->addToggle("Evaluate Scalars");
		++algorithmIndex;
		
		/* Add scalar algorithms: */
		firstScalarAlgorithmIndex=algorithmIndex;
		for(int i=0;i<module->getNumScalarAlgorithms();++i)
			{
			algorithms->addToggle(module->getScalarAlgorithmName(i));
			++algorithmIndex;
			}
		}
	
	if(variableManager->getNumVectorVariables()>0)
		{
		/* Add the vector evaluator algorithm: */
		algorithms->addToggle("Evaluate Vectors");
		++algorithmIndex;
		
		/* Add vector algorithms: */
		firstVectorAlgorithmIndex=algorithmIndex;
		for(int i=0;i<module->getNumVectorAlgorithms();++i)
			{
			algorithms->addToggle(module->getVectorAlgorithmName(i));
			++algorithmIndex;
			}
		}
		
	algorithms->setSelectedToggle(algorithm);
	algorithms->getValueChangedCallbacks().add(this,&Visualizer::changeAlgorithmCallback);
	
	algorithms->manageChild();
	
	algorithmsMenu->manageChild();
	
	return algorithmsMenuPopup;
	}

GLMotif::PopupMenu* Visualizer::createElementsMenu(void)
	{
	GLMotif::PopupMenu* elementsMenuPopup=new GLMotif::PopupMenu("ElementsMenuPopup",Vrui::getWidgetManager());
	
	/* Create the elements menu: */
	GLMotif::Menu* elementsMenu=new GLMotif::Menu("ElementsMenu",elementsMenuPopup,false);
	
	showElementListToggle=new GLMotif::ToggleButton("ShowElementListToggle",elementsMenu,"Show Element List");
	showElementListToggle->getValueChangedCallbacks().add(this,&Visualizer::showElementListCallback);
	
	GLMotif::Button* loadElementsButton=new GLMotif::Button("LoadElementsButton",elementsMenu,"Load Visualization Elements");
	loadElementsButton->getSelectCallbacks().add(this,&Visualizer::loadElementsCallback);
	
	GLMotif::Button* saveElementsButton=new GLMotif::Button("SaveElementsButton",elementsMenu,"Save Visualization Elements");
	saveElementsButton->getSelectCallbacks().add(this,&Visualizer::saveElementsCallback);
	
	new GLMotif::Separator("ClearElementsSeparator",elementsMenu,GLMotif::Separator::HORIZONTAL,0.0f,GLMotif::Separator::LOWERED);
	
	GLMotif::Button* clearElementsButton=new GLMotif::Button("ClearElementsButton",elementsMenu,"Clear Visualization Elements");
	clearElementsButton->getSelectCallbacks().add(this,&Visualizer::clearElementsCallback);
	
	elementsMenu->manageChild();
	
	return elementsMenuPopup;
	}

GLMotif::PopupMenu* Visualizer::createStandardLuminancePalettesMenu(void)
	{
	GLMotif::PopupMenu* standardLuminancePalettesMenuPopup=new GLMotif::PopupMenu("StandardLuminancePalettesMenuPopup",Vrui::getWidgetManager());
	
	/* Create the palette creation menu and add entries for all standard palettes: */
	GLMotif::Menu* standardLuminancePalettes=new GLMotif::Menu("StandardLuminancePalettes",standardLuminancePalettesMenuPopup,false);
	
	standardLuminancePalettes->addEntry("Grey");
	standardLuminancePalettes->addEntry("Red");
	standardLuminancePalettes->addEntry("Yellow");
	standardLuminancePalettes->addEntry("Green");
	standardLuminancePalettes->addEntry("Cyan");
	standardLuminancePalettes->addEntry("Blue");
	standardLuminancePalettes->addEntry("Magenta");
	
	standardLuminancePalettes->getEntrySelectCallbacks().add(this,&Visualizer::createStandardLuminancePaletteCallback);
	
	standardLuminancePalettes->manageChild();
	
	return standardLuminancePalettesMenuPopup;
	}

GLMotif::PopupMenu* Visualizer::createStandardSaturationPalettesMenu(void)
	{
	GLMotif::PopupMenu* standardSaturationPalettesMenuPopup=new GLMotif::PopupMenu("StandardSaturationPalettesMenuPopup",Vrui::getWidgetManager());
	
	/* Create the palette creation menu and add entries for all standard palettes: */
	GLMotif::Menu* standardSaturationPalettes=new GLMotif::Menu("StandardSaturationPalettes",standardSaturationPalettesMenuPopup,false);
	
	standardSaturationPalettes->addEntry("Red -> Cyan");
	standardSaturationPalettes->addEntry("Yellow -> Blue");
	standardSaturationPalettes->addEntry("Green -> Magenta");
	standardSaturationPalettes->addEntry("Cyan -> Red");
	standardSaturationPalettes->addEntry("Blue -> Yellow");
	standardSaturationPalettes->addEntry("Magenta -> Green");
	standardSaturationPalettes->addEntry("Rainbow");
	
	standardSaturationPalettes->getEntrySelectCallbacks().add(this,&Visualizer::createStandardSaturationPaletteCallback);
	
	standardSaturationPalettes->manageChild();
	
	return standardSaturationPalettesMenuPopup;
	}

GLMotif::PopupMenu* Visualizer::createColorMenu(void)
	{
	GLMotif::PopupMenu* colorMenuPopup=new GLMotif::PopupMenu("ColorMenuPopup",Vrui::getWidgetManager());
	
	/* Create the color menu and add entries for all standard palettes: */
	GLMotif::Menu* colorMenu=new GLMotif::Menu("ColorMenu",colorMenuPopup,false);
	
	GLMotif::CascadeButton* standardLuminancePalettesCascade=new GLMotif::CascadeButton("StandardLuminancePalettesCascade",colorMenu,"Create Luminance Palette");
	standardLuminancePalettesCascade->setPopup(createStandardLuminancePalettesMenu());
	
	GLMotif::CascadeButton* standardSaturationPalettesCascade=new GLMotif::CascadeButton("StandardSaturationPalettesCascade",colorMenu,"Create Saturation Palette");
	standardSaturationPalettesCascade->setPopup(createStandardSaturationPalettesMenu());
	
	GLMotif::Button* loadPaletteButton=new GLMotif::Button("LoadPaletteButton",colorMenu,"Load Palette File");
	loadPaletteButton->getSelectCallbacks().add(this,&Visualizer::loadPaletteCallback);
	
	showColorBarToggle=new GLMotif::ToggleButton("ShowColorBarToggle",colorMenu,"Show Color Bar");
	showColorBarToggle->getValueChangedCallbacks().add(this,&Visualizer::showColorBarCallback);
	
	showPaletteEditorToggle=new GLMotif::ToggleButton("ShowPaletteEditorToggle",colorMenu,"Show Palette Editor");
	showPaletteEditorToggle->getValueChangedCallbacks().add(this,&Visualizer::showPaletteEditorCallback);
	
	colorMenu->manageChild();
	
	return colorMenuPopup;
	}

GLMotif::PopupMenu* Visualizer::createMainMenu(void)
	{
	GLMotif::PopupMenu* mainMenuPopup=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
	mainMenuPopup->setTitle("3D Visualizer");
	
	GLMotif::Menu* mainMenu=new GLMotif::Menu("MainMenu",mainMenuPopup,false);
	
	GLMotif::CascadeButton* renderingModesCascade=new GLMotif::CascadeButton("RenderingModesCascade",mainMenu,"Rendering Modes");
	renderingModesCascade->setPopup(createRenderingModesMenu());
	
	if(variableManager->getNumScalarVariables()>0)
		{
		GLMotif::CascadeButton* scalarVariablesCascade=new GLMotif::CascadeButton("ScalarVariablesCascade",mainMenu,"Scalar Variables");
		scalarVariablesCascade->setPopup(createScalarVariablesMenu());
		}
	
	if(variableManager->getNumVectorVariables()>0)
		{
		GLMotif::CascadeButton* vectorVariablesCascade=new GLMotif::CascadeButton("VectorVariablesCascade",mainMenu,"Vector Variables");
		vectorVariablesCascade->setPopup(createVectorVariablesMenu());
		}
	
	GLMotif::CascadeButton* algorithmsCascade=new GLMotif::CascadeButton("AlgorithmsCascade",mainMenu,"Algorithms");
	algorithmsCascade->setPopup(createAlgorithmsMenu());
	
	GLMotif::CascadeButton* elementsCascade=new GLMotif::CascadeButton("ElementsCascade",mainMenu,"Elements");
	elementsCascade->setPopup(createElementsMenu());
	
	GLMotif::CascadeButton* colorCascade=new GLMotif::CascadeButton("ColorCascade",mainMenu,"Color Maps");
	colorCascade->setPopup(createColorMenu());
	
	mainMenu->manageChild();
	
	return mainMenuPopup;
	}

void Visualizer::loadElements(const char* elementFileName,bool ascii)
	{
	/* Open a pipe for cluster communication: */
	Cluster::MulticastPipe* pipe=Vrui::openPipe();
	
	if(pipe==0||pipe->isMaster())
		{
		/* Create a data sink to send element parameters to the cluster: */
		Visualization::Abstract::BinaryParametersSink sink(variableManager,*pipe,true);
		
		if(ascii)
			{
			/* Open the element file: */
			IO::ValueSource elementFile(IO::openFile(elementFileName));
			elementFile.setPunctuation("");
			elementFile.setQuotes("\"");
			elementFile.skipWs();
			
			/* Read all elements from the file: */
			while(!elementFile.eof())
				{
				/* Read the next algorithm name: */
				std::string algorithmName=elementFile.readLine();
				elementFile.skipWs();
				
				if(pipe!=0)
					{
					/* Send the algorithm name to the cluster: */
					Misc::Marshaller<std::string>::write(algorithmName,*pipe);
					pipe->flush(); // Redundant!!!
					}
				
				/* Create an extractor for the given name: */
				Cluster::MulticastPipe* algorithmPipe=Vrui::openPipe();
				Algorithm* algorithm=module->getAlgorithm(algorithmName.c_str(),variableManager,algorithmPipe);
				
				/* Extract an element using the given extractor: */
				if(algorithm!=0)
					{
					std::cout<<"Creating "<<algorithmName<<"..."<<std::flush;
					Misc::Timer extractionTimer;
					
					try
						{
						/* Read the element's extraction parameters from the file: */
						Visualization::Abstract::FileParametersSource source(variableManager,elementFile);
						Parameters* parameters=algorithm->cloneParameters();
						parameters->read(source);
						
						if(pipe!=0)
							{
							/* Send the extraction parameters to the cluster: */
							pipe->write<int>(1);
							parameters->write(sink);
							pipe->flush();
							}
						
						/* Extract the element: */
						Element* element=algorithm->createElement(parameters);
						
						/* Store the element: */
						elementList->addElement(algorithm,element);
						}
					catch(const std::runtime_error& err)
						{
						if(pipe!=0)
							{
							/* Tell the cluster there was a problem: */
							pipe->write<int>(0);
							pipe->flush();
							}
						
						std::cout<<"Cancelled due to exception "<<err.what()<<"...";
						}
					
					/* Destroy the extractor: */
					delete algorithm;
					
					extractionTimer.elapse();
					std::cout<<" done in "<<extractionTimer.getTime()*1000.0<<" ms"<<std::endl;
					}
				else
					{
					std::cout<<"Ignoring unknown algorithm "<<algorithmName<<std::endl;
					delete algorithmPipe;
					}
				}
			}
		else
			{
			/* Open the element file and create a data source to read from it: */
			IO::FilePtr elementFile(IO::openFile(elementFileName));
			elementFile->setEndianness(Misc::LittleEndian);
			Visualization::Abstract::BinaryParametersSource source(variableManager,*elementFile,false);
			
			/* Read all elements from the file: */
			while(!elementFile->eof())
				{
				/* Read the next algorithm name: */
				std::string algorithmName=Misc::Marshaller<std::string>::read(*elementFile);
				
				if(pipe!=0)
					{
					/* Send the algorithm name to the cluster: */
					Misc::Marshaller<std::string>::write(algorithmName,*pipe);
					}
				
				/* Create an extractor for the given name: */
				Cluster::MulticastPipe* algorithmPipe=Vrui::openPipe();
				Algorithm* algorithm=module->getAlgorithm(algorithmName.c_str(),variableManager,algorithmPipe);
				
				/* Extract an element using the given extractor: */
				if(algorithm!=0)
					{
					std::cout<<"Creating "<<algorithmName<<"..."<<std::flush;
					Misc::Timer extractionTimer;
					
					try
						{
						/* Read the element's extraction parameters from the file: */
						Parameters* parameters=algorithm->cloneParameters();
						parameters->read(source);
						
						if(pipe!=0)
							{
							/* Send the extraction parameters to the cluster: */
							pipe->write<int>(1);
							parameters->write(sink);
							pipe->flush();
							}
						
						/* Extract the element: */
						Element* element=algorithm->createElement(parameters);
						
						/* Store the element: */
						elementList->addElement(algorithm,element);
						}
					catch(const std::runtime_error& err)
						{
						if(pipe!=0)
							{
							/* Tell the cluster there was a problem: */
							pipe->write<int>(0);
							pipe->flush();
							}
						
						std::cout<<"Cancelled due to exception "<<err.what()<<"...";
						}
					
					/* Destroy the extractor: */
					delete algorithm;
					
					extractionTimer.elapse();
					std::cout<<" done in "<<extractionTimer.getTime()*1000.0<<" ms"<<std::endl;
					}
				else
					{
					std::cout<<"Ignoring unknown algorithm "<<algorithmName<<std::endl;
					delete algorithmPipe;
					}
				}
			}
		
		if(pipe!=0)
			{
			/* Send an empty algorithm name to signal end-of-file to the cluster: */
			Misc::Marshaller<std::string>::write("",*pipe);
			pipe->flush();
			}
		}
	else
		{
		std::cout<<"Ready to receive elements"<<std::endl;

		/* Create a data source to read elements' parameters: */
		Visualization::Abstract::BinaryParametersSource source(variableManager,*pipe,true);
		
		/* Receive all visualization elements from the head node: */
		while(true)
			{
			/* Receive the algorithm name from the head node: */
			std::cout<<"Reading algorithm name"<<std::endl;
			std::string algorithmName=Misc::Marshaller<std::string>::read(*pipe);
			if(algorithmName.empty()) // Check for end-of-file indicator
				break;
			
			// DEBUGGING
			std::cout<<"Received algorithm "<<algorithmName<<std::endl;
			
			/* Create an extractor for the given name: */
			Cluster::MulticastPipe* algorithmPipe=Vrui::openPipe();
			Algorithm* algorithm=module->getAlgorithm(algorithmName.c_str(),variableManager,algorithmPipe);
			
			/* Extract an element using the given extractor: */
			if(algorithm!=0)
				{
				/* Check if there are valid parameters: */
				if(pipe->read<int>()!=0)
					{
					std::cout<<"Receiving parameters"<<std::endl;
					
					/* Receive the extraction parameters: */
					Parameters* parameters=algorithm->cloneParameters();
					parameters->read(source);
					
					std::cout<<"Receiving element"<<std::endl;
					/* Receive the element: */
					Element* element=algorithm->startSlaveElement(parameters);
					algorithm->continueSlaveElement();
					
					std::cout<<"Done"<<std::endl;

					/* Store the element: */
					elementList->addElement(algorithm,element);
					}
				
				/* Destroy the extractor: */
				delete algorithm;
				}
			else
				delete algorithmPipe;
			}

		std::cout<<"Done"<<std::endl;
		}
	
	if(pipe!=0)
		{
		/* Close the communication pipe: */
		delete pipe;
		}
	}

Visualizer::Visualizer(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 moduleManager(VISUALIZATION_CONFIG_MODULENAMETEMPLATE),
	 module(0),dataSet(0),variableManager(0),
	 renderDataSet(true),dataSetRenderer(0),
	 sceneGraphRoot(new SceneGraph::GroupNode),renderSceneGraphs(false),
	 coordinateTransformer(0),
	 firstScalarAlgorithmIndex(0),firstVectorAlgorithmIndex(0),
	 #if VISUALIZATION_CONFIG_USE_COLLABORATION
	 sharedVisualizationClient(0),
	 #endif
	 numCuttingPlanes(0),cuttingPlanes(0),
	 elementList(0),
	 algorithm(0),
	 mainMenu(0),
	 inLoadPalette(false),inLoadElements(false)
	{
	/* Parse the command line: */
	IO::DirectoryPtr baseDirectory=IO::Directory::getCurrent();
	std::string moduleClassName;
	std::vector<std::string> dataSetArgs;
	const char* argColorMapName=0;
	std::vector<const char*> loadFileNames;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"class")==0)
				{
				/* Get visualization module class name and data set arguments from command line: */
				++i;
				if(i>=argc)
					throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Missing module class name after -class");
				moduleClassName=argv[i];
				++i;
				while(i<argc&&strcmp(argv[i],";")!=0)
					{
					dataSetArgs.push_back(argv[i]);
					++i;
					}
				}
			else if(strcasecmp(argv[i]+1,"palette")==0)
				{
				++i;
				if(i<argc)
					argColorMapName=argv[i];
				else
					std::cerr<<"Missing palette file name after -palette"<<std::endl;
				}
			else if(strcasecmp(argv[i]+1,"load")==0)
				{
				++i;
				if(i<argc)
					{
					/* Load an element file later: */
					loadFileNames.push_back(argv[i]);
					}
				else
					std::cerr<<"Missing element file name after -load"<<std::endl;
				}
			else if(strcasecmp(argv[i]+1,"sceneGraph")==0)
				{
				++i;
				if(i<argc)
					{
					try
						{
						/* Load the scene graph: */
						SG sg;
						sg.root=Vrui::getSceneGraphManager()->loadSceneGraph(argv[i]);
						
						/* Store the scene graph's name: */
						char* nameStart=argv[i];
						char* nameEnd=0;
						for(char* nPtr=argv[i];*nPtr!='\0';++nPtr)
							{
							if(*nPtr=='/')
								{
								nameStart=nPtr+1;
								nameEnd=0;
								}
							if(*nPtr=='.')
								nameEnd=nPtr;
							}
						sg.name=nameEnd!=0?std::string(nameStart,nameEnd):std::string(nameStart);
						
						/* Store the scene graph in the list and add it to the main scene graph: */
						sg.render=true;
						sceneGraphs.push_back(sg);
						renderSceneGraphs=true;
						sceneGraphRoot->addChild(*sg.root);
						}
					catch(const std::runtime_error& err)
						{
						std::cerr<<"Ignoring scene graph "<<argv[i]<<" due to exception "<<err.what()<<std::endl;
						}
					}
				else
					std::cerr<<"Missing scene graph file name after -sceneGraph"<<std::endl;
				}
			}
		else
			{
			/* Set the base directory to the directory containing the meta-input file: */
			baseDirectory=baseDirectory->openFileDirectory(argv[i]);
			
			/* Read the meta-input file of the given name: */
			IO::ValueSource metaInputFile(IO::openFile(argv[i]));
			metaInputFile.setPunctuation("#");
			metaInputFile.skipWs();
			
			/* Read the module class name while skipping any comments: */
			while((moduleClassName=metaInputFile.readString())=="#")
				{
				/* Skip the rest of the line: */
				metaInputFile.skipLine();
				metaInputFile.skipWs();
				}
			
			/* Read the data set arguments: */
			dataSetArgs.clear();
			while(!metaInputFile.eof())
				{
				/* Read the next module argument: */
				std::string argument=metaInputFile.readString();
				
				/* Check for comments: */
				if(argument=="#")
					{
					/* Skip the rest of the line: */
					metaInputFile.skipLine();
					metaInputFile.skipWs();
					}
				else
					{
					/* Store the argument: */
					dataSetArgs.push_back(argument);
					}
				}
			}
		}
	
	/* Add the main scene graph to Vrui's main scene graph: */
	Vrui::getSceneGraphManager()->addNavigationalNode(*sceneGraphRoot);
	
	/* Check if a module class name and data set arguments were provided: */
	if(moduleClassName.empty())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No visualization module class name provided");
	if(dataSetArgs.empty())
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No data set arguments provided");
	
	/* Load a visualization module and a data set: */
	try
		{
		/* Load the appropriate visualization module: */
		module=moduleManager.loadClass(moduleClassName.c_str());
		module->setBaseDirectory(baseDirectory);
		
		/* Load a data set: */
		Misc::Timer t;
		Cluster::MulticastPipe* pipe=Vrui::openPipe(); // Implicit synchronization point
		dataSet=module->load(dataSetArgs,pipe);
		delete pipe; // Implicit synchronization point
		t.elapse();
		if(Vrui::isHeadNode())
			std::cout<<"Time to load data set: "<<t.getTime()*1000.0<<" ms"<<std::endl;
		}
	catch(const std::runtime_error& err)
		{
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Could not load data set due to exception %s",err.what());
		}
	
	/* Create a variable manager: */
	variableManager=new VariableManager(dataSet,argColorMapName);
	variableManager->getColorBarDialog()->setCloseButton(true);
	variableManager->getColorBarDialog()->getCloseCallbacks().add(this,&Visualizer::colorBarClosedCallback);
	variableManager->getPaletteEditor()->setCloseButton(true);
	variableManager->getPaletteEditor()->getCloseCallbacks().add(this,&Visualizer::paletteEditorClosedCallback);
	
	/* Create a data set renderer and add it to Vrui's main scene graph: */
	dataSetRenderer=module->getRenderer(dataSet);
	dataSetRenderer->setGridLineWidth(1.0f);
	dataSetRenderer->setGridOpacity(0.15f);
	Vrui::getSceneGraphManager()->addNavigationalNode(*dataSetRenderer);
	
	/* Get the data set's coordinate transformer: */
	coordinateTransformer=dataSet->getCoordinateTransformer();
	
	/* Set Vrui's application unit: */
	if(dataSet->getUnit().unit!=Geometry::LinearUnit::UNKNOWN)
		Vrui::getCoordinateManager()->setUnit(dataSet->getUnit());
	
	/* Create cutting planes: */
	numCuttingPlanes=6;
	cuttingPlanes=new CuttingPlane[numCuttingPlanes];
	for(size_t i=0;i<numCuttingPlanes;++i)
		{
		cuttingPlanes[i].allocated=false;
		cuttingPlanes[i].active=false;
		}
	
	/* Create the main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Create the element list: */
	elementList=new ElementList(Vrui::getWidgetManager());
	elementList->getElementListDialog()->setCloseButton(true);
	elementList->getElementListDialog()->getCloseCallbacks().add(this,&Visualizer::elementListClosedCallback);
	
	#if VISUALIZATION_CONFIG_USE_COLLABORATION
	
	/* Check whether to connect to a shared visualization session: */
	Collab::Client* client=Collab::Client::getTheClient();
	if(client!=0)
		{
		/* Register the shared visualization protoocol: */
		sharedVisualizationClient=new Collab::Plugins::SharedVisualizationClient(client,variableManager,module,elementList);
		client->addPluginProtocol(sharedVisualizationClient);
		
		/* Let the element list know of the shared visualization client: */
		elementList->setSharedVisualizationClient(sharedVisualizationClient);
		}
	
	#endif
	
	/* Load all element files listed on the command line: */
	for(std::vector<const char*>::const_iterator lfnIt=loadFileNames.begin();lfnIt!=loadFileNames.end();++lfnIt)
		{
		/* Determine the type of the element file: */
		if(Misc::hasCaseExtension(*lfnIt,".asciielem"))
			{
			/* Load an ASCII elements file: */
			loadElements(*lfnIt,true);
			}
		else if(Misc::hasCaseExtension(*lfnIt,".binelem"))
			{
			/* Load a binary elements file: */
			loadElements(*lfnIt,false);
			}
		}
	}

Visualizer::~Visualizer(void)
	{
	delete mainMenu;
	
	/* Delete all finished visualization elements: */
	delete elementList;
	
	/* Remove all remaining locators from Vrui's central scene graph: */
	for(BaseLocatorList::iterator blIt=baseLocators.begin();blIt!=baseLocators.end();++blIt)
		Vrui::getSceneGraphManager()->removeNavigationalNode(**blIt);
	
	/* Delete the cutting planes: */
	delete[] cuttingPlanes;
	
	#if VISUALIZATION_CONFIG_USE_COLLABORATION
	#endif
	
	/* Delete the coordinate transformer: */
	delete coordinateTransformer;
	
	/* Delete the variable manager: */
	delete variableManager;
	
	/* Delete the data set: */
	delete dataSet;
	}

void Visualizer::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
	{
	/* Check if the new tool is a locator tool: */
	Vrui::LocatorTool* locatorTool=dynamic_cast<Vrui::LocatorTool*>(cbData->tool);
	if(locatorTool!=0)
		{
		BaseLocator* newLocator;
		if(cbData->cfg!=0)
			{
			/* Determine the algorithm type from the configuration file section: */
			std::string algorithmName=cbData->cfg->retrieveString("./algorithm");
			if(algorithmName=="Cutting Plane")
				{
				/* Create a cutting plane locator object and associate it with the new tool: */
				newLocator=new CuttingPlaneLocator(locatorTool,this,cbData->cfg);
				}
			else if(algorithmName=="Evaluate Scalars")
				{
				/* Create a scalar evaluation locator object and associate it with the new tool: */
				newLocator=new ScalarEvaluationLocator(locatorTool,this,cbData->cfg);
				}
			else if(algorithmName=="Evaluate Vectors")
				{
				/* Create a vector evaluation locator object and associate it with the new tool: */
				newLocator=new VectorEvaluationLocator(locatorTool,this,cbData->cfg);
				}
			else
				{
				/* Create an extractor locator: */
				Cluster::MulticastPipe* algorithmPipe=Vrui::openPipe();
				Algorithm* extractor=module->getAlgorithm(algorithmName.c_str(),variableManager,algorithmPipe);
				if(extractor!=0)
					{
					if(cbData->cfg!=0)
						{
						/* Read the extractor's parameters from the configuration file section: */
						Visualization::Abstract::ConfigurationFileParametersSource source(variableManager,*cbData->cfg);
						extractor->readParameters(source);
						}
					
					newLocator=new ExtractorLocator(locatorTool,this,extractor,cbData->cfg);
					}
				else
					{
					newLocator=0;
					delete algorithmPipe;
					}
				}
			}
		else
			{
			if(algorithm==0)
				{
				/* Create a cutting plane locator object and associate it with the new tool: */
				newLocator=new CuttingPlaneLocator(locatorTool,this);
				}
			else if(algorithm<firstScalarAlgorithmIndex)
				{
				/* Create a scalar evaluation locator object and associate it with the new tool: */
				newLocator=new ScalarEvaluationLocator(locatorTool,this);
				}
			else if(algorithm<firstScalarAlgorithmIndex+module->getNumScalarAlgorithms())
				{
				/* Create a data locator object and associate it with the new tool: */
				int algorithmIndex=algorithm-firstScalarAlgorithmIndex;
				Algorithm* extractor=module->getScalarAlgorithm(algorithmIndex,variableManager,Vrui::openPipe());
				newLocator=new ExtractorLocator(locatorTool,this,extractor);
				}
			else if(algorithm<firstVectorAlgorithmIndex)
				{
				/* Create a vector evaluation locator object and associate it with the new tool: */
				newLocator=new VectorEvaluationLocator(locatorTool,this);
				}
			else
				{
				/* Create a data locator object and associate it with the new tool: */
				int algorithmIndex=algorithm-firstVectorAlgorithmIndex;
				Algorithm* extractor=module->getVectorAlgorithm(algorithmIndex,variableManager,Vrui::openPipe());
				newLocator=new ExtractorLocator(locatorTool,this,extractor);
				}
			}
		
		if(newLocator!=0)
			{
			/* Add the new locator to the list and to Vrui's central scene graph: */
			baseLocators.push_back(newLocator);
			Vrui::getSceneGraphManager()->addNavigationalNode(*newLocator);
			}
		}
	}

void Visualizer::toolDestructionCallback(Vrui::ToolManager::ToolDestructionCallbackData* cbData)
	{
	/* Check if the to-be-destroyed tool is a locator tool: */
	Vrui::LocatorTool* locatorTool=dynamic_cast<Vrui::LocatorTool*>(cbData->tool);
	if(locatorTool!=0)
		{
		/* Find the data locator associated with the tool in the list: */
		for(BaseLocatorList::iterator blIt=baseLocators.begin();blIt!=baseLocators.end();++blIt)
			if((*blIt)->getTool()==locatorTool)
				{
				/* Remove the locator from Vrui's central scene graph and the list, which will delete it: */
				Vrui::getSceneGraphManager()->removeNavigationalNode(**blIt);
				baseLocators.erase(blIt);
				break;
				}
		}
	}

void Visualizer::prepareMainLoop(void)
	{
	#if VISUALIZATION_CONFIG_USE_COLLABORATION
	if(sharedVisualizationClient!=0)
		{
		/* Wait until the connection to the shared visualization server is established and check for success: */
		if(!sharedVisualizationClient->waitForConnection())
			{
			/* Notify the user and close the connection: */
			Misc::sourcedUserError(__PRETTY_FUNCTION__,"Connection rejected by shared visualization server");
			sharedVisualizationClient=0;
			
			/* Let the element list know that the connection failed: */
			elementList->setSharedVisualizationClient(0);
			}
		}
	#endif
	}

void Visualizer::frame(void)
	{
	}

void Visualizer::display(GLContextData& contextData) const
	{
	#if 0 // LOL, we're not doing this anymore!
	
	/* Enable all cutting planes: */
	int numSupportedCuttingPlanes;
	glGetIntegerv(GL_MAX_CLIP_PLANES,&numSupportedCuttingPlanes);
	int cuttingPlaneIndex=0;
	for(size_t i=0;i<numCuttingPlanes&&cuttingPlaneIndex<numSupportedCuttingPlanes;++i)
		if(cuttingPlanes[i].active)
			{
			/* Enable the cutting plane: */
			glEnable(GL_CLIP_PLANE0+cuttingPlaneIndex);
			GLdouble cuttingPlane[4];
			for(int j=0;j<3;++j)
				cuttingPlane[j]=cuttingPlanes[i].plane.getNormal()[j];
			cuttingPlane[3]=-cuttingPlanes[i].plane.getOffset();
			glClipPlane(GL_CLIP_PLANE0+cuttingPlaneIndex,cuttingPlane);
			
			/* Go to the next cutting plane: */
			++cuttingPlaneIndex;
			}
	
	/* Disable all cutting planes: */
	cuttingPlaneIndex=0;
	for(size_t i=0;i<numCuttingPlanes&&cuttingPlaneIndex<numSupportedCuttingPlanes;++i)
		if(cuttingPlanes[i].active)
			{
			/* Disable the cutting plane: */
			glDisable(GL_CLIP_PLANE0+cuttingPlaneIndex);
			
			/* Go to the next cutting plane: */
			++cuttingPlaneIndex;
			}
	
	#endif
	}

void Visualizer::resetNavigation(void)
	{
	/* Get the data set's domain box: */
	DataSet::Box domain=dataSet->getDomainBox();
	Vrui::Point center=Geometry::mid(domain.min,domain.max);
	Vrui::Scalar radius=Geometry::dist(domain.min,domain.max);
	
	Vrui::setNavigationTransformation(center,radius);
	}

void Visualizer::changeRenderingModeCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	if(cbData->newSelectedToggle!=0)
		{
		/* Enable data set rendering and set the new rendering mode: */
		if(!renderDataSet)
			{
			/* Add the data set renderer to Vrui's main scene graph: */
			Vrui::getSceneGraphManager()->addNavigationalNode(*dataSetRenderer);
			}
		renderDataSet=true;
		dataSetRenderer->setRenderingMode(cbData->radioBox->getToggleIndex(cbData->newSelectedToggle));
		}
	else
		{
		/* Disable data set rendering: */
		if(renderDataSet)
			{
			/* Remove the data set renderer from Vrui's main scene graph: */
			Vrui::getSceneGraphManager()->removeNavigationalNode(*dataSetRenderer);
			}
		renderDataSet=false;
		}
	}

void Visualizer::toggleSceneGraphCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& sceneGraphIndex)
	{
	/* Disable/enable the affected scene graph: */
	sceneGraphs[sceneGraphIndex].render=cbData->set;
	
	/* Add or remove the scene graph from the main scene graph: */
	if(sceneGraphs[sceneGraphIndex].render)
		sceneGraphRoot->removeChild(*sceneGraphs[sceneGraphIndex].root);
	else
		sceneGraphRoot->addChild(*sceneGraphs[sceneGraphIndex].root);
	}

void Visualizer::changeScalarVariableCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	if(!inLoadPalette)
		{
		/* Set the new scalar variable: */
		variableManager->setCurrentScalarVariable(cbData->radioBox->getToggleIndex(cbData->newSelectedToggle));
		}
	}

void Visualizer::changeVectorVariableCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	/* Set the new vector variable: */
	variableManager->setCurrentVectorVariable(cbData->radioBox->getToggleIndex(cbData->newSelectedToggle));
	}

void Visualizer::changeAlgorithmCallback(GLMotif::RadioBox::ValueChangedCallbackData* cbData)
	{
	/* Set the new algorithm: */
	algorithm=cbData->radioBox->getToggleIndex(cbData->newSelectedToggle);
	}

void Visualizer::loadPaletteCallback(Misc::CallbackData*)
	{
	if(!inLoadPalette)
		{
		/* Create a file selection dialog to select a palette file: */
		GLMotif::FileSelectionDialog* fsDialog=new GLMotif::FileSelectionDialog(Vrui::getWidgetManager(),"Load Palette File...",IO::openDirectory("."),".pal");
		fsDialog->getOKCallbacks().add(this,&Visualizer::loadPaletteOKCallback);
		fsDialog->getCancelCallbacks().add(this,&Visualizer::loadPaletteCancelCallback);
		Vrui::popupPrimaryWidget(fsDialog);
		inLoadPalette=true;
		}
	}

void Visualizer::loadPaletteOKCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	try
		{
		/* Load the palette file: */
		variableManager->loadPalette(cbData->selectedDirectory->getPath(cbData->selectedFileName).c_str());
		}
	catch(const std::runtime_error&)
		{
		/* Ignore the error... */
		}
	
	/* Destroy the file selection dialog: */
	cbData->fileSelectionDialog->close();
	inLoadPalette=false;
	}

void Visualizer::loadPaletteCancelCallback(GLMotif::FileSelectionDialog::CancelCallbackData* cbData)
	{
	/* Destroy the file selection dialog: */
	Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
	inLoadPalette=false;
	}

void Visualizer::showColorBarCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Hide or show color bar dialog based on toggle button state: */
	variableManager->showColorBar(cbData->set);
	}

void Visualizer::colorBarClosedCallback(Misc::CallbackData* cbData)
	{
	showColorBarToggle->setToggle(false);
	}

void Visualizer::showPaletteEditorCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Hide or show palette editor based on toggle button state: */
	variableManager->showPaletteEditor(cbData->set);
	}

void Visualizer::paletteEditorClosedCallback(Misc::CallbackData* cbData)
	{
	showPaletteEditorToggle->setToggle(false);
	}

void Visualizer::createStandardLuminancePaletteCallback(GLMotif::Menu::EntrySelectCallbackData* cbData)
	{
	if(!inLoadPalette)
		variableManager->createPalette(VariableManager::LUMINANCE_GREY+cbData->menu->getEntryIndex(cbData->selectedButton));
	}

void Visualizer::createStandardSaturationPaletteCallback(GLMotif::Menu::EntrySelectCallbackData* cbData)
	{
	if(!inLoadPalette)
		variableManager->createPalette(VariableManager::SATURATION_RED_CYAN+cbData->menu->getEntryIndex(cbData->selectedButton));
	}

void Visualizer::showElementListCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Hide or show element list based on toggle button state: */
	if(cbData->set)
		Vrui::popupPrimaryWidget(elementList->getElementListDialog());
	else
		Vrui::popdownPrimaryWidget(elementList->getElementListDialog());
	}

void Visualizer::elementListClosedCallback(Misc::CallbackData* cbData)
	{
	showElementListToggle->setToggle(false);
	}

void Visualizer::loadElementsCallback(Misc::CallbackData*)
	{
	if(!inLoadElements)
		{
		/* Create a file selection dialog to select an element file: */
		GLMotif::FileSelectionDialog* fsDialog=new GLMotif::FileSelectionDialog(Vrui::getWidgetManager(),"Load Visualization Elements...",IO::openDirectory("."),".asciielem;.binelem");
		fsDialog->getOKCallbacks().add(this,&Visualizer::loadElementsOKCallback);
		fsDialog->getCancelCallbacks().add(this,&Visualizer::loadElementsCancelCallback);
		Vrui::popupPrimaryWidget(fsDialog);
		inLoadElements=true;
		}
	}

void Visualizer::loadElementsOKCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	try
		{
		/* Determine the type of the element file: */
		if(Misc::hasCaseExtension(cbData->selectedFileName,".asciielem"))
			{
			/* Load the ASCII elements file: */
			loadElements(cbData->selectedDirectory->getPath(cbData->selectedFileName).c_str(),true);
			}
		else if(Misc::hasCaseExtension(cbData->selectedFileName,".binelem"))
			{
			/* Load the binary elements file: */
			loadElements(cbData->selectedDirectory->getPath(cbData->selectedFileName).c_str(),false);
			}
		}
	catch(const std::runtime_error& err)
		{
		Misc::sourcedUserError(__PRETTY_FUNCTION__,"Cannot load element file %s due to exception %s",cbData->selectedDirectory->getPath(cbData->selectedFileName).c_str(),err.what());
		}
	
	/* Destroy the file selection dialog: */
	Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
	inLoadElements=false;
	}

void Visualizer::loadElementsCancelCallback(GLMotif::FileSelectionDialog::CancelCallbackData* cbData)
	{
	/* Destroy the file selection dialog: */
	Vrui::getWidgetManager()->deleteWidget(cbData->fileSelectionDialog);
	inLoadElements=false;
	}

void Visualizer::saveElementsCallback(Misc::CallbackData*)
	{
	if(Vrui::isHeadNode())
		{
		#if 1
		/* Create the ASCII element file: */
		char elementFileNameBuffer[256];
		Misc::createNumberedFileName("SavedElements.asciielem",4,elementFileNameBuffer);
		
		/* Save the visible elements to an ASCII file: */
		elementList->saveElements(elementFileNameBuffer,true,variableManager);
		#else
		/* Create the binary element file: */
		char elementFileNameBuffer[256];
		Misc::createNumberedFileName("SavedElements.binelem",4,elementFileNameBuffer);
		
		/* Save the visible elements to a binary file: */
		elementList->saveElements(elementFileNameBuffer,false,variableManager);
		#endif
		}
	}

void Visualizer::clearElementsCallback(Misc::CallbackData*)
	{
	/* Delete all finished visualization elements: */
	elementList->clear();
	}

VRUI_APPLICATION_RUN(Visualizer)
