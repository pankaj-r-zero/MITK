/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile$
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <mitkSlicedGeometry3D.h>
#include <mitkPlaneGeometry.h>
#include <mitkXMLWriter.h>
#include <mitkXMLReader.h>
#include <mitkRotationOperation.h>

const std::string mitk::SlicedGeometry3D::DIRECTION_VECTOR = "DIRECTION_VECTOR";
const std::string mitk::SlicedGeometry3D::EVENLY_SPACED = "EVENLY_SPACED";
const std::string mitk::SlicedGeometry3D::SLICES = "SLICES";

//##ModelId=3DCBF50C0377
mitk::Geometry2D* mitk::SlicedGeometry3D::GetGeometry2D(int s) const
{
  mitk::Geometry2D::Pointer geometry2d = NULL;
  
  if(IsValidSlice(s))
  {
    geometry2d = m_Geometry2Ds[s];
    //If (a) m_EvenlySpaced==true, (b) we don't have a Geometry2D stored 
    //for the requested slice, and (c) the first slice (s=0) 
    //is a PlaneGeometry instance, then we calculate the geometry of the
    //requested as the plane of the first slice shifted by m_Spacing[2]*s
    //in the direction of m_DirectionVector.
    if((m_EvenlySpaced) && (geometry2d.IsNull()))
    {
      PlaneGeometry* firstslice=dynamic_cast<PlaneGeometry*> (m_Geometry2Ds[0].GetPointer());
      if(firstslice != NULL)
      {
        if((m_DirectionVector[0]==0) && (m_DirectionVector[1]==0) && (m_DirectionVector[2]==0))
        {
          m_DirectionVector=firstslice->GetNormal();
          m_DirectionVector.Normalize();
        }
        Vector3D direction;
        direction = m_DirectionVector*m_Spacing[2];

        mitk::PlaneGeometry::Pointer requestedslice;
        requestedslice = static_cast<mitk::PlaneGeometry*>(firstslice->Clone().GetPointer());
        requestedslice->SetOrigin(requestedslice->GetOrigin()+direction*s);
        geometry2d = requestedslice;
        m_Geometry2Ds[s] = geometry2d;
      }
    }
  }
  else
    return NULL;
  return geometry2d;
}

//##ModelId=3DCBF5D40253
const mitk::BoundingBox* mitk::SlicedGeometry3D::GetBoundingBox() const
{
  assert(m_BoundingBox.IsNotNull());
  return m_BoundingBox.GetPointer();
}

bool mitk::SlicedGeometry3D::SetGeometry2D(mitk::Geometry2D* geometry2D, int s)
{
  if(IsValidSlice(s))
  {
    m_Geometry2Ds[s]=geometry2D;
    return true;
  }
  return false;
}

void mitk::SlicedGeometry3D::Initialize(unsigned int slices)
{
  Superclass::Initialize();
  m_Slices = slices;

  m_Geometry2Ds.clear();
 
  Geometry2D::Pointer gnull=NULL;
  m_Geometry2Ds.reserve(m_Slices);
  m_Geometry2Ds.assign(m_Slices, gnull);
 
  Vector3D spacing;
  spacing.Fill(1.0);
  SetSpacing(spacing);

  m_DirectionVector.Fill(0);
}


void mitk::SlicedGeometry3D::InitializeEvenlySpaced(mitk::Geometry2D* geometry2D, unsigned int slices, bool flipped)
{
  assert(geometry2D!=NULL);
  InitializeEvenlySpaced(geometry2D, geometry2D->GetExtentInMM(2)/geometry2D->GetExtent(2), slices, flipped);
}

