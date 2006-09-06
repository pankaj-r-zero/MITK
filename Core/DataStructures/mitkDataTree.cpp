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


#include "mitkDataTree.h"
#include <mitkXMLWriter.h>
#include <mitkXMLReader.h>
#include <mitkSmartPointerProperty.h>

const std::string mitk::DataTree::XML_NODE_NAME = "mitkDataTree";
const std::string mitk::DataTree::XML_TAG_TREE_NODE = "treeNode";

//##ModelId=3E38F46A0190
mitk::DataTree::DataTree() : 
DataTreeBase( )
{
  SetRoot(mitk::DataTreeNode::New());
}


//##ModelId=3E38F46A01AE
mitk::DataTree::~DataTree()
{
  Clear();
}

/**
*
*/
//##ModelId=3E3FE0430148
mitk::DataTreeIteratorClone mitk::DataTree::GetNext( const char* propertyKey, const mitk::BaseProperty* property,  mitk::DataTreeIteratorBase* startPosition )
{
  DataTreeIteratorClone pos;

  if(startPosition != NULL)
    pos = *startPosition;
  else
    pos = DataTreePreOrderIterator(this);

  mitk::DataTreeNode::Pointer dtn;
  while ( !pos->IsAtEnd() )
  {
    dtn = pos->Get();
    mitk::PropertyList::Pointer propertyList = dtn->GetPropertyList();
    mitk::BaseProperty::Pointer tmp = propertyList->GetProperty( propertyKey );
    if ( (*property) == *(tmp) )
      return pos;
    ++pos;
  }
  return pos;
}

mitk::DataTreeIteratorClone mitk::DataTree::GetIteratorToNode(mitk::DataTreeBase* tree, const mitk::DataTreeNode* node, const mitk::DataTreeIteratorBase* startPosition )
{
  DataTreeIteratorClone pos;

  if(startPosition != NULL)
    pos = *startPosition;
  else
    pos = DataTreePreOrderIterator(tree);

  while ( !pos->IsAtEnd() )
  {
    if ( pos->Get().GetPointer() == node )
      return pos;
    ++pos;
  }
  return pos;

}


//##ModelId=3ED91D050085
mitk::BoundingBox::Pointer mitk::DataTree::ComputeBoundingBox(mitk::DataTreeIteratorBase* it, const char* boolPropertyKey, mitk::BaseRenderer* renderer, const char* boolPropertyKey2)
{
  mitk::DataTreeIteratorClone _it=it;
  mitk::BoundingBox::PointsContainer::Pointer pointscontainer=mitk::BoundingBox::PointsContainer::New();

  mitk::BoundingBox::PointIdentifier pointid=0;
  mitk::Point3D point;

  while (!_it->IsAtEnd())
  {
    mitk::DataTreeNode::Pointer node = _it->Get();
    if((node.IsNotNull()) && (node->GetData() != NULL) && node->IsOn(boolPropertyKey, renderer) && node->IsOn(boolPropertyKey2, renderer))
    {
      const Geometry3D* geometry = node->GetData()->GetUpdatedTimeSlicedGeometry();
      if (geometry != NULL ) 
      {
        unsigned char i;
        for(i=0; i<8; ++i)
        {
          point = geometry->GetCornerPoint(i);
          if(point[0]*point[0]+point[1]*point[1]+point[2]*point[2] < mitk::large)
            pointscontainer->InsertElement( pointid++, point);
          else
          {
            itkGenericOutputMacro( << "Unrealistically distant corner point encountered. Ignored. Node: " << node );
          }
        }
      }
    }
    ++_it;
  }

  mitk::BoundingBox::Pointer result = mitk::BoundingBox::New();
  result->SetPoints(pointscontainer);
  result->ComputeBoundingBox();

  return result;
}

