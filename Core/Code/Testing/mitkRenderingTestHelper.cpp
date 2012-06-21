/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center, 
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without 
even the implied warranty of MERCHANTABILITY or FITNESS FOR 
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "mitkRenderingTestHelper.h"
#include <mitkStandaloneDataStorage.h>

#include <vtkRenderWindow.h>
#include <vtkPNGWriter.h>
#include <vtkRenderLargeImage.h>
#include <vtkRenderWindowInteractor.h>

#include <mitkRenderWindow.h>
#include <mitkGlobalInteraction.h>
#include <mitkSliceNavigationController.h>
#include <mitkNodePredicateDataType.h>

mitkRenderingTestHelper::mitkRenderingTestHelper(int width, int height, int argc, char* argv[])
:m_width(width), m_height(height)
{
    // Global interaction must(!) be initialized
    mitk::GlobalInteraction::GetInstance()->Initialize("global");

    m_DataStorage = mitk::StandaloneDataStorage::New();

    m_RenderWindow = mitk::RenderWindow::New();
    m_RenderWindow->GetRenderer()->SetDataStorage(m_DataStorage);
    m_RenderWindow->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard2D);
    this->GetVtkRenderWindow()->SetSize( width, height );
    this->SetInputFileNames(argc, argv);
}

mitkRenderingTestHelper::~mitkRenderingTestHelper()
{
}

void mitkRenderingTestHelper::Render()
{
    //if the datastorage is initialized and at least 1 image is loaded render it
    if(m_DataStorage.IsNotNull() || m_DataStorage->GetAll()->Size() >= 1 )
    {
        mitk::TimeSlicedGeometry::Pointer geo = m_DataStorage->ComputeBoundingGeometry3D(m_DataStorage->GetAll());
        m_RenderWindow->GetRenderer()->Resize(m_width, m_height);

        mitk::RenderingManager::GetInstance()->InitializeViews( geo );
        mitk::RenderingManager::GetInstance()->RequestUpdate(m_RenderWindow->GetVtkRenderWindow());
    }
    else
    {
        MITK_ERROR << "No images loaded in data storage!";
    }

    //use this to actually show the iamge in a renderwindow
    //  this->GetVtkRenderWindow()->Render();
    //  this->GetVtkRenderWindow()->GetInteractor()->Start();
}

void mitkRenderingTestHelper::SetInputFileNames(int argc, char* argv[])
{
    // parse parameters
    for (int i = 1; i < argc; ++i)
    {
        //add everything to a list but -T and -V
        std::string tmp = argv[i];
        if((tmp.compare("-T")) && (tmp.compare("-V")))
        {
            this->AddToStorage(tmp);
        }
        else
        {
            break;
        }
    }
}

void mitkRenderingTestHelper::SetViewDirection(mitk::SliceNavigationController::ViewDirection viewDirection)
{
    mitk::BaseRenderer::GetInstance(m_RenderWindow->GetVtkRenderWindow())->GetSliceNavigationController()->SetDefaultViewDirection(viewDirection);
}

vtkRenderer* mitkRenderingTestHelper::GetVtkRenderer()
{
    return m_RenderWindow->GetRenderer()->GetVtkRenderer();
}

void mitkRenderingTestHelper::SetProperty(const char *propertyKey, mitk::BaseProperty* property )
{
    this->m_DataStorage->GetNode(mitk::NodePredicateDataType::New("Image"))->SetProperty(propertyKey, property);
}

vtkRenderWindow* mitkRenderingTestHelper::GetVtkRenderWindow()
{
    return m_RenderWindow->GetVtkRenderWindow();
}

//method to save a screenshot of the renderwindow (e.g. create a reference screenshot)
void mitkRenderingTestHelper::SaveAsPNG(std::string fileName)
{
    vtkSmartPointer<vtkRenderer> renderer = this->GetVtkRenderer();
    bool doubleBuffering( renderer->GetRenderWindow()->GetDoubleBuffer() );
    renderer->GetRenderWindow()->DoubleBufferOff();

    vtkSmartPointer<vtkRenderLargeImage> magnifier = vtkSmartPointer<vtkRenderLargeImage>::New();
    magnifier->SetInput(renderer);
    magnifier->SetMagnification(1.0);

    vtkSmartPointer<vtkImageWriter> fileWriter = vtkSmartPointer<vtkPNGWriter>::New();
    fileWriter->SetInput(magnifier->GetOutput());
    fileWriter->SetFileName(fileName.c_str());

    fileWriter->Write();
    renderer->GetRenderWindow()->SetDoubleBuffer(doubleBuffering);
}

void mitkRenderingTestHelper::AddToStorage(const std::string &filename)
{
    mitk::DataNodeFactory::Pointer reader = mitk::DataNodeFactory::New();
    try
    {
        reader->SetFileName( filename );
        reader->Update();

        if(reader->GetNumberOfOutputs()<1)
        {
            MITK_ERROR << "Could not find test data '" << filename << "'";
        }

        mitk::DataNode::Pointer node = reader->GetOutput( 0 );
        this->m_DataStorage->Add(node);
    }
    catch ( itk::ExceptionObject & e )
    {
        MITK_ERROR << "Failed loading test data '" << filename << "': " << e.what();
    }
}

