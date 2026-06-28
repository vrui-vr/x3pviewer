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

#include "X3PViewer.h"

#include <stdio.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <Misc/StdError.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/CommandLineParser.h>
#include <IO/OpenFile.h>
#include <IO/OStream.h>
#include <Math/Math.h>
#include <Geometry/Matrix.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/AffineTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <GL/Extensions/GLARBVertexShader.h>
#include <GL/Extensions/GLARBFragmentShader.h>
#include <GL/Extensions/GLARBMultitexture.h>
#include <GL/Extensions/GLARBTextureFloat.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLARBPixelBufferObject.h>
#include <GL/GLLightTracker.h>
#include <GL/GLLabel.h>
#include <GL/GLGeometryWrappers.h>
#include <Images/RGBImage.h>
#include <Images/WriteImageFile.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/TextField.h>
#include <Vrui/Vrui.h>

#include "Config.h"

/****************************************
Methods of class X3PViewer::DataItem:
****************************************/

X3PViewer::DataItem::DataItem(GLShaderManager::Namespace& sShaderNamespace)
	:textureId(0),
	 shaderNamespace(sShaderNamespace)
	{
	/* Initialize required OpenGL extensions: */
	GLARBMultitexture::initExtension();
	GLARBTextureFloat::initExtension();
	GLARBShaderObjects::initExtension();
	GLARBVertexShader::initExtension();
	GLARBFragmentShader::initExtension();
	
	/* Create the texture object: */
	glGenTextures(1,&textureId);
	}

X3PViewer::DataItem::~DataItem(void)
	{
	/* Destroy the texture object: */
	glDeleteTextures(1,&textureId);
	}

/******************************
Methods of class X3PViewer:
******************************/

void X3PViewer::characterEntryOkCallback(Misc::CallbackData* cbData)
	{
	/* Check if a character was entered: */
	const char* label=labelTextField->getString();
	if(label[0]!='\0'&&label[1]=='\0')
		{
		CharacterLabel& cl=characterLabels.back();
		
		/* Update the just-created label: */
		GLLabel* l=new GLLabel(label,*Vrui::getUiFont());
		l->setBackground(Vrui::getBackgroundColor());
		l->setForeground(Vrui::getForegroundColor());
		GLLabel::Box::Vector labelSize=l->getLabelSize();
		GLLabel::Box::Vector labelOrigin;
		labelOrigin[0]=-Math::div2(labelSize[0]);
		labelOrigin[1]=-Math::div2(labelSize[1]);
		labelOrigin[2]=0;
		l->setOrigin(labelOrigin);
		cl.label=l;
		}
	else
		{
		/* Delete the just-created label: */
		characterLabels.pop_back();
		}
	
	/* Finish the label creation process: */
	Vrui::popdownPrimaryWidget(characterEntryDialog);
	isLabeling=false;
	}

void X3PViewer::characterEntryCancelCallback(Misc::CallbackData* cbData)
	{
	/* Delete the just-created label: */
	characterLabels.pop_back();
	
	/* Finish the label creation process: */
	Vrui::popdownPrimaryWidget(characterEntryDialog);
	isLabeling=false;
	}

void X3PViewer::addCharacterLabel(const X3PViewer::ScanBox& labelBox)
	{
	/* Create a new label structure with the current scan transformation: */
	CharacterLabel cl;
	cl.inverseScanTransform=Geometry::invert(scanTransform);
	cl.flip=flipScan;
	cl.box=labelBox;
	cl.label=0;
	characterLabels.push_back(cl);
	
	/* Ask the user to enter a character label: */
	labelTextField->setString("");
	Vrui::popupPrimaryWidget(characterEntryDialog);
	if(Vrui::getWidgetManager()->requestFocus(labelTextField))
		labelTextField->giveTextFocus();
	}

#if 1

