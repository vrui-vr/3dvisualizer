/***********************************************************************
SharedVisualizationClient - Client for collaborative data exploration in
spatially distributed VR environments, implemented as a plug-in of the
Vrui remote collaboration infrastructure.
Copyright (c) 2009-2025 Oliver Kreylos

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

#ifndef SHAREDVISUALIZATIONCLIENT_INCLUDED
#define SHAREDVISUALIZATIONCLIENT_INCLUDED

#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Threads/MutexCond.h>
#include <Threads/WorkerPool.h>
#include <Collaboration2/MessageBuffer.h>
#include <Collaboration2/MessageWriter.h>
#include <Collaboration2/DataType.h>
#include <Collaboration2/Client.h>
#include <Collaboration2/PluginClient.h>
#include <Collaboration2/Plugins/KoinoniaClient.h>

#include <Abstract/VariableManager.h>
#include <Abstract/Module.h>
#include <Abstract/Element.h>
#include <PaletteEditor.h>

#include "SharedVisualizationProtocol.h"

/* Forward declarations: */
namespace Collab {
class MessageContinuation;
class MessageReader;
namespace Plugins {
class KoinoniaClient;
}
}
namespace Visualization {
namespace Abstract {
class Algorithm;
}
}
class ElementList;

namespace Collab {

namespace Plugins {

class SharedVisualizationClient:public PluginClient,public SharedVisualizationProtocol
	{
	/* Embedded classes: */
	private:
	typedef Misc::HashTable<const char*,unsigned int> AlgorithmNameMap; // Type for hash tables to map static algorithm names to algorithm indices
	
	struct SharedElement // Structure to represent a visualization element on the server
		{
		/* Elements: */
		public:
		KoinoniaProtocol::ObjectID objectId; // Element's object ID within the sharing namespace
		Misc::UInt8 algorithmIndex; // Index of the algorithm used to create the element
		Visualization::Abstract::Algorithm* algorithm; // Algorithm used to extract the visualization element
		DataType& elementTypes; // Reference to the element type dictionary
		DataType::TypeID parametersType; // Type of the parameters structure
		void* parameters; // Opaque pointer to the parameters used by the algorithm to create the element
		bool visible; // Flag whether the element is currently being rendered
		Visualization::Abstract::Element* element; // Pointer to the visualization element
		bool destroyed; // Flag if the visualization element has been destroyed before it finished extracting
		
		/* Constructors and destructors: */
		SharedElement(DataType& sElementTypes)
			:objectId(0),
			 algorithmIndex(-1),algorithm(0),
			 elementTypes(sElementTypes),parametersType(-1),parameters(0),
			 visible(true),element(0),destroyed(false)
			{
			}
		private:
		SharedElement(const SharedElement& source); // Prohibit copy constructor
		SharedElement& operator=(const SharedElement& source); // Prohibit assignment operator
		public:
		~SharedElement(void);
		};
	
	typedef Misc::HashTable<Collab::Plugins::KoinoniaProtocol::ObjectID,SharedElement*> SharedElementByIDMap; // Type for hash tables mapping object IDs to shared element states
	typedef Misc::HashTable<Visualization::Abstract::Element*,SharedElement*> SharedElementByElementMap; // Type for hash tables mapping element pointers to shared element states
	
	/* Elements: */
	private:
	Visualization::Abstract::VariableManager* variableManager; // Pointer to the variable manager
	ColorMap** colorMaps; // Array of pointers to scalar variable color maps shared with the server
	bool inSetPalette; // Flag if the client is currently updating a palette in the variable manager
	Visualization::Abstract::Module* module; // Pointer to the visualization module
	AlgorithmNameMap algorithmIndices; // Hash table from static algorithm name pointers to algorithm indices
	ElementList* elementList; // Pointer to the list of extracted visualization elements
	KoinoniaClient* koinonia; // Pointer to the Koinonia protocol client
	KoinoniaClient::NamespaceID elementNamespaceId; // ID for the Koinonia namespace to share visualization elements
	Threads::MutexCond connectionEstablishedCond; // Condition variable to signal when the server's connect reject or reply message has been received
	volatile bool receivedReply; // Flag whether the server's connect reject or reply message has been received
	bool connected; // Flag whether the server accepted the connection request
	unsigned int numScalarAlgorithms,numVectorAlgorithms; // Number of scalar and vector algorithms used by the visualization module
	DataType elementTypeDictionary; // Type dictionary for shared visualization elements
	DataType::TypeID* algorithmParameterTypes; // Array of parameter types for each algorithm
	DataType::TypeID* elementTypes; // Array of visualization element types for each algorithm
	SharedElementByIDMap sharedElementsById; // Hash table mapping object IDs to shared elements
	SharedElementByElementMap sharedElementsByElement; // Hash table mapping element pointers to shared elements
	
