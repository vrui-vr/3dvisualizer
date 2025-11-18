/***********************************************************************
SharedVisualizationProtocol - Common interface between a shared
visualization server and a shared visualization client.
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

#include "SharedVisualizationProtocol.h"

#include <Collaboration2/DataType.icpp>

namespace Collab {

namespace Plugins {

/****************************************************
Static elements of class SharedVisualizationProtocol:
****************************************************/

const char* SharedVisualizationProtocol::protocolName="SharedVisualization";

/********************************************
Methods of class SharedVisualizationProtocol:
********************************************/

SharedVisualizationProtocol::SharedVisualizationProtocol(void)
	{
	/*********************************************************************
	Create protocol data types:
	*********************************************************************/
	
	/* ColorMap: */
	DataType::TypeID colorScalarType=DataType::getAtomicType<Color::Scalar>();
	DataType::TypeID colorType=protocolTypes.createFixedArray(Color::numComponents,colorScalarType);
	DataType::StructureElement colorMapEntryElements[]=
		{
		{DataType::getAtomicType<VariableValue>(),offsetof(ColorMapEntry,value)},
		{colorType,offsetof(ColorMapEntry,color)}
		};
	DataType::TypeID colorMapEntryType=protocolTypes.createStructure(2,colorMapEntryElements,sizeof(ColorMapEntry));
	DataType::TypeID colorMapType=protocolTypes.createVector(colorMapEntryType);
	
	/*********************************************************************
	Create data types for client protocol messages:
	*********************************************************************/
	
	/* ConnectRequestMsg: */
	DataType::StructureElement connectRequestMsgElements[]=
		{
		{DataType::getAtomicType<VariableIndex>(),offsetof(ConnectRequestMsg,numScalarVariables)},
		{DataType::getAtomicType<VariableIndex>(),offsetof(ConnectRequestMsg,numVectorVariables)}
		};
	clientMessageTypes[ConnectRequest]=protocolTypes.createStructure(2,connectRequestMsgElements,sizeof(ConnectRequestMsg));
	
	/*********************************************************************
	Create data types for server protocol messages:
	*********************************************************************/
	
	/* ConnectRejectMsg: */
	serverMessageTypes[ConnectReject]=0; // Connect reject message doesn't have a message body
	
	/* ConnectReplyMsg: */
	DataType::StructureElement colorMapListEntryElements[]=
		{
		{DataType::getAtomicType<VariableIndex>(),offsetof(ConnectReplyMsg::ColorMapListEntry,scalarVariableIndex)},
		{colorMapType,offsetof(ConnectReplyMsg::ColorMapListEntry,colorMap)}
		};
	DataType::TypeID colorMapListEntryType=protocolTypes.createStructure(2,colorMapListEntryElements,sizeof(ConnectReplyMsg::ColorMapListEntry));
	DataType::TypeID colorMapListType=protocolTypes.createVector(colorMapListEntryType);
	DataType::StructureElement connectReplyMsgElements[]=
		{
		{colorMapListType,offsetof(ConnectReplyMsg,colorMaps)}
		};
	serverMessageTypes[ConnectReply]=protocolTypes.createStructure(1,connectReplyMsgElements,sizeof(ConnectReplyMsg));
	
	/* ColorMapUpdatedMsg: */
	DataType::StructureElement colorMapUpdatedMsgElements[]=
		{
		{DataType::getAtomicType<VariableIndex>(),offsetof(ColorMapUpdatedMsg,scalarVariableIndex)},
		{colorMapType,offsetof(ColorMapUpdatedMsg,colorMap)}
		};
	DataType::TypeID colorMapUpdatedMsgType=protocolTypes.createStructure(2,colorMapUpdatedMsgElements,sizeof(ColorMapUpdatedMsg));
	clientMessageTypes[ColorMapUpdatedRequest]=colorMapUpdatedMsgType;
	serverMessageTypes[ColorMapUpdatedNotification]=colorMapUpdatedMsgType;
	}

}

}