namespace {

/****************
Helper functions:
****************/

inline Images::RGBImage::Scalar mapNormalComponent(X3PScan::Scalar nc)
	{
	X3PScan::Scalar cc=nc+X3PScan::Scalar(0.5);
	
	/* Gamma correction: */
	if(cc<=X3PScan::Scalar(0.0031308))
		cc=cc*X3PScan::Scalar(12.92);
	else
		cc=X3PScan::Scalar(1.055)*Math::pow(cc,X3PScan::Scalar(1)/X3PScan::Scalar(2.4))-X3PScan::Scalar(0.055);
	
	/* Quantization: */
	if(cc<X3PScan::Scalar(0))
		return Images::RGBImage::Scalar(0);
	else if(cc>=X3PScan::Scalar(1))
		return Images::RGBImage::Scalar(255);
	else
		return Images::RGBImage::Scalar(Math::floor(cc*X3PScan::Scalar(256)));
	}

}

#endif

void X3PViewer::exportCharacterImage(const X3PViewer::CharacterLabel& cl,const char* imageFileName)
	{
	/* Sample the label's rectangle at the same resolution as the scan: */
	int min[2],max[2];
	for(int i=0;i<2;++i)
		{
		min[i]=int(Math::floor(cl.box.min[i]));
		max[i]=int(Math::floor(cl.box.max[i]));
		}
	Images::RGBImage image(Images::Size((max[0]-min[0])+1,(max[1]-min[1])+1));
	
	/* Calculate the scan sampling transformation: */
	Geometry::AffineTransformation<Scalar,2> sampleTransform(cl.inverseScanTransform);
	if(cl.flip)
		{
		Geometry::AffineTransformation<Scalar,2> flipTransform=Geometry::AffineTransformation<Scalar,2>::identity;
		Geometry::AffineTransformation<Scalar,2>::Matrix& ftm=flipTransform.getMatrix();
		ftm(0,0)=Scalar(-1);
		ftm(0,2)=Scalar(scan.getSize(0));
		sampleTransform.leftMultiply(flipTransform);
		}
	
	/* Calculate the normal vector transformation: */
	Geometry::AffineTransformation<X3PScan::Scalar,3> normalTransform=Geometry::AffineTransformation<X3PScan::Scalar,3>::identity;
	ScanRotation normalRot=Geometry::invert(cl.inverseScanTransform.getRotation());
	normalRot.writeMatrix(normalTransform.getMatrix());
	if(cl.flip)
		{
		Geometry::AffineTransformation<X3PScan::Scalar,3> flipTransform=Geometry::AffineTransformation<X3PScan::Scalar,3>::identity;
		Geometry::AffineTransformation<X3PScan::Scalar,3>::Matrix& ftm=flipTransform.getMatrix();
		ftm(0,0)=-1;
		normalTransform*=flipTransform;
		}
	
	/* Sample the label's rectangle: */
	Images::RGBImage::Color* iPtr=static_cast<Images::RGBImage::Color*>(image.replacePixels());
	ScanPoint boxp;
	boxp[1]=Scalar(min[1])+Scalar(0.5);
	for(int y=min[1];y<=max[1];++y,boxp[1]+=Scalar(1))
		{
		boxp[0]=Scalar(min[0])+Scalar(0.5);
		for(int x=min[0];x<=max[0];++x,boxp[0]+=Scalar(1),++iPtr)
			{
			/* Sample the scan at the transformed box position: */
			ScanPoint scanp=sampleTransform.transform(boxp);
			X3PScan::Vector normal(0,0,1);
			
			#if 0 // Nearest-neighbor interpolation
			
			int scanx=int(Math::floor(scanp[0]));
			int scany=int(Math::floor(scanp[1]));
			if(scanx>=0&&scanx<int(scan.getSize(0))&&scany>=0&&scany<int(scan.getSize(1)))
				normal=normalTransform.transform(scan.getEntry(scanx,scany).normal);
			
			#else // Bilinear interpolation
			
			int x0=Math::floor(scanp[0]-Scalar(0.5));
			int x1=x0+1;
			x0=Math::clamp(x0,0,int(scan.getSize(0))-1);
			x1=Math::clamp(x1,0,int(scan.getSize(0))-1);
			X3PScan::Scalar dx=scanp[0]-X3PScan::Scalar(x0);
			int y0=Math::floor(scanp[1]-Scalar(0.5));
			int y1=y0+1;
			y0=Math::clamp(y0,0,int(scan.getSize(1))-1);
			y1=Math::clamp(y1,0,int(scan.getSize(1))-1);
			X3PScan::Scalar dy=scanp[1]-X3PScan::Scalar(y0);
			
			const X3PScan::Vector& n0=scan.getEntry(x0,y0).normal;
			const X3PScan::Vector& n1=scan.getEntry(x1,y0).normal;
			const X3PScan::Vector& n2=scan.getEntry(x0,y1).normal;
			const X3PScan::Vector& n3=scan.getEntry(x1,y1).normal;
			normal=(n0*(X3PScan::Scalar(1)-dx)+n1*dx)*(X3PScan::Scalar(1)-dy)+(n2*(X3PScan::Scalar(1)-dx)+n3*dx)*dy;
			
			#endif
			
			/* RGB-encode the normal vector: */
			X3PScan::Scalar s=X3PScan::Scalar(1.0)/X3PScan::Scalar(normal.mag());
			for(int i=0;i<3;++i)
				(*iPtr)[i]=mapNormalComponent(normal[i]*s);
			}
		}
	
	Images::writeImageFile(image,imageFileName);
	}

