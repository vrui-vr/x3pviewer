/***********************************************************************
LightingTool - Definition of X3PViewer::LightingTool class.
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
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/EnvironmentDefinition.h>
#include <Vrui/ToolManager.h>

/****************************************************
Static elements of class X3PViewer::LightingTool:
****************************************************/

Vrui::GenericToolFactory<X3PViewer::LightingTool>* X3PViewer::LightingTool::factory=0;

/********************************************
Methods of class X3PViewer::LightingTool:
********************************************/

void X3PViewer::LightingTool::initClass(void)
	{
	/* Create a factory object for the lighting tool class: */
	factory=new Vrui::GenericToolFactory<LightingTool>("LightingTool","Light Scan",0,*Vrui::getToolManager());
	
	/* Set the lighting tool class's input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Light");
	
	/* Register the lighting tool class with Vrui's tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

X3PViewer::LightingTool::LightingTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 active(false)
	{
	/* Calculate the light transformation: */
	lightTransform=Vrui::getEnvironmentDefinition().calcStandardFrame();
	lightTransform*=Vrui::ONTransform::rotate(Vrui::Rotation::rotateX(Math::rad(Vrui::Scalar(90))));
	lightTransform.doInvert();
	}

const Vrui::ToolFactory* X3PViewer::LightingTool::getFactory(void) const
	{
	return factory;
	}

void X3PViewer::LightingTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState) // Button has just been pressed
		{
		/* Mark the tool as active: */
		active=true;
		
		/* Set the display mode to lighting: */
		application->lighting=true;
		}
	else // Button has just been released
		{
		/* Set the display mode to normal vectors: */
		application->lighting=false;
		
		/* Mark the tool as inactive: */
		active=false;
		}
	}

void X3PViewer::LightingTool::frame(void)
	{
	/* Check if the tool is currently active: */
	if(active)
		{
		/* Set a default lighting direction: */
		application->lightDirection=Vrui::Vector(0,0,1);
		
		/* Get the button slot's device ray in "lighting space": */
		Vrui::Ray ray=getButtonDeviceRay(0);
		ray.transform(lightTransform);
		
		/* Intersect the "lighting space" device ray with the z==0 plane: */
		if(ray.getDirection()[2]*ray.getOrigin()[2]<Vrui::Scalar(0))
			{
			Vrui::Scalar lambda=-ray.getOrigin()[2]/ray.getDirection()[2];
			Vrui::Point lightPoint=ray(lambda);
			
			/* Calculate a light direction: */
			Vrui::Vector ld;
			Vrui::Scalar s=Vrui::getDisplaySize();
			ld[0]=lightPoint[0]/s;
			ld[1]=lightPoint[1]/s;
			Vrui::Scalar r2=ld[0]*ld[0]+ld[1]*ld[1];
			if(r2<Vrui::Scalar(1))
				ld[2]=Math::sqrt(Vrui::Scalar(1)-r2);
			else
				{
				Vrui::Scalar r=Math::sqrt(r2);
				ld[0]/=r;
				ld[1]/=r;
				ld[2]=Vrui::Scalar(0);
				}
			application->lightDirection=ld;
			}
		else
			{
			/* Assign a default light direction: */
			application->lightDirection=Vrui::Vector(0,0,1);
			}
		}
	}
