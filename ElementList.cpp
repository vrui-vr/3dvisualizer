/***********************************************************************
ElementList - Class to manage a list of previously extracted
visualization elements.
Copyright (c) 2009-2023 Oliver Kreylos

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

#include "ElementList.h"

#include <stdexcept>
#include <Misc/StandardMarshallers.h>
#include <Misc/File.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Separator.h>
#include <GLMotif/ScrolledListBox.h>
#include <Vrui/Vrui.h>
#include <Vrui/SceneGraphManager.h>

#include <Abstract/Parameters.h>
#include <Abstract/Algorithm.h>
#include <Abstract/BinaryParametersSink.h>
#include <Abstract/FileParametersSink.h>
#include <Abstract/Element.h>

#if VISUALIZATION_CONFIG_USE_COLLABORATION
#include "SharedVisualizationClient.h"
#endif

/****************************
Methods of class ElementList:
****************************/

void ElementList::updateUiState(void)
	{
	/* Update the toggle buttons: */
	int selectedElementIndex=elementList->getSelectedItem();
	if(selectedElementIndex>=0)
		{
		/* Update the toggle buttons: */
		showElementToggle->setEnabled(true);
		showElementToggle->setToggle(elements[selectedElementIndex].show);
		showElementSettingsToggle->setEnabled(elements[selectedElementIndex].settingsDialog!=0);
		showElementSettingsToggle->setToggle(elements[selectedElementIndex].settingsDialogVisible);
		}
	else
		{
		/* Reset the toggle buttons: */
		showElementToggle->setToggle(false);
		showElementToggle->setEnabled(false);
		showElementSettingsToggle->setToggle(false);
		showElementSettingsToggle->setEnabled(false);
		}
	}

void ElementList::elementListValueChangedCallback(GLMotif::ListBox::ValueChangedCallbackData* cbData)
	{
	/* Update the user interface: */
	updateUiState();
	}

void ElementList::elementListItemSelectedCallback(GLMotif::ListBox::ItemSelectedCallbackData* cbData)
	{
	if(cbData->selectedItem>=0)
		{
		/* Toggle the visibility state of the selected item: */
		elements[cbData->selectedItem].show=!elements[cbData->selectedItem].show;
		
		/* Add or remove the selected element to or from Vrui's scene graph: */
		if(elements[cbData->selectedItem].show)
			Vrui::getSceneGraphManager()->addNavigationalNode(*elements[cbData->selectedItem].element);
		else
			Vrui::getSceneGraphManager()->removeNavigationalNode(*elements[cbData->selectedItem].element);
		
		#if VISUALIZATION_CONFIG_USE_COLLABORATION
		
		/* Let a shared visualization client know that the element is changing visibility: */
		if(sharedVisualizationClient!=0)
			sharedVisualizationClient->setElementVisible(elements[cbData->selectedItem].element.getPointer(),elements[cbData->selectedItem].show);
		
		#endif
		
		/* Update the user interface: */
		updateUiState();
		}
	}

void ElementList::showElementToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	int selectedElementIndex=elementList->getSelectedItem();
	if(selectedElementIndex>=0)
		{
		/* Show or hide the element: */
		elements[selectedElementIndex].show=cbData->set;
		
		/* Add or remove the selected element to or from Vrui's scene graph: */
		if(elements[selectedElementIndex].show)
			Vrui::getSceneGraphManager()->addNavigationalNode(*elements[selectedElementIndex].element);
		else
			Vrui::getSceneGraphManager()->removeNavigationalNode(*elements[selectedElementIndex].element);
		
		#if VISUALIZATION_CONFIG_USE_COLLABORATION
		
		/* Let a shared visualization client know that the element is changing visibility: */
		if(sharedVisualizationClient!=0)
			sharedVisualizationClient->setElementVisible(elements[selectedElementIndex].element.getPointer(),cbData->set);
		
		#endif
		}
	else
		cbData->toggle->setToggle(false);
	}