void mitk::SlicedGeometry3D::InitializeEvenlySpaced(mitk::Geometry2D* geometry2D, mitk::ScalarType zSpacing, unsigned int slices, bool flipped)
{
  assert(geometry2D!=NULL);
  assert(geometry2D->GetExtent(0)>0);
  assert(geometry2D->GetExtent(1)>0);

  geometry2D->Register();

  Superclass::Initialize();
  m_Slices = slices;

  BoundingBox::BoundsArrayType bounds = geometry2D->GetBounds();
  bounds[4] = 0;
  bounds[5] = slices;

  m_Geometry2Ds.clear(); 
  Geometry2D::Pointer gnull=NULL;
  m_Geometry2Ds.reserve(m_Slices);
  m_Geometry2Ds.assign(m_Slices, gnull);

  Vector3D directionVector = geometry2D->GetAxisVector(2);
  directionVector.Normalize();
  directionVector *= zSpacing;

  if(flipped == false)
  {
    //normally we should use the following four lines to create a copy of the transform contrained in geometry2D,
    //because it may not be changed by us. But we know that SetSpacing creates a new transform without changing
    //the old (coming from geometry2D), so we can use the fifth line instead. We check this at (**).
    //AffineTransform3D::Pointer transform = AffineTransform3D::New();
    //transform->SetMatrix(geometry2D->GetIndexToWorldTransform()->GetMatrix());
    //transform->SetOffset(geometry2D->GetIndexToWorldTransform()->GetOffset());
    //SetIndexToWorldTransform(transform);
    m_IndexToWorldTransform = const_cast<AffineTransform3D*>(geometry2D->GetIndexToWorldTransform());
  }
  else
  {
    directionVector *= -1.0;
    m_IndexToWorldTransform = AffineTransform3D::New();
    m_IndexToWorldTransform->SetMatrix(geometry2D->GetIndexToWorldTransform()->GetMatrix());

    AffineTransform3D::OutputVectorType scaleVector;
    FillVector3D(scaleVector, 1.0, 1.0, -1.0);
    m_IndexToWorldTransform->Scale(scaleVector, true);
    m_IndexToWorldTransform->SetOffset(geometry2D->GetIndexToWorldTransform()->GetOffset());
  }

  mitk::Vector3D spacing;
  FillVector3D(spacing, geometry2D->GetExtentInMM(0)/bounds[1], geometry2D->GetExtentInMM(1)/bounds[3], zSpacing);
  //ensure that spacing differs from m_Spacing to make SetSpacing change the matrix
  m_Spacing[2]=zSpacing-1;

  SetDirectionVector(directionVector);
  SetBounds(bounds);
  SetGeometry2D(geometry2D, 0);
  SetSpacing(spacing);
  SetEvenlySpaced();
  SetTimeBounds(geometry2D->GetTimeBounds());

  assert(m_IndexToWorldTransform.GetPointer() != geometry2D->GetIndexToWorldTransform()); // (**) see above.

  SetFrameOfReferenceID(geometry2D->GetFrameOfReferenceID());
  SetImageGeometry(geometry2D->GetImageGeometry());

  geometry2D->UnRegister();
}

void mitk::SlicedGeometry3D::InitializePlanes(const mitk::Geometry3D* geometry3D, mitk::PlaneGeometry::PlaneOrientation planeorientation, bool top, bool frontside, bool rotated)
{
  PlaneGeometry::Pointer planegeometry = mitk::PlaneGeometry::New();
  planegeometry->InitializeStandardPlane(geometry3D, top, planeorientation, frontside, rotated);

  ScalarType viewSpacing = 1;
  unsigned int slices = 1;

  switch(planeorientation)
  {
    case PlaneGeometry::Transversal:
      viewSpacing = geometry3D->GetSpacing()[2];
      slices = (unsigned int) geometry3D->GetExtent(2);
      break;
    case PlaneGeometry::Frontal:
      viewSpacing = geometry3D->GetSpacing()[1];
      slices = (unsigned int) geometry3D->GetExtent(1);
      break;
    case PlaneGeometry::Sagittal:
      viewSpacing = geometry3D->GetSpacing()[0];
      slices = (unsigned int) geometry3D->GetExtent(0);
      break;
    default:
      itkExceptionMacro("unknown PlaneOrientation");
  }

  bool flipped = (top == false);
  if(frontside == false)
    flipped = !flipped;
  if(planeorientation == PlaneGeometry::Frontal)
    flipped = !flipped;
  InitializeEvenlySpaced(planegeometry, viewSpacing, slices, flipped);
}

