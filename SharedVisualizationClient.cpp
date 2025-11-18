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

#include "SharedVisualizationClient.h"

#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Threads/FunctionCalls.h>
#include <Vrui/Vrui.h>
#include <Collaboration2/MessageReader.h>
#include <Collaboration2/DataType.icpp>
#include <Collaboration2/MessageContinuation.h>
#include <Collaboration2/NonBlockSocket.h>
#include <Collaboration2/Client.h>

#include <Abstract/VariableManager.h>
#include <Abstract/Parameters.h>
#include <Abstract/Algorithm.h>
#include <Abstract/Element.h>

#include "ElementList.h"

namespace Collab {

namespace Plugins {

/*********************************************************
Methods of class SharedVisualizationClient::SharedElement:
*********************************************************/

SharedVisualizationClient::SharedElement::~SharedElement(void)
	{
	/* Destroy the algorithm used to create the element: */
	delete algorithm;
	
	/* Destroy the shared parameters structure: */
	if(parameters!=0)
		elementTypes.destroyObject(parametersType,parameters);
	}

/******************************************
Methods of class SharedVisualizationClient:
******************************************/

PaletteEditor::Storage* SharedVisualizationClient::convertPalette(const SharedVisualizationProtocol::ColorMap& colorMap)
	{
	std::vector<PaletteEditor::Storage::Entry> entries;
	entries.reserve(colorMap.size());
	for(ColorMap::const_iterator eIt=colorMap.begin();eIt!=colorMap.end();++eIt)
		entries.push_back(PaletteEditor::Storage::Entry(eIt->value,eIt->color));
	
	return new PaletteEditor::Storage(entries);
	}

void SharedVisualizationClient::paletteChangedCallback(Visualization::Abstract::VariableManager::PaletteChangedCallbackData* cbData)
	{
	/* Bail out if the client is currently updating palettes: */
	if(inSetPalette)
		return;
	
	/* Upload the changed palette to the server: */
	ColorMapUpdatedMsg colorMapUpdatedRequest;
	colorMapUpdatedRequest.scalarVariableIndex=cbData->scalarVariableIndex;
	colorMapUpdatedRequest.colorMap.reserve(cbData->newPalette.getNumEntries());
	for(unsigned int i=0;i<cbData->newPalette.getNumEntries();++i)
		{
		ColorMapEntry cme;
		cme.value=cbData->newPalette.getKey(i);
		cme.color=cbData->newPalette.getColor(i);
		colorMapUpdatedRequest.colorMap.push_back(cme);
		}
	sendServerMessage(ColorMapUpdatedRequest,&colorMapUpdatedRequest,false);
	}

MessageContinuation* SharedVisualizationClient::connectRejectCallback(unsigned int messageId,MessageContinuation* continuation)
	{
	/* Signal a bad connection: */
	{
	Threads::MutexCond::Lock connectionEstablishedLock(connectionEstablishedCond);
	receivedReply=true;
	connected=false;
	connectionEstablishedCond.signal();
	}
	
	/* Done with message: */
	return 0;
	}

MessageContinuation* SharedVisualizationClient::connectReplyCallback(unsigned int messageId,MessageContinuation* continuation)
	{
	/* Check if this is the start of a new message: */
	if(continuation==0)
		{
		/* Prepare to read the connect reply message: */
		continuation=protocolTypes.prepareReading(serverMessageTypes[ConnectReply],new ConnectReplyMsg);
		}
	
	/* Continue reading the connect reply message and check whether it's complete: */
	if(protocolTypes.continueReading(client->getSocket(),continuation))
		{
		/* Extract the connect reply message: */
		ConnectReplyMsg* msg=protocolTypes.getReadObject<ConnectReplyMsg>(continuation);
		
		/* Store the color maps received from the server: */
		unsigned int numScalarVariables=variableManager->getNumScalarVariables();
		for(Misc::Vector<ConnectReplyMsg::ColorMapListEntry>::iterator cmIt=msg->colorMaps.begin();cmIt!=msg->colorMaps.end();++cmIt)
			if(cmIt->scalarVariableIndex<numScalarVariables)
				colorMaps[cmIt->scalarVariableIndex]=new ColorMap(cmIt->colorMap);
		
		/* Signal a good connection: */
		{
		Threads::MutexCond::Lock connectionEstablishedLock(connectionEstablishedCond);
		receivedReply=true;
		connected=true;
		connectionEstablishedCond.signal();
		}
		
		/* Delete the connect reply message and the continuation object: */
		delete msg;
		delete continuation;
		continuation=0;
		}
	
	return continuation;
	}

void SharedVisualizationClient::colorMapUpdatedNotificationFrontendCallback(unsigned int messageId,MessageReader& message)
	{
	/* Read the color map updated message: */
	ColorMapUpdatedMsg msg;
	protocolTypes.read(message,serverMessageTypes[ColorMapUpdatedNotification],&msg);
	
	/* Update the color map in the variable manager: */
	inSetPalette=true;
	variableManager->setPalette(msg.scalarVariableIndex,convertPalette(msg.colorMap));
	inSetPalette=false;
	}

MessageContinuation* SharedVisualizationClient::colorMapUpdatedNotificationCallback(unsigned int messageId,MessageContinuation* continuation)
	{
	/* Check if this is the start of a new message: */
	if(continuation==0)
		{
		/* Prepare to read the color map updated notification message: */
		continuation=protocolTypes.prepareReading(serverMessageTypes[ColorMapUpdatedNotification],new ColorMapUpdatedMsg);
		}
	
	/* Continue reading the color map updated notification message and check whether it's complete: */
	if(protocolTypes.continueReading(client->getSocket(),continuation))
		{
		/* Extract the color map updated notification message: */
		ColorMapUpdatedMsg* msg=protocolTypes.getReadObject<ColorMapUpdatedMsg>(continuation);
		
		/* Write the message structure into a message buffer: */
		MessageWriter message(MessageBuffer::create(serverMessageBase+ColorMapUpdatedNotification,protocolTypes.calcSize(serverMessageTypes[ColorMapUpdatedNotification],msg)));
		protocolTypes.write(serverMessageTypes[ColorMapUpdatedNotification],msg,message);
		
		/* Forward the message to the front end: */
		client->queueFrontendMessage(message.getBuffer());
		
		/* Delete the connect reply message and the continuation object: */
		delete msg;
		delete continuation;
		continuation=0;
		}
	
	return continuation;
	}

void* SharedVisualizationClient::createElementFunction(KoinoniaClient* client,KoinoniaProtocol::NamespaceID namespaceId,DataType::TypeID type,void* userData)
	{
	SharedVisualizationClient* thisPtr=static_cast<SharedVisualizationClient*>(userData);
	
	/* Return a new shared element structure: */
	return new SharedElement(thisPtr->elementTypeDictionary);
	}

void SharedVisualizationClient::extractElementJobComplete(Threads::WorkerPool::JobFunction* job,SharedVisualizationClient::SharedElement* sharedElement)
	{
	/* Check whether the element hasn't been destroyed already: */
	if(!sharedElement->destroyed)
		{
		/* Add the shared element to the secondary map: */
		sharedElementsByElement.setEntry(SharedElementByElementMap::Entry(sharedElement->element,sharedElement));
		
		/* Add the extracted element to the element list and set its visibility: */
		elementList->addElement(sharedElement->algorithm,sharedElement->element,true);
		elementList->setElementVisible(sharedElement->element,sharedElement->visible,true);
		
		/* Register a parameters updated callback with the new element: */
		sharedElement->element->getParametersUpdatedCallbacks().add(this,&SharedVisualizationClient::elementParametersUpdatedCallback);
		}
	else
		{
		/* Remove the shared element from the primary map and delete it: */
		sharedElementsById.removeEntry(sharedElement->objectId);
		delete sharedElement;
		}
	}

void SharedVisualizationClient::extractElementJob(int,SharedVisualizationClient::SharedElement* sharedElement)
	{
	/* Get an algorithm to create the new element: */
	if(sharedElement->algorithmIndex<numScalarAlgorithms)
		sharedElement->algorithm=module->getScalarAlgorithm(sharedElement->algorithmIndex,variableManager,Vrui::openPipe());
	else
		sharedElement->algorithm=module->getVectorAlgorithm(sharedElement->algorithmIndex-numScalarAlgorithms,variableManager,Vrui::openPipe());
	
	/* Extract the element using the received extraction parameters: */
	Visualization::Abstract::Parameters* parameters=sharedElement->algorithm->cloneParameters();
	parameters->read(sharedElement->parameters,variableManager);
	sharedElement->element=sharedElement->algorithm->createElement(parameters);
	}

void SharedVisualizationClient::elementCreatedCallback(KoinoniaClient* client,KoinoniaProtocol::NamespaceID namespaceId,KoinoniaProtocol::ObjectID objectId,void* object,void* userData)
	{
	SharedVisualizationClient* thisPtr=static_cast<SharedVisualizationClient*>(userData);
	
	/* Add the new shared element to the main map: */
	SharedElement* sharedElement=static_cast<SharedElement*>(object);
	sharedElement->objectId=objectId;
	sharedElement->parametersType=thisPtr->algorithmParameterTypes[sharedElement->algorithmIndex];
	thisPtr->sharedElementsById.setEntry(SharedElementByIDMap::Entry(sharedElement->objectId,sharedElement));
	
	/* Submit a background job to extract the new element: */
	Vrui::submitJob(*Threads::createFunctionCall(thisPtr,&SharedVisualizationClient::extractElementJob,sharedElement),*Threads::createFunctionCall(thisPtr,&SharedVisualizationClient::extractElementJobComplete,sharedElement));
	}

void SharedVisualizationClient::elementReplacedCallback(KoinoniaClient* client,KoinoniaProtocol::NamespaceID namespaceId,KoinoniaProtocol::ObjectID objectId,KoinoniaProtocol::VersionNumber newVersion,void* object,void* userData)
	{
	SharedVisualizationClient* thisPtr=static_cast<SharedVisualizationClient*>(userData);
	
	/* Access the shared element: */
	SharedElement* sharedElement=thisPtr->sharedElementsById.getEntry(objectId).getDest();
	
	/* Update the shared element's parameters: */
	sharedElement->element->getParameters()->read(sharedElement->parameters,thisPtr->variableManager);
	
	/* Check if the element has already been added to the element list: */
	if(sharedElement->element!=0)
		{
		/* Update the element's visibility in the element list: */
		thisPtr->elementList->setElementVisible(sharedElement->element,sharedElement->visible,true);
		}
	}

void SharedVisualizationClient::elementDestroyedCallback(KoinoniaClient* client,KoinoniaProtocol::NamespaceID namespaceId,KoinoniaProtocol::ObjectID objectId,void* object,void* userData)
	{
	SharedVisualizationClient* thisPtr=static_cast<SharedVisualizationClient*>(userData);
	
	/* Access the destroyed shared element: */
	SharedElement* sharedElement=thisPtr->sharedElementsById.getEntry(objectId).getDest();
	
	/* Check if the element has already been added to the element list: */
	if(sharedElement->element!=0)
		{
		/* Tell the element list that the shared element is being destroyed: */
		thisPtr->elementList->deleteElement(sharedElement->element,true);
		
		/* Remove the shared element from both hash tables and delete it: */
		thisPtr->sharedElementsById.removeEntry(sharedElement->objectId);
		thisPtr->sharedElementsByElement.removeEntry(sharedElement->element);
		delete sharedElement;
		}
	else
		{
		/* Mark the element for deletion once it's done extracting: */
		sharedElement->destroyed=true;
		}
	}

void SharedVisualizationClient::elementParametersUpdatedCallback(Visualization::Abstract::Element::ParametersUpdatedCallbackData* cbData)
	{
	/* Find the shared element associated with the given visualization element: */
	SharedElement* sharedElement=sharedElementsByElement.getEntry(cbData->element).getDest();
	
	/* Update the shared element: */
	sharedElement->element->getParameters()->write(sharedElement->parameters);
	koinonia->replaceNsObject(elementNamespaceId,sharedElement->objectId);
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

SharedVisualizationClient::SharedVisualizationClient(Client* sClient,Visualization::Abstract::VariableManager* sVariableManager,Visualization::Abstract::Module* sModule,ElementList* sElementList)
	:PluginClient(sClient),
	 variableManager(sVariableManager),
	 colorMaps(new ColorMap*[variableManager->getNumScalarVariables()]),
	 inSetPalette(false),
	 module(sModule),algorithmIndices(17),elementList(sElementList),
	 koinonia(KoinoniaClient::requestClient(client)),
	 receivedReply(false),connected(false),
	 numScalarAlgorithms(module->getNumScalarAlgorithms()),numVectorAlgorithms(variableManager->getNumVectorVariables()>0?module->getNumVectorAlgorithms():0),
	 algorithmParameterTypes(new DataType::TypeID[numScalarAlgorithms+numVectorAlgorithms]),elementTypes(new DataType::TypeID[numScalarAlgorithms+numVectorAlgorithms]),
	 sharedElementsById(17),sharedElementsByElement(17)
	{
	/* Initialize the scalar variable color maps: */
	for(int i=0;i<variableManager->getNumScalarVariables();++i)
		colorMaps[i]=0;
	
	/* Register all scalar algorithms: */
	for(unsigned int i=0;i<numScalarAlgorithms;++i)
		{
		algorithmIndices.setEntry(AlgorithmNameMap::Entry(module->getScalarAlgorithmName(i),i));
		algorithmParameterTypes[i]=module->createScalarAlgorithmParametersType(i,elementTypeDictionary);
		}
	
	/* Register all vector algorithms: */
	for(unsigned int i=0;i<numVectorAlgorithms;++i)
		{
		algorithmIndices.setEntry(AlgorithmNameMap::Entry(module->getVectorAlgorithmName(i),numScalarAlgorithms+i));
		algorithmParameterTypes[numScalarAlgorithms+i]=module->createVectorAlgorithmParametersType(i,elementTypeDictionary);
		}
	
	/* Register element types for all algorithms: */
	for(unsigned int i=0;i<numScalarAlgorithms+numVectorAlgorithms;++i)
		{
		DataType::TypeID pointerType=elementTypeDictionary.createPointer(algorithmParameterTypes[i]);
		DataType::StructureElement sharedElementElements[]=
			{
			{DataType::getAtomicType<Misc::UInt8>(),offsetof(SharedElement,algorithmIndex)},
			{pointerType,offsetof(SharedElement,parameters)},
			{DataType::getAtomicType<bool>(),offsetof(SharedElement,visible)},
			};
		elementTypes[i]=elementTypeDictionary.createStructure(3,sharedElementElements,sizeof(SharedElement));
		}
	
	/* Create a Koinonia namespace for extracted visualization elements: */
	elementNamespaceId=koinonia->shareNamespace("SharedVisualization::Elements",protocolVersion,elementTypeDictionary,
	                                            createElementFunction,this,
	                                            elementCreatedCallback,this,
	                                            elementReplacedCallback,this,
	                                            elementDestroyedCallback,this);
	
	/* Register a palette changed callback with the variable manager: */
	variableManager->getPaletteChangedCallbacks().add(this,&SharedVisualizationClient::paletteChangedCallback);
	}

#pragma GCC diagnostic pop

SharedVisualizationClient::~SharedVisualizationClient(void)
	{
	/* Unregister the palette changed callback with the variable manager: */
	variableManager->getPaletteChangedCallbacks().remove(this,&SharedVisualizationClient::paletteChangedCallback);
	
	/* Destroy all shared elements: */
	for(SharedElementByIDMap::Iterator seIt=sharedElementsById.begin();!seIt.isFinished();++seIt)
		delete seIt->getDest();
	
	delete[] algorithmParameterTypes;
	delete[] elementTypes;
	
	/* Release the scalar variable color maps: */
	for(int i=0;i<variableManager->getNumScalarVariables();++i)
		delete colorMaps[i];
	}

const char* SharedVisualizationClient::getName(void) const
	{
	return protocolName;
	}

unsigned int SharedVisualizationClient::getVersion(void) const
	{
	return protocolVersion;
	}

unsigned int SharedVisualizationClient::getNumClientMessages(void) const
	{
	return NumClientMessages;
	}

unsigned int SharedVisualizationClient::getNumServerMessages(void) const
	{
	return NumServerMessages;
	}

void SharedVisualizationClient::setMessageBases(unsigned int newClientMessageBase,unsigned int newServerMessageBase)
	{
	/* Call the base class method: */
	PluginClient::setMessageBases(newClientMessageBase,newServerMessageBase);
	
	/* Register front-end message handlers: */
	client->setFrontendMessageHandler(serverMessageBase+ColorMapUpdatedNotification,Client::wrapMethod<SharedVisualizationClient,&SharedVisualizationClient::colorMapUpdatedNotificationFrontendCallback>,this);
	
	/* Register message handlers: */
	client->setTCPMessageHandler(serverMessageBase+ConnectReject,Client::wrapMethod<SharedVisualizationClient,&SharedVisualizationClient::connectRejectCallback>,this,0);
	client->setTCPMessageHandler(serverMessageBase+ConnectReply,Client::wrapMethod<SharedVisualizationClient,&SharedVisualizationClient::connectReplyCallback>,this,getServerMsgSize(ConnectReply));
	client->setTCPMessageHandler(serverMessageBase+ColorMapUpdatedNotification,Client::wrapMethod<SharedVisualizationClient,&SharedVisualizationClient::colorMapUpdatedNotificationCallback>,this,getServerMsgSize(ColorMapUpdatedNotification));
	}

void SharedVisualizationClient::start(void)
	{
	/* Send a connect request message to the server: */
	ConnectRequestMsg connectRequest;
	connectRequest.numScalarVariables=VariableIndex(variableManager->getNumScalarVariables());
	connectRequest.numVectorVariables=VariableIndex(variableManager->getNumVectorVariables());
	sendServerMessage(ConnectRequest,&connectRequest,true);
	}

bool SharedVisualizationClient::waitForConnection(void)
	{
	/* Wait until the connect reject or reply message is received: */
	{
	Threads::MutexCond::Lock connectionEstablishedLock(connectionEstablishedCond);
	while(!receivedReply)
		connectionEstablishedCond.wait(connectionEstablishedLock);
	}
	
	if(connected)
		{
		/* Copy the color maps received from the server into the variable manager: */
		inSetPalette=true;
		for(int i=0;i<variableManager->getNumScalarVariables();++i)
			if(colorMaps[i]!=0)
				variableManager->setPalette(i,convertPalette(*colorMaps[i]));
		inSetPalette=false;
		}
	
	/* Return the connection status: */
	return connected;
	}

void SharedVisualizationClient::addElement(Visualization::Abstract::Algorithm* algorithm,Visualization::Abstract::Element* newElement)
	{
	/* Create a new shared element: */
	SharedElement* sharedElement=new SharedElement(elementTypeDictionary);
	sharedElement->algorithmIndex=algorithmIndices.getEntry(algorithm->getName()).getDest();
	sharedElement->parametersType=algorithmParameterTypes[sharedElement->algorithmIndex];
	sharedElement->parameters=elementTypeDictionary.createObject(sharedElement->parametersType);
	newElement->getParameters()->write(sharedElement->parameters);
	sharedElement->element=newElement;
	
	/* Create a new object in the visualization element namespace: */
	sharedElement->objectId=koinonia->createNsObject(elementNamespaceId,elementTypes[sharedElement->algorithmIndex],sharedElement);
	
	/* Add the new shared element to both maps: */
	sharedElementsById.setEntry(SharedElementByIDMap::Entry(sharedElement->objectId,sharedElement));
	sharedElementsByElement.setEntry(SharedElementByElementMap::Entry(sharedElement->element,sharedElement));
	
	/* Register a parameters updated callback with the new element: */
	sharedElement->element->getParametersUpdatedCallbacks().add(this,&SharedVisualizationClient::elementParametersUpdatedCallback);
	}

void SharedVisualizationClient::setElementVisible(Visualization::Abstract::Element* element,bool newVisible)
	{
	/* Find the shared element associated with the given visualization element: */
	SharedElement* sharedElement=sharedElementsByElement.getEntry(element).getDest();
	
	/* Update the shared element: */
	if(sharedElement->visible!=newVisible)
		{
		sharedElement->visible=newVisible;
		koinonia->replaceNsObject(elementNamespaceId,sharedElement->objectId);
		}
	}

void SharedVisualizationClient::deleteElement(Visualization::Abstract::Element* element)
	{
	/* Find the shared element associated with the given visualization element: */
	SharedElement* sharedElement=sharedElementsByElement.getEntry(element).getDest();
	
	/* Delete the shared element in the visualization element namespace: */
	koinonia->destroyNsObject(elementNamespaceId,sharedElement->objectId);
	
	/* Remove the shared element from both hash tables and delete it: */
	sharedElementsById.removeEntry(sharedElement->objectId);
	sharedElementsByElement.removeEntry(sharedElement->element);
	delete sharedElement;
	}

}

}