void X3PViewer::saveLabels(const char* labelFileName)
	{
	/* Create a file and save all defined character labels as a JSON array: */
	IO::OStream labelFile(IO::openFile(labelFileName,IO::File::WriteOnly));
	labelFile<<'['<<std::endl;
	
	/* Get the label file's base name: */
	ptrdiff_t pathLen=Misc::getFileName(labelFileName)-labelFileName;
	std::string labelFileBaseName(labelFileName,Misc::getExtension(labelFileName));
	
	unsigned int index=0;
	for(CharacterLabelList::iterator clIt=characterLabels.begin();clIt!=characterLabels.end();++clIt,++index)
		{
		/* If this isn't the first label, terminate the previous one: */
		if(clIt!=characterLabels.begin())
			labelFile<<','<<std::endl;
		
		labelFile<<"\t{"<<std::endl;
		
		/* Save the label as an image: */
		char imageFileName[1024];
		snprintf(imageFileName,sizeof(imageFileName),"%s-Character%03u.png",labelFileBaseName.c_str(),index);
		exportCharacterImage(*clIt,imageFileName);
		
		/* Save the character and the label image name: */
		labelFile<<"\t\t\"label\":\""<<clIt->label->getString()<<"\","<<std::endl;
		labelFile<<"\t\t\"imageName\":\""<<imageFileName+pathLen<<"\","<<std::endl;
		
		/* Save the extraction data: */
		const ScanVector& t=clIt->inverseScanTransform.getTranslation();
		labelFile<<"\t\t\"inverseScanTranslation\":["<<t[0]<<','<<t[1]<<"],"<<std::endl;
		labelFile<<"\t\t\"inverseScanRotation\":["<<clIt->inverseScanTransform.getRotation().getAngle()<<"],"<<std::endl;
		labelFile<<"\t\t\"scanFlipped\":"<<(clIt->flip?"true":"false")<<","<<std::endl;
		labelFile<<"\t\t\"labelBox\":["<<clIt->box.min[0]<<','<<clIt->box.min[1]<<','<<clIt->box.max[0]<<','<<clIt->box.max[1]<<"]"<<std::endl;
		
		labelFile<<"\t}";
		}
	if(!characterLabels.empty())
		labelFile<<std::endl;
	
	labelFile<<']'<<std::endl;
	}