mitk::TimeBounds mitk::DataTree::ComputeTimeBounds(mitk::DataTreeIteratorBase* it, const char* boolPropertyKey, mitk::BaseRenderer* renderer, const char* boolPropertyKey2)
{
  TimeBounds timeBounds;

  mitk::DataTreeIteratorClone _it=it;

  mitk::ScalarType stmin, stmax, cur;

  stmin=-ScalarTypeNumericTraits::max();
  stmax= ScalarTypeNumericTraits::max();

  timeBounds[0]=stmax; timeBounds[1]=stmin;

  while (!_it->IsAtEnd())
  {
    mitk::DataTreeNode::Pointer node = _it->Get();
    if((node.IsNotNull()) && (node->GetData() != NULL) && node->IsOn(boolPropertyKey, renderer) && node->IsOn(boolPropertyKey2, renderer) && node->IsOn(boolPropertyKey2, renderer))
    {
      const Geometry3D* geometry = node->GetData()->GetUpdatedTimeSlicedGeometry();
      if (geometry != NULL ) 
      {
        const TimeBounds & curTimeBounds = geometry->GetTimeBounds();
        cur=curTimeBounds[0];
        //is it after -infinity, but before everything else that we found until now?
        if((cur > stmin) && (cur < timeBounds[0]))
          timeBounds[0] = cur;

        cur=curTimeBounds[1];
        //is it before infinity, but after everything else that we found until now?
        if((cur < stmax) && (cur > timeBounds[1]))
          timeBounds[1] = cur;
      }
    }
    ++_it;
  }

  if(!(timeBounds[0]<stmax))
  {
    timeBounds[0] = stmin;
    timeBounds[1] = stmax;
  }

  return timeBounds;
}

bool mitk::DataTree::Load( const mitk::DataTreeIteratorBase* it, const char* filename )
{
  mitk::PropertyList::PrepareXML_IO(); // to get "shared" properties right  
  bool result = mitk::XMLReader::Load( filename, it );
  SmartPointerProperty::PostProcessXMLReading(); // to get "shared" properties right  

  return result;
}

bool mitk::DataTree::Save( const mitk::DataTreeIteratorBase* it, const char* fileName, bool writeRootNodeToo ) 
{
  if ( fileName == NULL || it == NULL || it->IsAtEnd() )
    return false;
  
  mitk::XMLWriter writer( fileName );

  if ( !Save( it, writer, writeRootNodeToo ) )
    writer.WriteComment( "Error" );

  return true;
}

bool mitk::DataTree::SaveNext( const mitk::DataTreeIteratorBase* it, mitk::XMLWriter& xmlWriter, bool writeRootNodeToo ) 
{
  if ( it == NULL || it->IsAtEnd() )
    return false;

  mitk::DataTreeNode* node = it->Get();
  xmlWriter.BeginNode(XML_TAG_TREE_NODE);

  if (node) 
  {

    if (writeRootNodeToo)
    {
      mitk::StringProperty* sprop = dynamic_cast<mitk::StringProperty*>( node->GetProperty("FILENAME").GetPointer() );
      if (sprop) // if this property is set, then use it as a filename
      {
        xmlWriter.SetSourceFileName( sprop->GetValue() );      
      }

      node->WriteXML( xmlWriter );
    }

    if ( it->HasChild() ) 
    {
      DataTreeChildIterator children(*it);

      while (!children.IsAtEnd())
      {
        SaveNext( &children, xmlWriter, true );
        ++children;
      }
    }
  }

  xmlWriter.EndNode(); // TreeNode

  return true;
}

bool mitk::DataTree::Save( const mitk::DataTreeIteratorBase* it, mitk::XMLWriter& xmlWriter, bool writeRootNodeToo ) 
{
  mitk::PropertyList::PrepareXML_IO(); // to get "shared" properties right  
  xmlWriter.BeginNode(XML_NODE_NAME);
  bool result = SaveNext( it, xmlWriter, writeRootNodeToo );
  xmlWriter.EndNode(); // mitkDataTree
  return result;
}
