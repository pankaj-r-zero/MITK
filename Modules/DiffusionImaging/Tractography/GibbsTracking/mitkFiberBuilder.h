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
#ifndef _BUILDFIBRES
#define _BUILDFIBRES


#include <MitkDiffusionImagingExports.h>
#include <itkOrientationDistributionFunction.h>

#include "mitkParticleGrid.h"

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkPolyLine.h>
#include <vtkCleanPolyData.h>

#include <itkVector.h>
#include <itkImage.h>

using namespace std;

namespace mitk
{

class MitkDiffusionImaging_EXPORT FiberBuilder
{
public:

    typedef itk::Image<float, 3>  ItkFloatImageType;

    FiberBuilder(ParticleGrid* grid, ItkFloatImageType* image);
    ~FiberBuilder();

    vtkSmartPointer<vtkPolyData> iterate(int minFiberLength);

protected:

    void AddPoint(Particle *dp, vtkSmartPointer<vtkPolyLine> container);

    void labelPredecessors(Particle *dp, vtkSmartPointer<vtkPolyLine> container);
    void labelSuccessors(Particle *dp, vtkSmartPointer<vtkPolyLine> container);

    itk::Point<float>           m_LastPoint;
    float                       m_FiberLength;
    ItkFloatImageType::Pointer  m_Image;
    ParticleGrid*               m_Grid;
    Particle*                   particles;
    vtkSmartPointer<vtkCellArray> m_VtkCellArray;
    vtkSmartPointer<vtkPoints>    m_VtkPoints;

};

}

#endif