void ElementList::showElementSettingsToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	int selectedElementIndex=elementList->getSelectedItem();
	if(selectedElementIndex>=0&&elements[selectedElementIndex].settingsDialog!=0)
		{
		/* Show or hide the element's settings dialog: */
		if(cbData->set)
			{
			typedef GLMotif::WidgetManager::Transformation WTransform;
			typedef WTransform::Vector WVector;
			
			/* Open the settings dialog right next to the element list dialog: */
			WTransform transform=widgetManager->calcWidgetTransformation(elementListDialogPopup);
			const GLMotif::Box& box=elementListDialogPopup->getExterior();
			WVector offset(box.origin[0]+box.size[0],box.origin[1]+box.size[1]*0.5f,0.0f);
			
			GLMotif::Widget* dialog=elements[selectedElementIndex].settingsDialog;
			offset[0]-=dialog->getExterior().origin[0];
			offset[1]-=dialog->getExterior().origin[1]+dialog->getExterior().size[1]*0.5f;
			transform*=WTransform::translate(offset);
			widgetManager->popupPrimaryWidget(dialog,transform);
			}
		else
			widgetManager->popdownWidget(elements[selectedElementIndex].settingsDialog);
		}
	else
		cbData->toggle->setToggle(false);
	}

void ElementList::elementSettingsCloseCallback(Misc::CallbackData* cbData)
	{
	GLMotif::PopupWindow::CloseCallbackData* myCbData=dynamic_cast<GLMotif::PopupWindow::CloseCallbackData*>(cbData);
	if(myCbData!=0)
		{
		/* Find the element whose settings dialog was closed: */
		ListElementList::iterator eIt;
		for(eIt=elements.begin();eIt!=elements.end();++eIt)
			if(myCbData->popupWindow==eIt->settingsDialog)
				{
				/* Update the element and the UI: */
				eIt->settingsDialogVisible=false;
				updateUiState();
				break;
				}
		}
	}

void ElementList::deleteElementSelectedCallback(Misc::CallbackData* cbData)
	{
	int selectedElementIndex=elementList->getSelectedItem();
	if(selectedElementIndex>=0)
		{
		#if VISUALIZATION_CONFIG_USE_COLLABORATION
		
		/* Let a shared visualizaiton client know that the element is being deleted: */
		if(sharedVisualizationClient!=0)
			sharedVisualizationClient->deleteElement(elements[selectedElementIndex].element.getPointer());
		
		#endif
		
		/* Remove the visualization element from Vrui's scene graph if it was visible: */
		if(elements[selectedElementIndex].show)
			Vrui::getSceneGraphManager()->removeNavigationalNode(*elements[selectedElementIndex].element);
		
		/* Delete the visualization element and its settings dialog: */
		delete elements[selectedElementIndex].settingsDialog;
		elements.erase(elements.begin()+selectedElementIndex);
		
		/* Remove the entry from the list box: */
		elementList->removeItem(selectedElementIndex);
		
		/* Update the user interface: */
		updateUiState();
		}
	}