void mitk::SlicedGeometry3D::SetImageGeometry(const bool isAnImageGeometry)
{
  Superclass::SetImageGeometry(isAnImageGeometry);

  mitk::Geometry3D* geometry;
  unsigned int s;
  for(s=0; s < m_Slices; ++s)
  {
    geometry = m_Geometry2Ds[s];
    if(geometry!=NULL)
      geometry->SetImageGeometry(isAnImageGeometry);
  }
}

//##ModelId=3E15572E0269
mitk::SlicedGeometry3D::SlicedGeometry3D() : m_EvenlySpaced(true), m_Slices(0)
{
  Initialize(m_Slices);
}

//##ModelId=3E3456C50067
mitk::SlicedGeometry3D::~SlicedGeometry3D()
{

}

//##ModelId=3E3BE1F10106
bool mitk::SlicedGeometry3D::IsValidSlice(int s) const
{
  return ((s>=0) && (s<(int)m_Slices));
}

//##ModelId=3E3BE8CF010E
void mitk::SlicedGeometry3D::SetSpacing(const mitk::Vector3D& aSpacing)
{
  bool hasEvenlySpacedPlaneGeometry=false;
  mitk::Point3D origin;
  mitk::Vector3D rightDV, bottomDV;
  mitk::ScalarType oldwidthInMM, oldheightInMM;
  BoundingBox::BoundsArrayType bounds;

  assert(aSpacing[0]>0 && aSpacing[1]>0 && aSpacing[2]>0);

  //in case of evenly-spaced data: re-initialize instances of Geometry2D, since the spacing influences them
  if((m_EvenlySpaced) && (m_Geometry2Ds.size()>0))
  {
    mitk::Geometry2D::ConstPointer firstGeometry = m_Geometry2Ds[0].GetPointer();

    const PlaneGeometry* constplanegeometry=dynamic_cast<const PlaneGeometry*>(firstGeometry.GetPointer());
    if(constplanegeometry != NULL)
    {
      WorldToIndex(constplanegeometry->GetOrigin(), origin);
      WorldToIndex(constplanegeometry->GetOrigin(), constplanegeometry->GetAxisVector(0), rightDV);
      WorldToIndex(constplanegeometry->GetOrigin(), constplanegeometry->GetAxisVector(1), bottomDV);
      oldwidthInMM = constplanegeometry->GetExtentInMM(0);
      oldheightInMM = constplanegeometry->GetExtentInMM(0);
      bounds = constplanegeometry->GetBounds();
      hasEvenlySpacedPlaneGeometry=true;
    }
  }

  Superclass::SetSpacing(aSpacing);

  mitk::Geometry2D::Pointer firstGeometry;

  //in case of evenly-spaced data: re-initialize instances of Geometry2D, since the spacing influences them
  if(hasEvenlySpacedPlaneGeometry)
  {
    //create planegeometry according to new spacing

    IndexToWorld(origin, origin);
    IndexToWorld(origin, rightDV, rightDV);
    IndexToWorld(origin, bottomDV, bottomDV);

    mitk::PlaneGeometry::Pointer planegeometry=mitk::PlaneGeometry::New();
    planegeometry->InitializeStandardPlane(rightDV.Get_vnl_vector(), bottomDV.Get_vnl_vector(), &m_Spacing);
    planegeometry->SetOrigin(origin);
    planegeometry->SetBounds(bounds);
    firstGeometry = planegeometry;
  }
  else
  if((m_EvenlySpaced) && (m_Geometry2Ds.size()>0))
    firstGeometry = m_Geometry2Ds[0].GetPointer();

  //clear and reserve
  m_Geometry2Ds.clear();

  Geometry2D::Pointer gnull=NULL;
  m_Geometry2Ds.reserve(m_Slices);
  m_Geometry2Ds.assign(m_Slices, gnull);

  if(m_Slices>0)
    m_Geometry2Ds[0] = firstGeometry;

  Modified();
}