X3PViewer::X3PViewer(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 scanTransform(ScanTransform::identity),flipScan(false),
	 lighting(false),
	 isLabeling(false)
	{
	/* Parse the command line: */
	Misc::CommandLineParser cmd;
	cmd.setDescription("Utility to view 3D scans in X3P format as normal vector images");
	cmd.setArguments("<X3P file name>","Name of the input X3P file");
	std::vector<std::string> arguments;
	cmd.addArgumentsToList(arguments);
	bool use3d=false;
	cmd.addEnableOption("use3d","3d",use3d,"Encode all three normalized normal vector components");
	X3PScan::Scalar normalScale(0.5);
	cmd.addValueOption("normalScale","ns",normalScale,"<normal scale>","Scale factor for normal vector x and y components");
	cmd.parse(argv,argv+argc);
	if(cmd.hadHelp())
		{
		Vrui::shutdown();
		return;
		}
	if(arguments.size()<1)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No input X3P file given");
	else if(arguments.size()>1)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Too many arguments");
	
	/* Load the point array contained in the given X3P file: */
	scanFileName=arguments.front();
	scan=readX3PFile(scanFileName.c_str());
	
	#if 0
	
	/* Export the normal vector array as an image: */
	Images::RGBImage image(Images::Size(scan.getSize(0),scan.getSize(1)));
	Images::RGBImage::Color* cPtr=image.replacePixels();
	for(unsigned int y=0;y<scan.getSize(1);++y)
		for(unsigned int x=0;x<scan.getSize(0);++x,++cPtr)
			{
			/* Retrieve and scale or normalize the normal vector: */
			X3PScan::Vector normal=scan.getNormal(x,y);
			if(use3d)
				{
				/* Normalize the normal vector for 3D encoding: */
				normal.normalize();
				}
			else
				{
				/* Scale the normal vector for 2D encoding: */
				normal[0]*=normalScale;
				normal[1]*=normalScale;
				normal[2]=X3PScan::Scalar(0);
				}
			
			/* Encode the normal vector's x, y, and z components as red, green, and blue, respectively: */
			for(int i=0;i<3;++i)
				(*cPtr)[i]=mapNormalComponent(normal[i]);
			}
	
	/* Save the encoded normal image: */
	Images::writeImageFile(image,arguments[1].c_str());
	
	#endif
	
	/* Create the character entry dialog: */
	characterEntryDialog=new GLMotif::PopupWindow("CharacterEntryDialog",Vrui::getWidgetManager(),"Character Label");
	characterEntryDialog->setHideButton(false);
	characterEntryDialog->setCloseButton(false);
	characterEntryDialog->setResizableFlags(false,false);
	
	GLMotif::RowColumn* dialog=new GLMotif::RowColumn("Dialog",characterEntryDialog,false);
	dialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	dialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	GLMotif::RowColumn* labelBox=new GLMotif::RowColumn("LabelBox",dialog,false);
	labelBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	labelBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	new GLMotif::Label("LabelLabel",labelBox,"Label");
	
	labelTextField=new GLMotif::TextField("LabelTextField",labelBox,3);
	labelTextField->setValueType(GLMotif::TextField::ALPHA);
	labelTextField->setEditable(true);
	
	labelBox->manageChild();
	
	GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",dialog,false);
	buttonMargin->setAlignment(GLMotif::Alignment::RIGHT);
	
	GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",buttonMargin,false);
	buttonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	buttonBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	
	GLMotif::Button* okButton=new GLMotif::Button("OkButton",buttonBox,"OK");
	okButton->getSelectCallbacks().add(this,&X3PViewer::characterEntryOkCallback);
	
	GLMotif::Button* cancelButton=new GLMotif::Button("CancelButton",buttonBox,"Cancel");
	cancelButton->getSelectCallbacks().add(this,&X3PViewer::characterEntryCancelCallback);
	
	buttonBox->manageChild();
	
	buttonMargin->manageChild();
	
	dialog->manageChild();
	
	/* Initialize application tool classes: */
	ScanRotateTool::initClass();
	addEventTool("Flip Horizontally",0,0);
	LightingTool::initClass();
	LabelTool::initClass();
	addEventTool("Delete Label",0,1);
	}

X3PViewer::~X3PViewer(void)
	{
	/* Delete UI components: */
	delete characterEntryDialog;
	
	/* Check if there are any character labels: */
	if(!characterLabels.empty())
		{
		/* Generate the name of the save file: */
		std::string labelFileName(scanFileName.c_str(),Misc::getExtension(scanFileName.c_str()));
		labelFileName.append("-Labels.json");
		
		/* Save all character labels: */
		saveLabels(labelFileName.c_str());
		
		/* Delete all character labels: */
		for(CharacterLabelList::iterator clIt=characterLabels.begin();clIt!=characterLabels.end();++clIt)
			delete clIt->label;
		}
	}

void X3PViewer::display(GLContextData& contextData) const
	{
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POLYGON_BIT);
	bool lightingWasEnabled=contextData.getLightTracker()->isLightingEnabled();
	contextData.getLightTracker()->setLightingEnabled(false);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	
	/* Apply the scan transformation: */
	glPushMatrix();
	glTranslate(scanTransform.getTranslation());
	glRotate(scanTransform.getRotation());
	
	/* Install the normal vector or lighting shader: */
	unsigned int shaderIndex=lighting?1:0;
	dataItem->shaderNamespace.useShader(shaderIndex);
	
	/* Bind the scan texture: */
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D,dataItem->textureId);
	dataItem->shaderNamespace.uniform(shaderIndex,0,0);
	
	/* Upload the normal vector transformation matrix: */
	Geometry::Matrix<GLfloat,3,3> nvm(1);
	scanTransform.getRotation().writeMatrix(nvm);
	if(flipScan)
		for(int i=0;i<3;++i)
			nvm(i,0)=-nvm(i,0);
	dataItem->shaderNamespace.uniformMatrix3fv(shaderIndex,1,1,GL_TRUE,nvm.getEntries());
	
	if(lighting)
		{
		/* Upload the light direction: */
		Geometry::Vector<GLfloat,3> ld(lightDirection);
		dataItem->shaderNamespace.uniform3fv(shaderIndex,2,1,ld.getComponents());
		}
	
	/* Draw the scan: */
	GLfloat x0=flipScan?1.0f:0.0f;
	GLfloat x1=1.0f-x0;
	glBegin(GL_QUADS);
	glTexCoord2f(x0,0.0f);
	glVertex2i(0,0);
	glTexCoord2f(x1,0.0f);
	glVertex2i(scan.getSize(0),0);
	glTexCoord2f(x1,1.0f);
	glVertex2i(scan.getSize(0),scan.getSize(1));
	glTexCoord2f(x0,1.0f);
	glVertex2i(0,scan.getSize(1));
	glEnd();
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Disable the normal vector shader: */
	dataItem->shaderNamespace.disableShaders();
	
	/* Draw all character labels: */
	for(CharacterLabelList::const_iterator clIt=characterLabels.begin();clIt!=characterLabels.end();++clIt)
		{
		/* Go to label space: */
		glPushMatrix();
		glTranslate(clIt->inverseScanTransform.getTranslation());
		glRotate(clIt->inverseScanTransform.getRotation());
		
		/* Draw the label box: */
		const ScanBox& box=clIt->box;
		
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
		
		if(clIt->label!=0)
			{
			/* Draw the character label's label: */
			glTranslated(Math::mid(box.min[0],box.max[0]),Math::mid(box.min[1],box.max[1]),0.1);
			Vrui::Scalar s=Vrui::getInverseNavigationTransformation().getScaling()*Vrui::Scalar(2);
			glScaled(s,s,s);
			clIt->label->draw(contextData);
			}
		
		glPopMatrix();
		}
	
	/* Go back to navigational space: */
	glPopMatrix();
	
	/* Restore OpenGL state: */
	contextData.getLightTracker()->setLightingEnabled(lightingWasEnabled);
	glPopAttrib();
	}

