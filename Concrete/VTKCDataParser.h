/***********************************************************************
VTKCDataParser - Class to parse ASCII-encoded numbers from XML character
data in a VTK data file.
Copyright (c) 2019 Oliver Kreylos

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

#ifndef VISUALIZATION_CONCRETE_VTKCDATAPARSER_INCLUDED
#define VISUALIZATION_CONCRETE_VTKCDATAPARSER_INCLUDED

#include <Misc/SizedTypes.h>
#include <IO/XMLSource.h>

namespace Visualization {

namespace Concrete {

class VTKCDataParser
	{
	/* Elements: */
	private:
	IO::XMLSource& xml; // XML file from which to read character data
	int lastChar; // Last character read from XML file
	
	/* Constructors and destructors: */
	public:
	VTKCDataParser(IO::XMLSource& sXml); // Creates a parser for the given XML file's current character data segment
	~VTKCDataParser(void); // Destroys parser and puts last read-ahead character back into the XML source
	
	/* Methods: */
	bool eocd(void) const // Returns true if the entire character data segment has been read
		{
		return lastChar<0;
		}
	void finish(void) // Skips to the end of the character data segment
		{
		while(lastChar>=0)
			lastChar=xml.readCharacterData();
		}
	void skipWs(bool mustHaveWhitespace =false); // Skips whitespace; throws exception if mustHaveWhitespace is true and there is no whitespace or EOF
	Misc::UInt64 readUnsignedInteger(void); // Reads an unsigned integer
	Misc::SInt64 readInteger(void); // Reads an integer
	Misc::Float64 readFloat(void); // Reads a floating-point number
	};

}

}

#endif
