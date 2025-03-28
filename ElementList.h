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

#ifndef ELEMENTLIST_INCLUDED
#define ELEMENTLIST_INCLUDED

#include <Config.h>

#include <string>
#include <vector>
#include <Misc/Autopointer.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/ListBox.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
namespace GLMotif {
class Widget;
class PopupWindow;
}
namespace Visualization {
namespace Abstract {
class Algorithm;
class Element;
class VariableManager;
}
}
#if VISUALIZATION_CONFIG_USE_COLLABORATION
namespace Collab {
namespace Plugins {
class SharedVisualizationClient;
}
}
#endif
class GLRenderState;

class ElementList
	{
	/* Embedded classes: */
	public:
	typedef Visualization::Abstract::Element Element;
	typedef Misc::Autopointer<Element> ElementPointer;
	
	private:
	struct ListElement // Structure storing information relating to a visualization element
		{
		/* Elements: */
		public:
		ElementPointer element; // Pointer to the element itself
		std::string name; // Name of algorithm used to create the element
		GLMotif::Widget* settingsDialog; // Pointer to the element's settings dialog (or NULL)
		bool settingsDialogVisible; // Flag if the element's settings dialog is currently popped up
		bool show; // Flag if the element is being rendered
		};
	
	typedef std::vector<ListElement> ListElementList;
	
	/* Elements: */
	private:
	GLMotif::WidgetManager* widgetManager; // Pointer to the widget manager
	#if VISUALIZATION_CONFIG_USE_COLLABORATION
	Collab::Plugins::SharedVisualizationClient* sharedVisualizationClient; // Pointer to the shared visualization client
	#endif
	ListElementList elements; // List of previously extracted visualization elements
	GLMotif::PopupWindow* elementListDialogPopup; // Dialog listing visualization elements
	GLMotif::ListBox* elementList; // List box widget containing the names of all visualization elements
	GLMotif::ToggleButton* showElementToggle; // Toggle button to set the visibility of a visualization element
	GLMotif::ToggleButton* showElementSettingsToggle; // Toggle button to show or hide a visualization element's settings dialog
	
	/* Private methods: */
	void updateUiState(void); // Updates the state of the element list's user interface
	void elementListValueChangedCallback(GLMotif::ListBox::ValueChangedCallbackData* cbData);
	void elementListItemSelectedCallback(GLMotif::ListBox::ItemSelectedCallbackData* cbData);
	void showElementToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void showElementSettingsToggleValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void elementSettingsCloseCallback(Misc::CallbackData* cbData);
	void deleteElementSelectedCallback(Misc::CallbackData* cbData);
	
	/* Constructors and destructors: */
	public:
	ElementList(GLMotif::WidgetManager* sWidgetManager); // Creates an empty element list
	~ElementList(void); // Destroys the element list
	
	/* Methods: */
	#if VISUALIZATION_CONFIG_USE_COLLABORATION
	void setSharedVisualizationClient(Collab::Plugins::SharedVisualizationClient* newSharedVisualizationClient); // Sets the shared visualization client
	#endif
	void clear(void); // Deletes all elements from the list
	void addElement(Visualization::Abstract::Algorithm* algorithm,Element* newElement,bool fromSharedVisualizationClient =false); // Adds a new visualization element created by the given algorithm to the list
	void setElementVisible(Element* element,bool newVisible,bool fromSharedVisualizationClient =false); // Shows or hides the given visualization element
	void deleteElement(Element* element,bool fromSharedVisualizationClient =false); // Deletes the given visualization element
	void saveElements(const char* elementFileName,bool ascii,const Visualization::Abstract::VariableManager* variableManager) const; // Saves all visible visualization elements to the given file
	GLMotif::PopupWindow* getElementListDialog(void) // Returns the element list dialog
		{
		return elementListDialogPopup;
		}
	};

#endif
