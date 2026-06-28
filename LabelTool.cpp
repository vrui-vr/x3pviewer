/***********************************************************************
LabelTool - Definition of X3PViewer::LabelTool class.
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
#include <Geometry/Ray.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

/*************************************************
Static elements of class X3PViewer::LabelTool:
*************************************************/

Vrui::GenericToolFactory<X3PViewer::LabelTool>* X3PViewer::LabelTool::factory=0;

/*****************************************
Methods of class X3PViewer::LabelTool:
*****************************************/

X3PViewer::ScanPoint X3PViewer::LabelTool::calcScanPoint(void)
	{
	/* Get the button button slot's device ray in navigational coordinates: */
	Vrui::Ray ray=getButtonDeviceNavRay(0);
	
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

void X3PViewer::LabelTool::initClass(void)
	{
	/* Create a factory object for the labeling tool class: */
	factory=new Vrui::GenericToolFactory<LabelTool>("LabelTool","Label Character",0,*Vrui::getToolManager());
	
	/* Set the scan rotating tool class's input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Draw Box");
	
	/* Register the scan rotating tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

X3PViewer::LabelTool::LabelTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 active(false)
	{
	}

const Vrui::ToolFactory* X3PViewer::LabelTool::getFactory(void) const
	{
	return factory;
	}

void X3PViewer::LabelTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Check if another labeling is currently in progress: */
		if(!application->isLabeling)
			{
			/* Mark the tool as active: */
			application->isLabeling=true;
			active=true;
			
			/* Initialize the label box: */
			initial=calcScanPoint();
			box.max=box.min=initial;
			}
		}
	else // Button has just been released
		{
		/* Check if the tool is currently active: */
		if(active)
			{
			/* Add the current label box with the current scan transformation to the application's label list: */
			application->addCharacterLabel(box);
			
			/* Mark the tool as inactive: */
			active=false;
			}
		}
	}

void X3PViewer::LabelTool::frame(void)
	{
	/* Check if the tool is currently active: */
	if(active)
		{
		/* Calculate the second label box corner: */
		ScanPoint corner=calcScanPoint();
		
		/* Update the label box: */
		for(int i=0;i<2;++i)
			{
			box.min[i]=Math::min(initial[i],corner[i]);
			box.max[i]=Math::max(initial[i],corner[i]);
			}
		}
	}

void X3PViewer::LabelTool::display(GLContextData& contextData) const
	{
	/* Check if the tool is currently active: */
	if(active)
		{
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		
		/* Go to navigational coordinates: */
		Vrui::goToNavigationalSpace(contextData);
		
		/* Draw the current label box: */
		glLineWidth(3.0f);
		glBegin(GL_LINE_LOOP);
		glColor(Vrui::getBackgroundColor());
		glVertex3d(box.min[0],box.min[1],0.1);
		glVertex3d(box.max[0],box.min[1],0.1);
		glVertex3d(box.max[0],box.max[1],0.1);
		glVertex3d(box.min[0],box.max[1],0.1);
		glEnd();
		
		glLineWidth(1.0f);
		glBegin(GL_LINE_LOOP);
		glColor(Vrui::getForegroundColor());
		glVertex3d(box.min[0],box.min[1],0.1);
		glVertex3d(box.max[0],box.min[1],0.1);
		glVertex3d(box.max[0],box.max[1],0.1);
		glVertex3d(box.min[0],box.max[1],0.1);
		glEnd();
		
		/* Go back to physical space: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}
