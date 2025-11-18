/***********************************************************************
SharedVisualizationServer - Server for collaborative data exploration in
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

#include "SharedVisualizationServer.h"

#include <Collaboration2/MessageContinuation.h>

namespace Collab {

namespace Plugins {

/******************************************
Methods of class SharedVisualizationServer:
******************************************/

MessageContinuation* SharedVisualizationServer::connectRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object and its TCP socket: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	
	/* Read the layout of the client's dataset: */
	unsigned int clientNumScalarVariables=socket.read<VariableIndex>();
	unsigned int clientNumVectorVariables=socket.read<VariableIndex>();
	
	/* Check if the client's dataset is compatible with the one currently "held" on the server: */
	bool compatible=false;
	if(clientNumScalarVariables==numScalarVariables&&clientNumVectorVariables==numVectorVariables)
		compatible=true;
	else if(numScalarVariables==0&&numVectorVariables==0)
		{
		/* Adopt the client's dataset layout: */
		numScalarVariables=clientNumScalarVariables;
		colorMaps=new ColorMap*[numScalarVariables];
		for(unsigned int i=0;i<numScalarVariables;++i)
			colorMaps[i]=0;
		numVectorVariables=clientNumVectorVariables;
		
		compatible=true;
		}
	if(compatible)
		{
		/* Create a connect reply message: */
		ConnectReplyMsg connectReply;
		
		/* Send each already-defined color map to the new client: */
		for(unsigned int i=0;i<numScalarVariables;++i)
			if(colorMaps[i]!=0)
				{
				/* Add the color map to the connect reply message: */
				connectReply.colorMaps.push_back(ConnectReplyMsg::ColorMapListEntry(VariableIndex(i),*colorMaps[i]));
				}
		
		/* Send the connect reply message: */
		sendClientMessage(ConnectReply,&connectReply,client);
		}
	else
		{
		/* Send a connect reject message: */
		client->queueMessage(MessageBuffer::create(serverMessageBase+ConnectReject,0));
		}
	
	/* Done with message: */
	return 0;
	}

MessageContinuation* SharedVisualizationServer::colorMapUpdatedRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation)
	{
	/* Access the base client state object and its TCP socket: */
	Server::Client* client=server->getClient(clientId);
	NonBlockSocket& socket=client->getSocket();
	
	/* Check if this is the start of a new message: */
	if(continuation==0)
		{
		/* Prepare to read the color map updated request message: */
		continuation=protocolTypes.prepareReading(clientMessageTypes[ColorMapUpdatedRequest],new ColorMapUpdatedMsg);
		}
	
	/* Continue reading the color map updated request message and check whether it's complete: */
	if(protocolTypes.continueReading(socket,continuation))
		{
		/* Extract the connect reply message: */
		ColorMapUpdatedMsg* msg=protocolTypes.getReadObject<ColorMapUpdatedMsg>(continuation);
		
		/* Update the stored color map: */
		delete colorMaps[msg->scalarVariableIndex];
		colorMaps[msg->scalarVariableIndex]=new ColorMap(msg->colorMap);
		
		/* Forward the new color map to all other clients: */
		{
		/* Create a message writer: */
		MessageWriter message(MessageBuffer::create(serverMessageBase+ColorMapUpdatedNotification,protocolTypes.calcSize(serverMessageTypes[ColorMapUpdatedNotification],msg)));
		
		/* Write the message structure into the message: */
		protocolTypes.write(serverMessageTypes[ColorMapUpdatedNotification],msg,message);
		
		/* Broadcast the update notification to all other clients: */
		broadcastMessage(clientId,message.getBuffer());
		}
		
		/* Delete the color map updated request message and the continuation object: */
		delete msg;
		delete continuation;
		continuation=0;
		}
	
	return continuation;
	}

void SharedVisualizationServer::clearDataCommandCallback(const char* argumentBegin,const char* argumentEnd)
	{
	/* Release all color maps: */
	for(unsigned int i=0;i<numScalarVariables;++i)
		delete colorMaps[i];
	delete[] colorMaps;
	colorMaps=0;
	
	/* Reset the variable space: */
	numScalarVariables=0;
	numVectorVariables=0;
	}

SharedVisualizationServer::SharedVisualizationServer(Server* sServer)
	:PluginServer(sServer),
	 numScalarVariables(0),colorMaps(0),
	 numVectorVariables(0)
	{
	/* Register pipe commands: */
	server->getCommandDispatcher().addCommandCallback("3DVisualizer::clearData",Misc::CommandDispatcher::wrapMethod<SharedVisualizationServer,&SharedVisualizationServer::clearDataCommandCallback>,this,0,"Clears the dataset currently held by the server");
	}

SharedVisualizationServer::~SharedVisualizationServer(void)
	{
	/* Release all color maps: */
	for(unsigned int i=0;i<numScalarVariables;++i)
		delete colorMaps[i];
	delete[] colorMaps;
	
	/* Unregister pipe commands: */
	server->getCommandDispatcher().removeCommandCallback("3DVisualizer::clearData");
	}

const char* SharedVisualizationServer::getName(void) const
	{
	return protocolName;
	}

unsigned int SharedVisualizationServer::getVersion(void) const
	{
	return protocolVersion;
	}

unsigned int SharedVisualizationServer::getNumClientMessages(void) const
	{
	return NumClientMessages;
	}

unsigned int SharedVisualizationServer::getNumServerMessages(void) const
	{
	return NumServerMessages;
	}

void SharedVisualizationServer::setMessageBases(unsigned int newClientMessageBase,unsigned int newServerMessageBase)
	{
	/* Call the base class method: */
	PluginServer::setMessageBases(newClientMessageBase,newServerMessageBase);
	
	/* Register message handlers: */
	server->setMessageHandler(clientMessageBase+ConnectRequest,Server::wrapMethod<SharedVisualizationServer,&SharedVisualizationServer::connectRequestCallback>,this,getClientMsgSize(ConnectRequest));
	server->setMessageHandler(clientMessageBase+ColorMapUpdatedRequest,Server::wrapMethod<SharedVisualizationServer,&SharedVisualizationServer::colorMapUpdatedRequestCallback>,this,getClientMsgSize(ColorMapUpdatedRequest));
	}

void SharedVisualizationServer::start(void)
	{
	}

void SharedVisualizationServer::clientConnected(unsigned int clientId)
	{
	/* Add the new client to the list of clients using this protocol: */
	addClientToList(clientId);
	}

void SharedVisualizationServer::clientDisconnected(unsigned int clientId)
	{
	/* Remove the client from the list of clients using this protocol: */
	removeClientFromList(clientId);
	}

/***********************
DSO loader entry points:
***********************/

extern "C" {

PluginServer* createObject(PluginServerLoader& objectLoader,Server* server)
	{
	return new SharedVisualizationServer(server);
	}

void destroyObject(PluginServer* object)
	{
	delete object;
	}

}

}

}