ElementList::ElementList(GLMotif::WidgetManager* sWidgetManager)
	:widgetManager(sWidgetManager),
	 #if VISUALIZATION_CONFIG_USE_COLLABORATION
	 sharedVisualizationClient(0),
	 #endif
	 elementListDialogPopup(0),elementList(0)
	{
	/* Create the settings dialog window: */
	elementListDialogPopup=new GLMotif::PopupWindow("ElementListDialogPopup",widgetManager,"Visualization Element List");
	elementListDialogPopup->setResizableFlags(true,true);
	
	GLMotif::RowColumn* elementListDialog=new GLMotif::RowColumn("ElementListDialog",elementListDialogPopup,false);
	elementListDialog->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	elementListDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	elementListDialog->setNumMinorWidgets(1);
	
	/* Create a listbox containing all visualization elements: */
	GLMotif::ScrolledListBox* scrolledElementList=new GLMotif::ScrolledListBox("ScrolledElementList",elementListDialog,GLMotif::ListBox::ALWAYS_ONE,20,10);
	scrolledElementList->showHorizontalScrollBar(false);
	elementList=scrolledElementList->getListBox();
	elementList->getValueChangedCallbacks().add(this,&ElementList::elementListValueChangedCallback);
	elementList->getItemSelectedCallbacks().add(this,&ElementList::elementListItemSelectedCallback);
	
	elementListDialog->setColumnWeight(0,1.0f);
	
	/* Create a list of buttons to control elements: */
	GLMotif::Margin* buttonBoxMargin=new GLMotif::Margin("ButtonBoxMargin",elementListDialog,false);
	buttonBoxMargin->setAlignment(GLMotif::Alignment::VCENTER);
	
	GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",buttonBoxMargin,false);
	buttonBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	buttonBox->setNumMinorWidgets(1);
	
	showElementToggle=new GLMotif::ToggleButton("ShowElementToggle",buttonBox,"Show");
	showElementToggle->getValueChangedCallbacks().add(this,&ElementList::showElementToggleValueChangedCallback);
	
	showElementSettingsToggle=new GLMotif::ToggleButton("ShowElementSettingsToggle",buttonBox,"Show Settings");
	showElementSettingsToggle->getValueChangedCallbacks().add(this,&ElementList::showElementSettingsToggleValueChangedCallback);
	
	new GLMotif::Separator("Separator",buttonBox,GLMotif::Separator::HORIZONTAL,0.0f,GLMotif::Separator::LOWERED);
	
	GLMotif::Button* deleteElementButton=new GLMotif::Button("DeleteElementButton",buttonBox,"Delete");
	deleteElementButton->getSelectCallbacks().add(this,&ElementList::deleteElementSelectedCallback);
	
	buttonBox->manageChild();
	
	buttonBoxMargin->manageChild();
	
	elementListDialog->manageChild();
	}

ElementList::~ElementList(void)
	{
	/* Delete all elements: */
	clear();
	
	/* Delete the element list dialog: */
	delete elementListDialogPopup;
	}

#if VISUALIZATION_CONFIG_USE_COLLABORATION

void ElementList::setSharedVisualizationClient(Collab::Plugins::SharedVisualizationClient* newSharedVisualizationClient)
	{
	sharedVisualizationClient=newSharedVisualizationClient;
	}

#endif

void ElementList::clear(void)
	{
	/* Delete all visualization elements: */
	for(ListElementList::iterator eIt=elements.begin();eIt!=elements.end();++eIt)
		{
		delete eIt->settingsDialog;
		
		#if VISUALIZATION_CONFIG_USE_COLLABORATION
		
		/* Let a shared visualization client know that the element is being deleted: */
		if(sharedVisualizationClient!=0)
			sharedVisualizationClient->deleteElement(eIt->element.getPointer());
		
		#endif
		
		/* Remove the visualization element from Vrui's scene graph if it was visible: */
		if(eIt->show)
			Vrui::getSceneGraphManager()->removeNavigationalNode(*eIt->element);
		}
	elements.clear();
	
	/* Clear the list box: */
	elementList->clear();
	
	/* Update the GUI: */
	updateUiState();
	}

void ElementList::addElement(Visualization::Abstract::Algorithm* algorithm,ElementList::Element* newElement,bool fromSharedVisualizationClient)
	{
	/* Create the element's list structure: */
	ListElement le;
	le.element=newElement;
	le.name=algorithm->getName();
	le.settingsDialog=newElement->createSettingsDialog(widgetManager);
	le.settingsDialogVisible=false;
	le.show=true;
	
	/* Add the element to the list and select it: */
	elements.push_back(le);
	elementList->selectItem(elementList->addItem(le.name.c_str()),true);
	
	/* Update the toggle buttons: */
	showElementToggle->setToggle(true);
	showElementSettingsToggle->setToggle(false);
	
	/* Check if the element's settings dialog is a dialog: */
	GLMotif::PopupWindow* sd=dynamic_cast<GLMotif::PopupWindow*>(le.settingsDialog);
	if(sd!=0)
		{
		/* Add a close button to the settings dialog, and register a close callback: */
		sd->setCloseButton(true);
		sd->getCloseCallbacks().add(this,&ElementList::elementSettingsCloseCallback);
		}
	
	#if VISUALIZATION_CONFIG_USE_COLLABORATION
	
	/* Let a shared visualization client know that a new element is being added: */
	if(!fromSharedVisualizationClient&&sharedVisualizationClient!=0)
		sharedVisualizationClient->addElement(algorithm,newElement);
	
	#endif
	
	/* Add the element to Vrui's scene graph: */
	Vrui::getSceneGraphManager()->addNavigationalNode(*newElement);
	}

