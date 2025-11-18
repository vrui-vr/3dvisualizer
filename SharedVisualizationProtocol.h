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

#ifndef SHAREDVISUALIZATIONPROTOCOL_INCLUDED
#define SHAREDVISUALIZATIONPROTOCOL_INCLUDED

#include <Misc/SizedTypes.h>
#include <Misc/RGBA.h>
#include <Misc/Vector.h>
#include <Collaboration2/DataType.h>
#include <Collaboration2/MessageBuffer.h>

namespace Collab {

namespace Plugins {

class SharedVisualizationProtocol
	{
	/* Embedded classes: */
	protected:
	
	/* Protocol message IDs: */
	enum ClientMessages // Enumerated type for protocol message IDs sent by clients
		{
		ConnectRequest=0,
		ColorMapUpdatedRequest,
		
		NumClientMessages
		};
	
	enum ServerMessages // Enumerated type for protocol message IDs sent by servers
		{
		ConnectReject=0,
		ConnectReply,
		ColorMapUpdatedNotification,
		
		NumServerMessages
		};
	
	/* Protocol data type declarations: */
	typedef Misc::UInt8 VariableIndex; // Type for scalar or vector variable indices
	typedef Misc::Float64 VariableValue; // Type for scalar variable values and vector variable components
	typedef Misc::RGBA<Misc::Float32> Color; // Type for color map colors
	
	struct ColorMapEntry // Type for color map entries
		{
		/* Elements: */
		public:
		VariableValue value; // Scalar value to which this entry applies
		Color color; // Color and opacity mapped to the scalar value
		};
	
	typedef Misc::Vector<ColorMapEntry> ColorMap; // Type for color maps
	
	/* Protocol message data structure declarations: */
	struct ConnectRequestMsg
		{
		/* Elements: */
		public:
		VariableIndex numScalarVariables,numVectorVariables; // Client's dataset variable layout, to check compatibility with dataset currently represented by server
		};
	
	struct ConnectReplyMsg
		{
		/* Embedded classes: */
		public:
		struct ColorMapListEntry
			{
			/* Elements: */
			public:
			VariableIndex scalarVariableIndex; // Index of the scalar variable to which the color map is applied
			ColorMap colorMap; // The color map
			
			/* Constructors and destructors: */
			ColorMapListEntry(VariableIndex sScalarVariableIndex,const ColorMap& sColorMap)
				:scalarVariableIndex(sScalarVariableIndex),colorMap(sColorMap)
				{
				}
			};
		
		/* Elements: */
		public:
		Misc::Vector<ColorMapListEntry> colorMaps; // List of scalar variable color maps already defined on the server
		};
	
	struct ColorMapUpdatedMsg
		{
		/* Elements: */
		public:
		VariableIndex scalarVariableIndex; // Index of the scalar variable to which the color map is applied
		ColorMap colorMap; // The new color map
		};
	
	/* Elements: */
	static const char* protocolName;
	static const unsigned int protocolVersion=(6U<<16)+0U;
	
	/* Protocol data type declarations: */
	DataType protocolTypes; // Definitions of data types used by the shared visualization protocol
	DataType::TypeID clientMessageTypes[NumClientMessages]; // Types for message structures sent by clients
	DataType::TypeID serverMessageTypes[NumServerMessages]; // Types for message structures sent by servers
	
	/* Constructors and destructors: */
	SharedVisualizationProtocol(void);
	
	/* Methods: */
	};

}

}

#endif
