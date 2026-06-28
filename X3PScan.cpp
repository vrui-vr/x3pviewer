/***********************************************************************
X3PScan - Class to represent a 3D scan read from an X3P file.
Copyright (c) 2026 Oliver Kreylos

This file is part of the X3P Scan Viewer (X3PViewer).

The X3P Scan Viewer is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The X3P Scan Viewer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the X3P Scan Viewer; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include "X3PScan.h"

#include <stdlib.h>
#include <errno.h>
#include <string>
#include <Misc/SizedTypes.h>
#include <Misc/StdError.h>
#include <IO/File.h>
#include <IO/ZipArchive.h>
#include <IO/XMLDocument.h>
#include <Math/Math.h>

/************************
Methods of class X3PScan:
************************/

X3PScan::X3PScan(void)
	:entries(0)
	{
	/* Initialize the array size: */
	size[1]=size[0]=0;
	}

X3PScan::X3PScan(unsigned int width,unsigned int height)
	:entries(new Entry[height*width])
	{
	/* Copy the array size: */
	size[0]=width;
	size[1]=height;
	}

X3PScan::X3PScan(X3PScan&& source)
	:entries(source.entries)
	{
	/* Copy the array size: */
	for(int i=0;i<2;++i)
		size[i]=source.size[i];
	
	/* Invalidate the source: */
	source.size[1]=source.size[0]=0;
	source.entries=0;
	}

X3PScan::~X3PScan(void)
	{
	delete[] entries;
	}

X3PScan& X3PScan::operator=(X3PScan&& source)
	{
	if(this!=&source)
		{
		/* Copy the source: */
		for(int i=0;i<2;++i)
			size[i]=source.size[i];
		entries=source.entries;
		
		/* Invalidate the source: */
		source.size[1]=source.size[0]=0;
		source.entries=0;
		}
	
	return *this;
	}

namespace {

/*************************************************
Helper structures and functions to read X3P files:
*************************************************/

struct X3P // Container structure
	{
	/* Embedded classes: */
	public:
	enum DataType // Enumerated type for scalar value types stored in binary point data files
		{
		Int32,Int64,Float32,Float64
		};
	
	struct Axis // Definition of a measurement axis
		{
		/* Elements: */
		public:
		bool incremental; // Flag if the axis's positions are implicit
		double offset,increment; // Offset and increment for implicit axes
		DataType dataType; // Type of data stored with the axis
		};
	};

const IO::XMLElement* getElement(const IO::XMLElement* current,const char* path)
	{
	/* Follow the path one component at the time from the document root: */
	const char* compStart=path;
	while(*compStart!='\0')
		{
		/* Find the end of the current path component: */
		const char* compEnd;
		for(compEnd=compStart;*compEnd!='\0'&&*compEnd!='/';++compEnd)
			;
		std::string compName(compStart,compEnd);
		
		/* Find the first sub-element whose name matches the current path component: */
		const IO::XMLElement* child=current->findNextElement(compName.c_str(),0);
		if(child==0)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Path component %s not found in XML element",compName.c_str());
		
		current=child;
		
		/* Find the beginning of the next path component: */
		compStart=compEnd;
		while(*compStart=='/')
			++compStart;
		}
	
	return current;
	}

const IO::XMLElement* getElement(const IO::XMLDocument& doc,const char* path)
	{
	/* Skip an initial sequence of slashes in the path: */
	while(*path=='/')
		++path;
	
	/* Get the element from the document root: */
	const IO::XMLElement* root=dynamic_cast<const IO::XMLElement*>(doc.getRoot());
	if(root==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"XML document's root is not an XML element");
	return getElement(root,path);
	}

const std::string& getCharacterData(const IO::XMLElement* current,const char* path)
	{
	/* Find the XML element indicated by the given path: */
	const IO::XMLElement* element=getElement(current,path);
	
	/* Find the first character data node under the found element: */
	const IO::XMLNode* chPtr=element->getChildren().front();
	const IO::XMLCharacterData* cdPtr=0;
	while(chPtr!=0&&cdPtr==0)
		{
		/* Check if the child is a character data node: */
		cdPtr=dynamic_cast<const IO::XMLCharacterData*>(chPtr);
		
		/* Go to the next child: */
		chPtr=chPtr->getSibling();
		}
	if(cdPtr==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"XML element does not contain character data");
	
	return cdPtr->getData();
	}

const std::string& getCharacterData(const IO::XMLDocument& doc,const char* path)
	{
	/* Skip an initial sequence of slashes in the path: */
	while(*path=='/')
		++path;
	
	/* Get the element from the document root: */
	const IO::XMLElement* root=dynamic_cast<const IO::XMLElement*>(doc.getRoot());
	if(root==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"XML document's root is not an XML element");
	return getCharacterData(root,path);
	}

double readCoordinateX3P(IO::File& file,X3P::DataType dataType) // Reads a binary coordinate component of the given data type
	{
	switch(dataType)
		{
		case X3P::Int32:
			return double(file.read<Misc::SInt32>());
		
		case X3P::Int64:
			return double(file.read<Misc::SInt64>());
		
		case X3P::Float32:
			return double(file.read<Misc::Float32>());
		
		case X3P::Float64:
			return double(file.read<Misc::Float64>());
		}
	
	/* Never reached; just to make compiler happy: */
	return 0.0;
	}

}

