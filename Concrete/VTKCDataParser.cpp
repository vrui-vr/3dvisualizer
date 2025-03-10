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

#include <Concrete/VTKCDataParser.h>

#include <stdexcept>
#include <Math/Math.h>

namespace Visualization {

namespace Concrete {

/*******************************
Methods of class VTKCDataParser:
*******************************/

VTKCDataParser::VTKCDataParser(IO::XMLSource& sXml)
	:xml(sXml),
	 lastChar(xml.readCharacterData())
	{
	/* Skip initial whitespace: */
	skipWs();
	}

VTKCDataParser::~VTKCDataParser(void)
	{
	/* If the last character read wasn't EOF, put it back into the XML file: */
	if(lastChar>=0)
		xml.putback(lastChar);
	}

void VTKCDataParser::skipWs(bool mustHaveWhitespace)
	{
	/* Check if there is whitespace if requested: */
	if(mustHaveWhitespace&&lastChar>=0&&!IO::XMLSource::isSpace(lastChar))
		throw std::runtime_error("CharacterDataParser: Missing required whitespace");
	
	/* Skip whitespace: */
	while(IO::XMLSource::isSpace(lastChar))
		lastChar=xml.readCharacterData();
	}

Misc::UInt64 VTKCDataParser::readUnsignedInteger(void)
	{
	/* Read a sequence of digits: */
	bool haveDigits=false;
	Misc::UInt64 result(0);
	while(lastChar>='0'&&lastChar<='9')
		{
		haveDigits=true;
		result=result*10UL+Misc::UInt64(lastChar-'0');
		lastChar=xml.readCharacterData();
		}
	if(!haveDigits)
		throw std::runtime_error("CharacterDataParser::readUnsignedInteger: Invalid character");
	
	/* Skip required whitespace: */
	skipWs(true);
	
	return result;
	}

Misc::SInt64 VTKCDataParser::readInteger(void)
	{
	/* Read an optional sign: */
	bool negate=lastChar=='-';
	if(lastChar=='-'||lastChar=='+')
		lastChar=xml.readCharacterData();
	
	/* Read a sequence of digits: */
	bool haveDigits=false;
	Misc::SInt64 result(0);
	while(lastChar>='0'&&lastChar<='9')
		{
		haveDigits=true;
		result=result*10L+Misc::SInt64(lastChar-'0');
		lastChar=xml.readCharacterData();
		}
	if(!haveDigits)
		throw std::runtime_error("CharacterDataParser::readInteger: Invalid character");
	if(negate)
		result=-result;
	
	/* Skip required whitespace: */
	skipWs(true);
	
	return result;
	}

Misc::Float64 VTKCDataParser::readFloat(void)
	{
	/* Read a plus or minus sign: */
	bool negate=lastChar=='-';
	if(lastChar=='-'||lastChar=='+')
		lastChar=xml.readCharacterData();
	
	/* Read an integral number part: */
	bool haveDigit=false;
	Misc::Float64 result(0);
	while(lastChar>='0'&&lastChar<='9')
		{
		haveDigit=true;
		result=result*Misc::Float64(10)+Misc::Float64(lastChar-'0');
		lastChar=xml.readCharacterData();
		}
	
	/* Check for a period: */
	if(lastChar=='.')
		{
		lastChar=xml.readCharacterData();
		
		/* Read a fractional number part: */
		Misc::Float64 fraction(0);
		Misc::Float64 fractionBase(1);
		while(lastChar>='0'&&lastChar<='9')
			{
			haveDigit=true;
			fraction=fraction*Misc::Float64(10)+Misc::Float64(lastChar-'0');
			fractionBase*=Misc::Float64(10);
			lastChar=xml.readCharacterData();
			}
		
		result+=fraction/fractionBase;
		}
	
	/* Signal an error if no digits were read in the integral or fractional part: */
	if(!haveDigit)
		throw std::runtime_error("CharacterDataParser::readFloat: Invalid character");
	
	/* Negate the result if a minus sign was read: */
	if(negate)
		result=-result;
	
	/* Check for an exponent indicator: */
	if(lastChar=='e'||lastChar=='E')
		{
		lastChar=xml.readCharacterData();
		
		/* Read a plus or minus sign: */
		bool negateExponent=lastChar=='-';
		if(lastChar=='-'||lastChar=='+')
			lastChar=xml.readCharacterData();
		
		/* Check if there are any digits in the exponent: */
		if(lastChar<'0'||lastChar>'9')
			throw std::runtime_error("CharacterDataParser::readFloat: Invalid character");
		
		/* Read the exponent: */
		Misc::Float64 exponent(0);
		while(lastChar>='0'&&lastChar<='9')
			{
			exponent=exponent*Misc::Float64(10)+Misc::Float64(lastChar-'0');
			lastChar=xml.readCharacterData();
			}
		
		/* Multiply the mantissa with the exponent: */
		result*=Math::pow(Misc::Float64(10),negateExponent?-exponent:exponent);
		}
	
	/* Skip required whitespace: */
	skipWs(true);
	
	return result;
	}

}

}
