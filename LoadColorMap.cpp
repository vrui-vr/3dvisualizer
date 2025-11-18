/***********************************************************************
LoadColorMap - Helper function to load color maps from text files.
Copyright (c) 2025 Oliver Kreylos

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

#include "LoadColorMap.h"

#include <vector>
#include <Misc/StdError.h>
#include <Misc/MessageLogger.h>
#include <IO/OpenFile.h>
#include <IO/ValueSource.h>

Misc::ColorMap<Misc::RGBA<float> >* createDefaultColorMap(const std::pair<double,double>& valueRange)
	{
	typedef Misc::ColorMap<Misc::RGBA<float> > ColorMap;
	typedef ColorMap::Entry Entry;
	
	/* Add the two grayscale color map entries: */
	std::vector<Entry> entries;
	entries.push_back(Entry(valueRange.first,ColorMap::Color(0.0f,0.0f,0.0f,0.0f)));
	entries.push_back(Entry(valueRange.second,ColorMap::Color(1.0f,1.0f,1.0f,1.0f)));
	
	return new ColorMap(entries);
	}

Misc::ColorMap<Misc::RGBA<float> >* loadColorMap(const char* fileName,const std::pair<double,double>& valueRange)
	{
	typedef Misc::ColorMap<Misc::RGBA<float> > ColorMap;
	typedef ColorMap::Entry Entry;
	
	/* Open the color map file of the given name and attach a value source to it: */
	IO::ValueSource file(IO::openFile(fileName));
	file.setPunctuation('#',true);
	file.setPunctuation('\n',true);
	
	/* Read the map file line-by-line: */
	std::vector<Entry> entries;
	while(!file.eof())
		{
		/* Skip empty and comment lines: */
		file.skipWs();
		if(file.peekc()!='#'&&file.peekc()!='\n')
			{
			Entry entry;
			
			/* Read the entry key: */
			entry.key=file.readNumber();
			
			/* Read the entry color: */
			for(int i=0;i<4;++i)
				entry.color[i]=file.readNumber();
			
			/* Check that we're at end-of-line: */
			if(!file.eof()&&file.peekc()!='#'&&file.peekc()!='\n')
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Extra text at end of line");
			
			entries.push_back(entry);
			
			/* Skip the rest of the line: */
			file.skipLine();
			}
		}
	
	/* Create the color map: */
	ColorMap* result=new ColorMap(entries);
	
	/* Adapt the color map to the given value range: */
	result->setRange(valueRange.first,valueRange.second);
	
	return result;
	}
