/***********************************************************************
ScanRotateTool - Definition of X3PViewer::ScanRotateTool class.
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

#include "X3PViewer.h"

#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Ray.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

/******************************************************
Static elements of class X3PViewer::ScanRotateTool:
******************************************************/

Vrui::GenericToolFactory<X3PViewer::ScanRotateTool>* X3PViewer::ScanRotateTool::factory=0;

/**********************************************
Methods of class X3PViewer::ScanRotateTool:
**********************************************/

X3PViewer::ScanPoint X3PViewer::ScanRotateTool::calcScanPoint(int buttonSlotIndex)
	{
	/* Get the button button slot's device ray in navigational coordinates: */
	Vrui::Ray ray=getButtonDeviceNavRay(buttonSlotIndex);
	
	/* Check if the ray is pointed at the z==0 plane: */
	if(ray.getOrigin()[2]*ray.getDirection()[2]<Vrui::Scalar(0))
		{
		/* Intersect the ray with the z==0 plane: */
		Vrui::Scalar lambda=-ray.getOrigin()[2]/ray.getDirection()[2];
		Vrui::Point intersection=ray(lambda);
		return ScanPoint(intersection[0],intersection[1]);
		}
	else
		{
		/* Return the scan's center point, for lack of a better option: */
		return ScanPoint(Math::div2(Scalar(application->scan.getSize(0))),Math::div2(Scalar(application->scan.getSize(1))));
		}
	}

void X3PViewer::ScanRotateTool::initClass(void)
	{
	/* Create a factory object for the scan rotating tool class: */
	factory=new Vrui::GenericToolFactory<ScanRotateTool>("ScanRotateTool","Rotate Scan",0,*Vrui::getToolManager());
	
	/* Set the scan rotating tool class's input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Rotate");
	
	/* Register the scan rotating tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

X3PViewer::ScanRotateTool::ScanRotateTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 activeButtonSlot(-1)
	{
	}

const Vrui::ToolFactory* X3PViewer::ScanRotateTool::getFactory(void) const
	{
	return factory;
	}

void X3PViewer::ScanRotateTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Mark the tool as active: */
		activeButtonSlot=buttonSlotIndex;
		
		/* Calculate the rotation center point: */
		Vrui::Point dCenter=Vrui::getNavigationTransformation().inverseTransform(Vrui::getDisplayCenter());
		for(int i=0;i<2;++i)
			center[i]=Scalar(dCenter[i]); // Use the display center as rotation center
		
		/* Calculate the initial grab point: */
		drag=calcScanPoint(activeButtonSlot);
		}
	else // Button has just been released
		{
		/* Mark the tool as inactive: */
		activeButtonSlot=-1;
		}
	}

void X3PViewer::ScanRotateTool::frame(void)
	{
	/* Check if the tool is currently active: */
	if(activeButtonSlot>=0)
		{
		/* Calculate the current grab point: */
		ScanPoint grab=calcScanPoint(activeButtonSlot);
		
		/* Calculate an incremental rotation if it is well-defined: */
		ScanVector v0=drag-center;
		ScanVector v1=grab-center;
		Scalar denom2=v0.sqr()*v1.sqr();
		if(denom2>Scalar(0))
			{
			/* Calculate the angle between the two vectors: */
			Scalar angle=Math::asin((v0[0]*v1[1]-v0[1]*v1[0])/Math::sqrt(denom2));
			if(v0*v1<Scalar(0))
				{
				if(angle>=Scalar(0))
					angle=Math::Constants<Scalar>::pi-angle;
				else
					angle=-Math::Constants<Scalar>::pi-angle;
				}
			
			/* Update the scan transformation: */
			application->scanTransform.leftMultiply(ScanTransform::rotateAround(center,ScanRotation(angle)));
			}
		
		/* Update the initial grab point: */
		drag=grab;
		}
	}

void X3PViewer::ScanRotateTool::display(GLContextData& contextData) const
	{
	/* Check if the tool is currently active: */
	if(activeButtonSlot>=0)
		{
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		
		/* Go to navigational coordinates: */
		Vrui::goToNavigationalSpace(contextData);
		
		/* Draw alignment guides around the current rotation center: */
		Scalar scanSize=Math::div2(Math::sqrt(Math::sqr(Scalar(application->scan.getSize(0)))+Math::sqr(Scalar(application->scan.getSize(1)))));
		glLineWidth(3.0f);
		glBegin(GL_LINES);
		glColor(Vrui::getBackgroundColor());
		glVertex3d(center[0]-scanSize,center[1],0.1);
		glVertex3d(center[0]+scanSize,center[1],0.1);
		glVertex3d(center[0],center[1]-scanSize,0.1);
		glVertex3d(center[0],center[1]+scanSize,0.1);
		glEnd();
		
		glLineWidth(1.0f);
		glBegin(GL_LINES);
		glColor(Vrui::getForegroundColor());
		glVertex3d(center[0]-scanSize,center[1],0.1);
		glVertex3d(center[0]+scanSize,center[1],0.1);
		glVertex3d(center[0],center[1]-scanSize,0.1);
		glVertex3d(center[0],center[1]+scanSize,0.1);
		glEnd();
		
		/* Go back to physical space: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}
