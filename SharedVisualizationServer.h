/***********************************************************************
SharedVisualizationServer - Server for collaborative data exploration in
spatially distributed VR environments, implemented as a plug-in of the
Vrui remote collaboration infrastructure.
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

#ifndef SHAREDVISUALIZATIONSERVER_INCLUDED
#define SHAREDVISUALIZATIONSERVER_INCLUDED

#include <Collaboration2/MessageBuffer.h>
#include <Collaboration2/MessageWriter.h>
#include <Collaboration2/Server.h>
#include <Collaboration2/PluginServer.h>

#include "SharedVisualizationProtocol.h"

/* Forward declarations: */
namespace Collab {
class MessageContinuation;
class Server;
}

namespace Collab {

namespace Plugins {

class SharedVisualizationServer:public PluginServer,public SharedVisualizationProtocol
	{
	/* Elements: */
	private:
	unsigned int numScalarVariables; // Number of scalar variables in the current dataset
	ColorMap** colorMaps; // Array of the current color map for each of the dataset's scalar variables, or 0 if the color map has not yet been defined
	unsigned int numVectorVariables; // Number of vector variables in the current dataset
	
	/* Private methods: */
	size_t getClientMsgSize(unsigned int messageId) const // Returns the minimum size of a client protocol message
		{
		return protocolTypes.getMinSize(clientMessageTypes[messageId]);
		}
	void sendClientMessage(unsigned int messageId,const void* messageStructure,Server::Client* client) // Writes a message structure for the given message ID into a buffer and queues it for delivery to the given client
		{
		/* Create a message writer: */
		MessageWriter message(MessageBuffer::create(serverMessageBase+messageId,protocolTypes.calcSize(serverMessageTypes[messageId],messageStructure)));
		
		/* Write the message structure into the message: */
		protocolTypes.write(serverMessageTypes[messageId],messageStructure,message);
		
		/* Queue the message for delivery to the given client: */
		client->queueMessage(message.getBuffer());
		}
	MessageContinuation* connectRequestCallback(unsigned int messageId,unsigned int clientId,MessageContinuation* continuation);
	
	/* Constructors and destructors: */
	public:
	SharedVisualizationServer(Server* sServer);
	virtual ~SharedVisualizationServer(void);
	
	/* Methods from class PluginServer: */
	virtual const char* getName(void) const;
	virtual unsigned int getVersion(void) const;
	virtual unsigned int getNumClientMessages(void) const;
	virtual unsigned int getNumServerMessages(void) const;
	virtual void setMessageBases(unsigned int newClientMessageBase,unsigned int newServerMessageBase);
	virtual void start(void);
	virtual void clientConnected(unsigned int clientId);
	virtual void clientDisconnected(unsigned int clientId);
	};

}

}

#endif