void X3PViewer::resetNavigation(void)
	{
	/* Reset Vrui's navigation transformation: */
	Vrui::Scalar w(scan.getSize(0));
	Vrui::Scalar h(scan.getSize(1));
	Vrui::Point center(Math::div2(w),Math::div2(h),Vrui::Scalar(0.1));
	Vrui::Scalar size=Math::sqrt(Math::sqr(w)+Math::sqr(h));
	Vrui::setNavigationTransformation(center,size,Vrui::Vector(0,1,0),Vrui::Vector(1,0,0));
	
	/* Reset the scan transformation: */
	scanTransform=ScanTransform::identity;
	flipScan=false;
	}

void X3PViewer::eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	/* Check if the tool button was just pressed: */
	if(cbData->newButtonState)
		{
		switch(eventId)
			{
			case 0: // Flip the scan horizontally
				{
				/***************************************************************
				Flipping the scan horizontally in scan space will change the
				display rotation of the scan, but we don't want that, so we'll
				fix the scan transformation to make up for it. Essentially,
				we're converting a horizontal flip in display space to a
				horizontal flip and a rotation/translation in scan space.
				***************************************************************/
				
				/* Adjust the scan transformation to keep the rotated y axis fixed: */
				ScanTransform newScanTransform=scanTransform;
				ScanVector y=newScanTransform.inverseTransform(ScanVector(0,1));
				Scalar alpha=Scalar(2)*Math::atan2(-y[0],y[1]); // Angle between the rotated y axis and its flipped version
				newScanTransform*=ScanTransform::rotate(ScanRotation(alpha));
				
				/* Calculate the flipping center point: */
				Vrui::Point dCenter=Vrui::getNavigationTransformation().inverseTransform(Vrui::getDisplayCenter());
				ScanPoint center;
				for(int i=0;i<2;++i)
					center[i]=Scalar(dCenter[i]); // Use the display center as flipping center
				ScanPoint scanCenter=scanTransform.inverseTransform(center);
				
				/* Calculate the post-flipping center point: */
				ScanPoint scanCenterp(Scalar(scan.getSize(0))-scanCenter[0],scanCenter[1]);
				ScanPoint centerp=newScanTransform.transform(scanCenterp);
				
				/* Adjust the scan transformation to keep the flipping center fixed: */
				newScanTransform.leftMultiply(ScanTransform::translate(center-centerp));
				
				/* Apply the new scan transformation: */
				scanTransform=newScanTransform;
				flipScan=!flipScan;
				
				break;
				}
			
			case 1: // Delete the most recent label
				if(!characterLabels.empty())
					{
					delete characterLabels.back().label;
					characterLabels.pop_back();
					}
				
				break;
			}
		}
	else
		{
		}
	}