	/* Private methods: */
	private:
	size_t getServerMsgSize(unsigned int messageId) const // Returns the minimum size of a server protocol message
		{
		return protocolTypes.getMinSize(serverMessageTypes[messageId]);
		}
	void sendServerMessage(unsigned int messageId,const void* messageStructure,bool direct) // Writes a message structure for the given message ID into a buffer and queues it for delivery to the server, using the direct method if the given flag is true
		{
		/* Create a message writer: */
		MessageWriter message(MessageBuffer::create(clientMessageBase+messageId,protocolTypes.calcSize(clientMessageTypes[messageId],messageStructure)));
		
		/* Write the message structure into the message: */
		protocolTypes.write(clientMessageTypes[messageId],messageStructure,message);
		
		/* Queue the message for delivery to the server directly or indirectly: */
		if(direct)
			client->queueMessage(message.getBuffer());
		else
			client->queueServerMessage(message.getBuffer());
		}
	PaletteEditor::Storage* convertPalette(const ColorMap& colorMap); // Converts a protocol color map into a palette editor palette
	void paletteChangedCallback(Visualization::Abstract::VariableManager::PaletteChangedCallbackData* cbData);
	MessageContinuation* connectRejectCallback(unsigned int messageId,MessageContinuation* continuation);
	MessageContinuation* connectReplyCallback(unsigned int messageId,MessageContinuation* continuation);
	void colorMapUpdatedNotificationFrontendCallback(unsigned int messageId,MessageReader& message);
	MessageContinuation* colorMapUpdatedNotificationCallback(unsigned int messageId,MessageContinuation* continuation);
	static void* createElementFunction(KoinoniaClient* client,KoinoniaProtocol::NamespaceID namespaceId,DataType::TypeID type,void* userData);
	void extractElementJobComplete(Threads::WorkerPool::JobFunction* job,SharedElement* sharedElement); // Called from main thread when an element extraction job is finished
	void extractElementJob(int,SharedElement* sharedElement); // Job function to create a newly-arrived element, called from a background worker thread
	static void elementCreatedCallback(KoinoniaClient* client,KoinoniaProtocol::NamespaceID namespaceId,KoinoniaProtocol::ObjectID objectId,void* object,void* userData);
	static void elementReplacedCallback(KoinoniaClient* client,KoinoniaProtocol::NamespaceID namespaceId,KoinoniaProtocol::ObjectID objectId,KoinoniaProtocol::VersionNumber newVersion,void* object,void* userData);
	static void elementDestroyedCallback(KoinoniaClient* client,KoinoniaProtocol::NamespaceID namespaceId,KoinoniaProtocol::ObjectID objectId,void* object,void* userData);
	void elementParametersUpdatedCallback(Visualization::Abstract::Element::ParametersUpdatedCallbackData* cbData);
	
	/* Constructors and destructors: */
	public:
	SharedVisualizationClient(Client* sClient,Visualization::Abstract::VariableManager* sVariableManager,Visualization::Abstract::Module* sModule,ElementList* sElementList);
	virtual ~SharedVisualizationClient(void);
	
	/* Methods from class PluginClient: */
	virtual const char* getName(void) const;
	virtual unsigned int getVersion(void) const;
	virtual unsigned int getNumClientMessages(void) const;
	virtual unsigned int getNumServerMessages(void) const;
	virtual void setMessageBases(unsigned int newClientMessageBase,unsigned int newServerMessageBase);
	virtual void start(void);
	
	/* New method: */
	bool waitForConnection(void); // Blocks the caller until the server replies to the connect request message; returns true if connection is valid
	void addElement(Visualization::Abstract::Algorithm* algorithm,Visualization::Abstract::Element* newElement); // Notifies the client that a new visualization element has been added to the element list
	void setElementVisible(Visualization::Abstract::Element* element,bool newVisible); // Notifies the client that a visualization element has changed visibility
	void deleteElement(Visualization::Abstract::Element* element); // Notifies the client that the given visualization element is being deleted
	};

}

}

#endif