//##ModelId=3E3C13F802A6
void mitk::SlicedGeometry3D::SetEvenlySpaced(bool on)
{
  if(m_EvenlySpaced!=on)
  {
    m_EvenlySpaced=on;
    Modified();
  }
}

//##ModelId=3E3C2C37031B
void mitk::SlicedGeometry3D::SetDirectionVector(const mitk::Vector3D& directionVector)
{
  VnlVector diff = m_DirectionVector.Get_vnl_vector()-directionVector.Get_vnl_vector();
  if(diff.squared_magnitude()>=vnl_math::float_sqrteps)
  {
    m_DirectionVector = directionVector;
    m_DirectionVector.Normalize();
    Modified();
  }
}

void mitk::SlicedGeometry3D::SetTimeBounds(const mitk::TimeBounds& timebounds)
{
  Superclass::SetTimeBounds(timebounds);

  unsigned int s;
  for(s=0; s<m_Slices; ++s)
  {
    if(m_Geometry2Ds[s].IsNotNull())
    {
      m_Geometry2Ds[s]->SetTimeBounds(timebounds);
    }
  }
  m_TimeBounds = timebounds;
}

mitk::AffineGeometryFrame3D::Pointer mitk::SlicedGeometry3D::Clone() const
{
  Self::Pointer newGeometry = Self::New();
  newGeometry->Initialize(m_Slices);
  InitializeGeometry(newGeometry);
  return newGeometry.GetPointer();
}

void mitk::SlicedGeometry3D::InitializeGeometry(Self * newGeometry) const
{
  Superclass::InitializeGeometry(newGeometry);

  newGeometry->SetEvenlySpaced(m_EvenlySpaced);
  newGeometry->SetSpacing(GetSpacing());
  newGeometry->SetDirectionVector(GetDirectionVector());

  if(m_EvenlySpaced)
  {
    AffineGeometryFrame3D::Pointer geometry = m_Geometry2Ds[0]->Clone();
    Geometry2D* geometry2d = dynamic_cast<Geometry2D*>(geometry.GetPointer());
    assert(geometry2d!=NULL);
    newGeometry->SetGeometry2D(geometry2d, 0);
  }
  else
  {
    unsigned int s;
    for(s=0; s<m_Slices; ++s)
    {
      if(m_Geometry2Ds[s].IsNull())
      {
        assert(m_EvenlySpaced);
      }
      else
      {
        AffineGeometryFrame3D::Pointer geometry = m_Geometry2Ds[s]->Clone();
        Geometry2D* geometry2d = dynamic_cast<Geometry2D*>(geometry.GetPointer());
        assert(geometry2d!=NULL);
        newGeometry->SetGeometry2D(geometry2d, s);
      }
    }
  }
}

void mitk::SlicedGeometry3D::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os,indent);
  os << indent << " EvenlySpaced: " << m_EvenlySpaced << std::endl;
  if(m_EvenlySpaced)
    os << indent << " DirectionVector: " << m_DirectionVector << std::endl;
  os << indent << " Slices: " << m_Slices << std::endl;

  os << indent << " GetGeometry2D(0): ";
  if(GetGeometry2D(0)==NULL)
    os << "NULL" << std::endl;
  else
    GetGeometry2D(0)->Print(os, indent);
}