void ElementList::setElementVisible(ElementList::Element* element,bool newVisible,bool fromSharedVisualizationClient)
	{
	/* Find the index of the element in the element list: */
	size_t elementIndex;
	for(elementIndex=0;elementIndex<elements.size()&&elements[elementIndex].element!=element;++elementIndex)
		;
	if(elementIndex<elements.size()&&elements[elementIndex].show!=newVisible)
		{
		#if VISUALIZATION_CONFIG_USE_COLLABORATION
		
		/* Let a shared visualization client know that the element is changing visibility: */
		if(!fromSharedVisualizationClient&&sharedVisualizationClient!=0)
			sharedVisualizationClient->setElementVisible(element,newVisible);
		
		#endif
		
		/* Update the element's visibility: */
		elements[elementIndex].show=newVisible;
		
		/* Add or remove the selected element to or from Vrui's scene graph: */
		if(elements[elementIndex].show)
			Vrui::getSceneGraphManager()->addNavigationalNode(*elements[elementIndex].element);
		else
			Vrui::getSceneGraphManager()->removeNavigationalNode(*elements[elementIndex].element);
		
		/* Update the user interface: */
		updateUiState();
		}
	}

void ElementList::deleteElement(ElementList::Element* element,bool fromSharedVisualizationClient)
	{
	/* Find the index of the element in the element list: */
	size_t elementIndex;
	for(elementIndex=0;elementIndex<elements.size()&&elements[elementIndex].element!=element;++elementIndex)
		;
	if(elementIndex<elements.size())
		{
		#if VISUALIZATION_CONFIG_USE_COLLABORATION
		
		/* Let a shared visualization client know that a the element is being destroyed: */
		if(!fromSharedVisualizationClient&&sharedVisualizationClient!=0)
			sharedVisualizationClient->deleteElement(element);
		
		#endif
		
		/* Remove the visualization element from Vrui's scene graph if it was visible: */
		if(elements[elementIndex].show)
			Vrui::getSceneGraphManager()->removeNavigationalNode(*element);
		
		/* Delete the visualization element and its settings dialog: */
		delete elements[elementIndex].settingsDialog;
		elements.erase(elements.begin()+elementIndex);
		
		/* Remove the entry from the list box: */
		elementList->removeItem(elementIndex);
		
		/* Update the user interface: */
		updateUiState();
		}
	}

void ElementList::saveElements(const char* elementFileName,bool ascii,const Visualization::Abstract::VariableManager* variableManager) const
	{
	if(ascii)
		{
		/* Create a text element file and a sink to write into it: */
		Misc::File elementFile(elementFileName,"wt");
		Visualization::Abstract::FileParametersSink sink(variableManager,elementFile);
		
		/* Save all visible visualization elements: */
		for(ListElementList::const_iterator veIt=elements.begin();veIt!=elements.end();++veIt)
			if(veIt->show)
				{
				/* Write the element's name: */
				elementFile.puts(veIt->name.c_str());
				elementFile.puts("\n");
				
				/* Write the element's parameters: */
				elementFile.puts("\t{\n");
				veIt->element->getParameters()->write(sink);
				elementFile.puts("\t}\n");
				}
		}
	else
		{
		/* Create a binary element file and a data sink to write into it: */
		IO::FilePtr elementFile(IO::openFile(elementFileName,IO::File::WriteOnly));
		elementFile->setEndianness(Misc::LittleEndian);
		Visualization::Abstract::BinaryParametersSink sink(variableManager,*elementFile,false);
		
		/* Save all visible visualization elements: */
		for(ListElementList::const_iterator veIt=elements.begin();veIt!=elements.end();++veIt)
			if(veIt->show)
				{
				/* Write the element's name: */
				Misc::Marshaller<std::string>::write(veIt->name,*elementFile);
				
				/* Write the element's parameters: */
				veIt->element->getParameters()->write(sink);
				}
		}
	}
