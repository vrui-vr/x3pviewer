/***********************************************************************
X3PViewer - Viewer for 3D scans stored in X3P format.
Copyright (c) 2025-2026 Oliver Kreylos

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

#ifndef X3PVIEWER_INCLUDED
#define X3PVIEWER_INCLUDED

#include <vector>
#include <Geometry/Box.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/GLShaderManager.h>
#include <Vrui/Types.h>
#include <Vrui/Application.h>
#include <Vrui/Tool.h>
#include <Vrui/GenericToolFactory.h>

#include "X3PScan.h"

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
class GLLabel;
namespace GLMotif {
class PopupWindow;
class TextField;
}

class X3PViewer:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	private:
	typedef Vrui::Scalar Scalar; // Scalar type for scan transformations
	typedef Geometry::OrthonormalTransformation<Scalar,2> ScanTransform; // Type for scan transformations in the 2D plane
	typedef ScanTransform::Point ScanPoint; // Type for points in the 2D plane
	typedef ScanTransform::Vector ScanVector; // Type for vectors in the 2D plane
	typedef ScanTransform::Rotation ScanRotation; // Type for rotations in the 2D plane
	typedef Geometry::Box<Scalar,2> ScanBox; // Type for axis-aligned boxes in the 2D plane
	
	struct CharacterLabel // Structure to label a rectangular area of a transformed scan as a character
		{
		/* Elements: */
		public:
		ScanTransform inverseScanTransform; // Inverse scan transformation at the time the label was created
		bool flip; // Flag if the scan was flipped at the time the label was created
		ScanBox box; // Label box in transformed scan space
		GLLabel* label; // Pointer to label object to display the character label
		};
	
	typedef std::vector<CharacterLabel> CharacterLabelList; // Type for lists of character labels
	
	class ScanRotateTool:public Vrui::Tool,public Vrui::Application::Tool<X3PViewer> // Tool class to rotate a scan in the 2D plane
		{
		friend class Vrui::GenericToolFactory<ScanRotateTool>; // Tool class uses the generic factory class
		
		/* Elements: */
		private:
		static Vrui::GenericToolFactory<ScanRotateTool>* factory; // Pointer to the factory object for this tool class
		int activeButtonSlot; // Index of the tool button slot currently operating, or -1 if idle
		ScanPoint center; // The center point of rotation
		ScanPoint drag; // The initially grabbed point
		
		/* Private methods: */
		ScanPoint calcScanPoint(int buttonSlotIndex); // Calculates the point in the 2D plane at which the input device associated with the given button slot is pointing
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initializes the scan rotating tool's factory class
		ScanRotateTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment); // Creates a scan rotating tool bound to the given input device features
		
		/* Methods: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		virtual void display(GLContextData& contextData) const;
		};
	
	class LightingTool:public Vrui::Tool,public Vrui::Application::Tool<X3PViewer> // Tool class to display the scan with Phong lighting
		{
		friend class Vrui::GenericToolFactory<LightingTool>; // Tool class uses the generic factory class
		
		/* Elements: */
		private:
		static Vrui::GenericToolFactory<LightingTool>* factory; // Pointer to the factory object for this tool class
		Vrui::ONTransform lightTransform; // Transformation from physical space to "lighting space" where the display center is the origin, x points right, and y points up
		bool active; // Flag if the tool is currently active
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initializes the lighting tool's factory class
		LightingTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment); // Creates a lighting tool bound to the given input device features
		
		/* Methods: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		};
	
	class LabelTool:public Vrui::Tool,public Vrui::Application::Tool<X3PViewer> // Tool class to create character labels
		{
		friend class Vrui::GenericToolFactory<LabelTool>; // Tool class uses the generic factory class
		
		/* Elements: */
		private:
		static Vrui::GenericToolFactory<LabelTool>* factory; // Pointer to the factory object for this tool class
		bool active; // Flag if the tool is currently active
		ScanTransform scanTransform; // The scan transformation at the time a new label was started
		ScanPoint initial; // The initially-selected label box corner
		ScanBox box; // The current label box
		
		/* Private methods: */
		ScanPoint calcScanPoint(void); // Calculates the point in scan space at which the input device is pointing
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initializes the lighting tool's factory class
		LabelTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment); // Creates a labeling tool bound to the given input device features
		
		/* Methods: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		virtual void display(GLContextData& contextData) const;
		};
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint textureId; // Texture object ID for the normal vector texture
		GLShaderManager::Namespace& shaderNamespace; // Namespace containing the normal vector texture shader
		
		/* Constructors and destructors: */
		DataItem(GLShaderManager::Namespace& sShaderNamespace);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	std::string scanFileName; // The name of the X3P file containing the scan
	X3PScan scan; // A 3D scan loaded from an X3P file
	ScanTransform scanTransform; // The current scan transformation
	bool flipScan; // Flag to flip the scan horizontally after the scan transformation is applied
	bool lighting; // Flag whether to use the lighting shader instead of the normal vector shader
	Vrui::Vector lightDirection; // Normalized light direction vector for the lighting shader
	bool isLabeling; // Flag if the application is currently creating a character label
	GLMotif::PopupWindow* characterEntryDialog; // Pointer to dialog window to enter a character label
	GLMotif::TextField* labelTextField;
	CharacterLabelList characterLabels; // Extracted character labels
	
	/* Private methods: */
	void characterEntryOkCallback(Misc::CallbackData* cbData);
	void characterEntryCancelCallback(Misc::CallbackData* cbData);
	void addCharacterLabel(const ScanBox& labelBox); // Adds a new character label with the given label box
	void exportCharacterImage(const CharacterLabel& cl,const char* imageFileName); // Exports the scan region associated with the given character label as an RGB-encoded image
	void saveLabels(const char* labelFileName); // Exports all character labels as a JSON metadata file with separate PNG images for each character label
	
	/* Constructors and destructors: */
	public:
	X3PViewer(int& argc,char**& argv);
	virtual ~X3PViewer(void);
	
	/* Methods from class Vrui::Application: */
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	virtual void eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData);
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

#endif
