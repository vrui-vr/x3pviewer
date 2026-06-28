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

#ifndef X3PSCAN_INCLUDED
#define X3PSCAN_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>

class X3PScan
	{
	/* Embedded classes: */
	public:
	typedef double Scalar; // Scalar type for points
	typedef Geometry::Point<Scalar,3> Point; // Type for 3D points
	typedef Geometry::Vector<Scalar,3> Vector; // Type for 3D vectors
	
	struct Entry // Structure representing an entry of a scanned array
		{
		/* Elements: */
		public:
		bool valid; // Flag if the entry is valid
		Point pos; // The entry's position
		Vector normal; // The entry's normal vector
		
		/* Constructors and destructors: */
		Entry(void) // Creates an invalid entry
			:valid(false)
			{
			}
		Entry(const Point& sPos) // Creates a valid entry from the given position
			:valid(true),pos(sPos)
			{
			}
		};
	
	/* Elements: */
	private:
	unsigned int size[2]; // Horizontal and vertical size of the 2D array
	Entry* entries; // The 2D array of scan entries
	
	/* Private methods: */
	void setPos(unsigned int x,unsigned int y,const Point& newPos) // Sets the scan entry of the given index to the given position and marks it as valid
		{
		Entry& e=entries[y*size[0]+x];
		e.valid=true;
		e.pos=newPos;
		}
	void setNormal(unsigned int x,unsigned int y,const Vector& newNormal) // Sets the normal vector of the given index
		{
		entries[y*size[0]+x].normal=newNormal;
		}
	
	/* Constructors and destructors: */
	public:
	X3PScan(void); // Creates an invalid scan
	X3PScan(unsigned int width,unsigned int height); // Constructs a 2D array of invalid points
	X3PScan(X3PScan&& source); // Move constructor
	~X3PScan(void); // Releases all resources
	
	/* Methods: */
	X3PScan& operator=(X3PScan&& source); // Move assignment operator
	const unsigned int* getSize(void) const // Returns the scan size as a 2D array
		{
		return size;
		}
	unsigned int getSize(int axis) const // Returns the scan size along the given axis
		{
		return size[axis];
		}
	const Entry& getEntry(unsigned int x,unsigned int y) const // Returns the scan entry of the given index
		{
		return entries[y*size[0]+x];
		}
	bool isValid(unsigned int x,unsigned int y) const // Returns the valid flag of the given index
		{
		return entries[y*size[0]+x].valid;
		}
	const Point& getPos(unsigned int x,unsigned int y) const // Returns the position of the given index
		{
		return entries[y*size[0]+x].pos;
		}
	const Vector& getNormal(unsigned int x,unsigned int y) const // Returns the normal vector of the given index
		{
		return entries[y*size[0]+x].normal;
		}
	
	/* Friend functions: */
	friend X3PScan readX3PFile(const char* fileName);
	};

X3PScan readX3PFile(const char* fileName); // Reads the X3P file of the given name and returns an X3P scan structure

#endif