bool mitk::SlicedGeometry3D::WriteXMLData( XMLWriter& xmlWriter )
{
  Geometry3D::WriteXMLData( xmlWriter );

  xmlWriter.WriteProperty( DIRECTION_VECTOR, m_DirectionVector );
  xmlWriter.WriteProperty( EVENLY_SPACED, m_EvenlySpaced );
  xmlWriter.WriteProperty( SLICES, (int) m_Slices );

  if ( m_EvenlySpaced )
  {  
    if ( m_Geometry2Ds.size() > 0 )
    {
      Geometry2D::Pointer geometry2D = m_Geometry2Ds[0];

      if ( geometry2D.IsNotNull() )
        geometry2D->WriteXML( xmlWriter );
    }
  }
  else
  {  
    std::vector<Geometry2D::Pointer>::iterator i = m_Geometry2Ds.begin();
    const std::vector<Geometry2D::Pointer>::iterator end = m_Geometry2Ds.end();
  
    while ( i != end )
    {
      if ( (*i).IsNotNull() )
        (*i)->WriteXML( xmlWriter );

      i++;
    }
  }
  return true;
}


bool mitk::SlicedGeometry3D::ReadXMLData( XMLReader& xmlReader )
{
  Geometry3D::ReadXMLData( xmlReader );
  xmlReader.GetAttribute( DIRECTION_VECTOR, m_DirectionVector );
  xmlReader.GetAttribute( EVENLY_SPACED, m_EvenlySpaced );
  int slices;
  xmlReader.GetAttribute( "SLICES", slices );
  m_Slices = slices;

  if ( m_EvenlySpaced )
  {
    if ( xmlReader.Goto( Geometry3D::XML_NODE_NAME ) ) {
      Geometry2D::Pointer geometry2D = dynamic_cast<Geometry2D*>( xmlReader.CreateObject().GetPointer() );
      if ( geometry2D.IsNotNull() ) geometry2D->ReadXMLData( xmlReader );
      InitializeEvenlySpaced( geometry2D, GetSlices() );
      xmlReader.GotoParent();
    }
  } else if ( xmlReader.Goto( Geometry3D::XML_NODE_NAME ) ) {

    do {
      Geometry2D::Pointer geometry2D = dynamic_cast<Geometry2D*>( xmlReader.CreateObject().GetPointer() );
      if ( geometry2D.IsNotNull() ) geometry2D->ReadXMLData( xmlReader );
      m_Geometry2Ds.push_back( geometry2D );
    } while ( xmlReader.GotoNext() );
    xmlReader.GotoParent();
  }



  return false;
}

void mitk::SlicedGeometry3D::ExecuteOperation(Operation* operation)
{
  // processing is done for all operations, but only with a RotationOperation in mind.
  // @TODO check the operation for Move, Rotate, Scale, ...
  if (m_EvenlySpaced)
  {
    // clear all generated geometries and then rotate only the first slice
    // the other slices will be re-generated on demand
    // 1. save first slice
    Geometry2D::Pointer geometry2d = m_Geometry2Ds[0];
    // 2. rotate first slice
    geometry2d->ExecuteOperation(operation);

    // 3. clear all slices
    m_Geometry2Ds.clear();

    // 4. reset first slice 
    m_Geometry2Ds.reserve(m_Slices);
    m_Geometry2Ds.assign(m_Slices, NULL);
    m_Geometry2Ds[0] = geometry2d;
     
    // also rotate direction vector (which tells us, how to get the other slices from the first one)

    RotationOperation* rotOp = dynamic_cast<RotationOperation*>(operation);
    // m_DirectionVector = rot.matrix * m_DirectionVector
    typedef itk::AffineTransform<ScalarType, 3> TransformType;
    TransformType::Pointer rotation = TransformType::New();
    rotation->Rotate3D( rotOp->GetVectorOfRotation(), rotOp->GetAngleOfRotation() * vnl_math::pi / 180.0 );
    m_DirectionVector = rotation->TransformVector( m_DirectionVector );
  }
  else
  {
    // reach through to all slices
    for (std::vector<Geometry2D::Pointer>::iterator iter = m_Geometry2Ds.begin();
        iter != m_Geometry2Ds.end();
        ++iter)
    {
      (*iter)->ExecuteOperation(operation);
    }
  }

  Geometry3D::ExecuteOperation(operation); // this class itself has a transform, too

  Modified();
}