void X3PViewer::initContext(GLContextData& contextData) const
	{
	/* Register a shader namespace for the normal vector and lighting shaders: */
	unsigned int numShaderUniforms[2]={2,3};
	unsigned int numShaderVersionNumbers[2]={0,0};
	std::pair<GLShaderManager::Namespace&,bool> cnsr=contextData.getShaderManager()->createNamespace("X3PViewer",2,numShaderUniforms,numShaderVersionNumbers);
	
	/******************************
	Build the normal vector shader:
	******************************/
	
	{
	/* Compile the normal vector shader's vertex shader: */
	GLhandleARB vertexShader=glCompileVertexShaderFromFile(X3PVIEWER_CONFIG_SHAREDIR "/Shaders/NormalVectorShader.vs");
	
	/* Compile the normal vector shader's fragment shader: */
	GLhandleARB fragmentShader=glCompileFragmentShaderFromFile(X3PVIEWER_CONFIG_SHAREDIR "/Shaders/NormalVectorShader.fs");
	
	/* Link the shader program: */
	GLhandleARB shader=glCreateProgramObjectARB();
	glAttachObjectARB(shader,vertexShader);
	glAttachObjectARB(shader,fragmentShader);
	glLinkAndTestShader(shader);
	
	/* Release extra references for the vertex and fragment shaders: */
	glDeleteObjectARB(vertexShader);
	glDeleteObjectARB(fragmentShader);
	
	/* Store the shader program in the namespace: */
	cnsr.first.setShader(0,shader);
	
	/* Query the locations of the shader's uniform variables: */
	cnsr.first.setUniformLocation(0,0,"normalVectorTexture");
	cnsr.first.setUniformLocation(0,1,"normalVectorMatrix");
	}
	
	/*************************
	Build the lighting shader:
	*************************/
	
	{
	/* Compile the lighting shader's vertex shader: */
	GLhandleARB vertexShader=glCompileVertexShaderFromFile(X3PVIEWER_CONFIG_SHAREDIR "/Shaders/LightingShader.vs");
	
	/* Compile the lighting shader's fragment shader: */
	GLhandleARB fragmentShader=glCompileFragmentShaderFromFile(X3PVIEWER_CONFIG_SHAREDIR "/Shaders/LightingShader.fs");
	
	/* Link the shader program: */
	GLhandleARB shader=glCreateProgramObjectARB();
	glAttachObjectARB(shader,vertexShader);
	glAttachObjectARB(shader,fragmentShader);
	glLinkAndTestShader(shader);
	
	/* Release extra references for the vertex and fragment shaders: */
	glDeleteObjectARB(vertexShader);
	glDeleteObjectARB(fragmentShader);
	
	/* Store the shader program in the namespace: */
	cnsr.first.setShader(1,shader);
	
	/* Query the locations of the shader's uniform variables: */
	cnsr.first.setUniformLocation(1,0,"normalVectorTexture");
	cnsr.first.setUniformLocation(1,1,"normalVectorMatrix");
	cnsr.first.setUniformLocation(1,2,"lightDirection");
	}
	
	/* Create a data item and associate it with this object in the given OpenGL context: */
	DataItem* dataItem=new DataItem(cnsr.first);
	contextData.addDataItem(this,dataItem);
	
	/* Initialize required OpenGL extensions: */
	GLARBVertexBufferObject::initExtension();
	GLARBPixelBufferObject::initExtension();
	
	/* Initialize the normal vector texture object to hold RGB colors with floating-point components: */
	glBindTexture(GL_TEXTURE_2D,dataItem->textureId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB32F_ARB,scan.getSize(0),scan.getSize(1),0,GL_RGB,GL_FLOAT,0);
	
	/* Create a temporary pixel buffer object to upload the normal vector array into the texture: */
	GLuint pixelBufferId;
	glGenBuffersARB(1,&pixelBufferId);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,pixelBufferId);
	glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,scan.getSize(1)*scan.getSize(0)*3*sizeof(GLfloat),0,GL_STREAM_DRAW_ARB);
	
	/* Copy the normal vector array into the pixel buffer object: */
	GLfloat* pbPtr=static_cast<GLfloat*>(glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	for(unsigned int y=0;y<scan.getSize(1);++y)
		for(unsigned int x=0;x<scan.getSize(0);++x,pbPtr+=3)
			{
			const X3PScan::Vector& normal=scan.getNormal(x,y);
			for(int i=0;i<3;++i)
				pbPtr[i]=GLfloat(normal[i]);
			}
	glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
	
	/* Upload the pixel buffer object into the normal vector texture object: */
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,scan.getSize(0),scan.getSize(1),GL_RGB,GL_FLOAT,0);
	
	/* Unbind and destroy the temporary pixel buffer object: */
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,0);
	glDeleteBuffersARB(1,&pixelBufferId);
	
	/* Protect the normal vector texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	}

VRUI_APPLICATION_RUN(X3PViewer)