/***************************
Functions to read X3P files:
***************************/

X3PScan readX3PFile(const char* fileName)
	{
	/* Open the X3P file, which is actually a ZIP archive: */
	IO::ZipArchive x3pArchive(fileName);
	
	/* Read the central main.xml file into an XML document: */
	IO::XMLDocument main(*x3pArchive.openFile(x3pArchive.findFile("main.xml")));
	
	/* Access the Axes element: */
	const IO::XMLElement* axesElement=getElement(main,"/Record1/Axes");
	
	/* Read the measurement matrix description: */
	X3P::Axis axes[3];
	unsigned int numIncrementalAxes=0;
	for(int axisIndex=0;axisIndex<3;++axisIndex)
		{
		/* Go to the axis element: */
		char axisName[3]="C_";
		axisName[1]='X'+axisIndex;
		const IO::XMLElement* axis=getElement(axesElement,axisName);
		
		/* Parse the axis type: */
		const std::string& axisType=getCharacterData(axis,"AxisType");
		axes[axisIndex].incremental=axisType=="I";
		if(axes[axisIndex].incremental)
			++numIncrementalAxes;
		axes[axisIndex].offset=0.0;
		axes[axisIndex].increment=1.0;
		if(axes[axisIndex].incremental||axisType=="A")
			{
			char* endPtr;
			
			/* Parse the axis offset: */
			const std::string& offset=getCharacterData(axis,"Offset");
			errno=0;
			axes[axisIndex].offset=strtod(offset.c_str(),&endPtr);
			if(errno!=0||*endPtr!='\0'||endPtr==offset.c_str())
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid offset %s for %c axis",offset.c_str(),'X'+axisIndex);
			
			/* Parse the axis incremeent: */
			const std::string& increment=getCharacterData(axis,"Increment");
			errno=0;
			axes[axisIndex].increment=strtod(increment.c_str(),&endPtr);
			if(errno!=0||*endPtr!='\0'||endPtr==increment.c_str())
				throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid increment %s for %c axis",increment.c_str(),'X'+axisIndex);
			}
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid axis type %s for %c axis",axisType.c_str(),'X'+axisIndex);
		
		/* Parse the axis data type: */
		const std::string& dataType=getCharacterData(axis,"DataType");
		if(dataType=="I")
			axes[axisIndex].dataType=X3P::Int32;
		else if(dataType=="L")
			axes[axisIndex].dataType=X3P::Int64;
		else if(dataType=="F")
			axes[axisIndex].dataType=X3P::Float32;
		else if(dataType=="D")
			axes[axisIndex].dataType=X3P::Float64;
		else
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid data type %s for %c axis",dataType.c_str(),'X'+axisIndex);
		}
	
	if(numIncrementalAxes!=2)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"X3P file %s does not contain a 2D array",fileName);
	
	/* Access the MatrixDimension element: */
	const IO::XMLElement* matrixDimension=getElement(main,"/Record3/MatrixDimension");
	
	/* Parse the matrix dimensions: */
	unsigned int matrixSize[3];
	for(int axisIndex=0;axisIndex<3;++axisIndex)
		{
		/* Parse the matrix size: */
		char sizeName[6]="Size_";
		sizeName[4]='X'+axisIndex;
		const std::string& size=getCharacterData(matrixDimension,sizeName);
		char* endPtr;
		errno=0;
		matrixSize[axisIndex]=strtoul(size.c_str(),&endPtr,10);
		if(errno!=0||*endPtr!='\0'||endPtr==size.c_str())
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Invalid matrix size %s for %c axis",size.c_str(),'X'+axisIndex);
		
		/* Check that non-incremental axes only have a single element: */
		if(!axes[axisIndex].incremental&&matrixSize[axisIndex]!=1)
			throw Misc::makeStdErr(__PRETTY_FUNCTION__,"X3P file %s has invalid matrix layout",fileName);
		}
	
	/* Create a swizzle array to make the single non-incremental axis the Z axis: */
	int axisIndices[3];
	for(int axisIndex=0;axisIndex<3;++axisIndex)
		if(!axes[axisIndex].incremental)
			axisIndices[2]=axisIndex;
	axisIndices[0]=(axisIndices[2]+1)%3;
	axisIndices[1]=(axisIndices[2]+2)%3;
	
	/* Create a result structure: */
	X3PScan result(matrixSize[axisIndices[0]],matrixSize[axisIndices[1]]);
	ptrdiff_t height=result.getSize(1);
	
	/* Read point data from a binary point data file: */
	const std::string& pointDataLink=getCharacterData(main,"/Record3/DataLink/PointDataLink");
	IO::FilePtr pointData=x3pArchive.openFile(x3pArchive.findFile(pointDataLink.c_str()));
	pointData->setEndianness(Misc::LittleEndian);
	
	/* Read the value matrix and convert it to a point array: */
	unsigned int index[3];
	double p[3];
	p[2]=axes[2].offset;
	for(index[2]=0;index[2]<matrixSize[2];++index[2],p[2]+=axes[2].increment)
		{
		p[1]=axes[1].offset;
		for(index[1]=0;index[1]<matrixSize[1];++index[1],p[1]+=axes[1].increment)
			{
			p[0]=axes[0].offset;
			for(index[0]=0;index[0]<matrixSize[0];++index[0],p[0]+=axes[0].increment)
				{
				/* Read the point position: */
				bool valid=true;
				for(int i=0;i<3;++i)
					if(!axes[i].incremental)
						{
						p[i]=readCoordinateX3P(*pointData,axes[i].dataType)*axes[i].increment;
						valid=valid&&Math::isFinite(p[i]);
						}
				
				if(valid)
					{
					/* Swizzle and store the point position in the 2D array: */
					X3PScan::Point sp;
					for(int i=0;i<3;++i)
						sp[i]=X3PScan::Scalar(p[axisIndices[i]]);
					result.setPos(index[axisIndices[0]],index[axisIndices[1]],sp);
					}
				}
			}
		}
	
	/* Calculate normal vectors for all valid scan entries: */
	for(unsigned int y=0;y<result.getSize(1);++y)
		for(unsigned int x=0;x<result.getSize(0);++x)
			{
			X3PScan::Vector normal(0,0,1);
			
			/* Check if the scan entry is valid: */
			if(result.isValid(x,y))
				{
				/* Access the scan entry's position: */
				const X3PScan::Point& p=result.getPos(x,y);
				
				/* Calculate the horizontal normal vector component: */
				bool leftValid=x>0&&result.isValid(x-1,y);
				bool rightValid=x<result.getSize(0)-1&&result.isValid(x+1,y);
				if(leftValid&&rightValid)
					{
					X3PScan::Vector l=result.getPos(x-1,y)-p;
					X3PScan::Vector r=result.getPos(x+1,y)-p;
					normal[0]=(r[0]*r[0]*l[2]-l[0]*l[0]*r[2])/(l[0]*l[0]*r[0]-l[0]*r[0]*r[0]);
					}
				else if(leftValid)
					{
					X3PScan::Vector l=result.getPos(x-1,y)-p;
					if(x>1&&result.isValid(x-2,y))
						{
						X3PScan::Vector ll=result.getPos(x-2,y)-p;
						normal[0]=(ll[0]*ll[0]*l[0]-l[0]*l[0]*ll[2])/(l[0]*l[0]*ll[0]-l[0]*ll[0]*ll[0]);
						}
					else
						normal[0]=(p[2]-l[2])/(l[0]-p[0]);
					}
				else if(rightValid)
					{
					X3PScan::Vector r=result.getPos(x+1,y)-p;
					if(x<result.getSize(0)-2&&result.isValid(x+2,y))
						{
						X3PScan::Vector rr=result.getPos(x+2,y)-p;
						normal[0]=(rr[0]*rr[0]*r[0]-r[0]*r[0]*rr[2])/(r[0]*r[0]*rr[0]-r[0]*rr[0]*rr[0]);
						}
					else
						normal[0]=(p[2]-r[2])/(r[0]-p[0]);
					}
				
				/* Calculate the vertical normal vector component: */
				bool bottomValid=y>0&&result.isValid(x,y-1);
				bool topValid=y<result.getSize(1)-1&&result.isValid(x,y+1);
				if(bottomValid&&topValid)
					{
					X3PScan::Vector b=result.getPos(x,y-1)-p;
					X3PScan::Vector t=result.getPos(x,y+1)-p;
					normal[1]=(t[1]*t[1]*b[2]-b[1]*b[1]*t[2])/(b[1]*b[1]*t[1]-b[1]*t[1]*t[1]);
					}
				else if(bottomValid)
					{
					X3PScan::Vector b=result.getPos(x,y-1)-p;
					if(y>2&&result.isValid(x,y-2))
						{
						X3PScan::Vector bb=result.getPos(x,y-2)-p;
						normal[1]=(bb[1]*bb[1]*b[1]-b[1]*b[1]*bb[2])/(b[1]*b[1]*bb[1]-b[1]*bb[1]*bb[1]);
						}
					else
						normal[1]=(p[2]-b[2])/(b[1]-p[1]);
					}
				else if(topValid)
					{
					X3PScan::Vector t=result.getPos(x,y+1)-p;
					if(y<result.getSize(1)-2&&result.isValid(x,y+2))
						{
						X3PScan::Vector tt=result.getPos(x,y+2)-p;
						normal[1]=(tt[1]*tt[1]*t[1]-t[1]*t[1]*tt[2])/(t[1]*t[1]*tt[1]-t[1]*tt[1]*tt[1]);
						}
					else
						normal[1]=(p[2]-t[2])/(t[1]-p[1]);
					}
				}
			
			result.setNormal(x,y,normal);
			}
	
	return result;
	}
