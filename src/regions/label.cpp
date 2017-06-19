/*
 * DIPlib 3.0
 * This file contains the definition for the Label function.
 *
 * (c)2017, Cris Luengo.
 * Based on original DIPlib code: (c)1995-2014, Delft University of Technology.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "diplib.h"
#include "diplib/regions.h"
#include "diplib/union_find.h"
#include "diplib/neighborlist.h"
#include "diplib/iterators.h"
#include "diplib/framework.h" // for OptimalProcessingDim

#include "labelingGrana2016.h"

namespace dip {

namespace {

// Returns true if a pixel with relative coordinates `coords` is connected according to `connectivity`.
bool IsConnected( IntegerArray const& coords, dip::uint connectivity ) {
   dip::uint dist = 0;
   for( auto c : coords ) {
      c = std::abs( c );
      if( c > 1 ) {
         return false;
      } else if( c == 1 ) {
         ++dist;
      }
   }
   return dist <= connectivity;
}

// Returns true if relative coordinates `coords` is the previous pixel along the same line, given by `procDim`.
bool IsPrevious( IntegerArray const& coords, dip::uint procDim ) {
   for( dip::uint ii = 0; ii < coords.size(); ++ii ) {
      if( coords[ ii ] != ( ii == procDim ? -1 : 0 )) {
         return false;
      }
   }
   return true;
}

void LabelFirstPass(
      Image& c_img,
      LabelRegionList& regions,
      NeighborList const& c_neighborList,
      dip::uint connectivity
) {
   dip::uint procDim = Framework::OptimalProcessingDim( c_img );
   // Select only those neighbors that are processed earlier
   NeighborList neighborList = c_neighborList.SelectBackward( procDim );
   // Prepare other needed data
   IntegerArray neighborOffsets = neighborList.ComputeOffsets( c_img.Strides() );
   ImageIterator< LabelType > it( c_img, procDim );
   dip::uint length = c_img.Size( procDim );
   DIP_THROW_IF( length < 3, "The image is too small for the labeling routine to work correctly" );
   // Note that if length < 3, the image is very small all around, because `OptimalProcessingDim` will return a larger dimension if it exists.
   // TODO: make a trivial labeling routine for tiny images like this.
   dip::sint stride = c_img.Stride( procDim );
   dip::sint endOffset = stride * static_cast< dip::sint >( length  - 1 );
   LabelType lastLabel = regions.Create( 0 ); // This is the region for label 1, which we cannot use because unprocessed pixels have this value
   DIP_ASSERT( lastLabel == 1 );
   // Loop over every image line
   do {
      // Which neighbors can we use on this line?
      //    +-+-+-+  `x` = current pixel
      //    |n|n|m|  `p` = previous pixel
      //    +-+-+-+  `n` = neighbor that is also a neighbor to `p`
      //    |p|x| |  `m` = neighbor that is not a neighbor to `p`
      //    +-+-+-+
      //    | | | |  If `p` is set, `x` gets the same label. We need to test only `m` pixels
      //    +-+-+-+  Otherwise, we test all pixels `n` and `m`.
      IntegerArray allNeighbors;     // `n` and `m` pixels
      IntegerArray forwardNeighbors; // `m` pixels only
      UnsignedArray coords = it.Coordinates();
      coords[ procDim ] = 1; // pretend we're in the middle of the line here, so we can properly test for neighbors being inside or outside the image
                             // NOTE! this will not work correctly if the image has less that 3 pixels along `procDim`.
      auto nl = neighborList.begin();
      auto no = neighborOffsets.begin();
      for( ; nl != neighborList.end(); ++no, ++nl ) {
         if( !IsPrevious( nl.Coordinates(), procDim ) && nl.IsInImage( coords, c_img.Sizes() )) {
            allNeighbors.push_back( *no );
            IntegerArray cc = nl.Coordinates();
            ++( cc[ procDim ] );
            if( !IsConnected( cc, connectivity )) {
               forwardNeighbors.push_back( *no );
            }
         }
      }
      lastLabel = 0;
      LabelType* img = it.Pointer();
      LabelType* end = img + endOffset;

      // First pixel on line:
      if( *img ) {
         // TODO: We only need to check allNeighbors, which potentially has fewer elements that neighborList. But we'd need the coordinates along with it.
         coords[ procDim ] = 0;
         nl = neighborList.begin();
         no = neighborOffsets.begin();
         for( ; nl != neighborList.end(); ++no, ++nl ) {
            if( nl.IsInImage( coords, c_img.Sizes() )) {
               LabelType lab = img[ *no ];
               if( lab ) {
                  if( lastLabel ) {
                     lastLabel = regions.Union( lastLabel, lab );
                  } else {
                     lastLabel = lab;
                  }
               }
            }
         }
         if( lastLabel ) {
            ++( regions.Value( lastLabel ));
         } else {
            lastLabel = regions.Create( 1 );
         }
         *img = lastLabel;
      }
      img += stride;

      // The rest of the pixels:
      do {
         if( *img ) {
            if( lastLabel ) {
               for( auto nn : forwardNeighbors ) {
                  LabelType lab = img[ nn ];
                  if( lab ) {
                     lastLabel = regions.Union( lastLabel, lab );
                  }
               }
               ++( regions.Value( lastLabel ));
               *img = lastLabel;
            } else {
               for( auto nn : allNeighbors ) {
                  LabelType lab = img[ nn ];
                  if( lab ) {
                     if( lastLabel ) {
                        lastLabel = regions.Union( lastLabel, lab );
                     } else {
                        lastLabel = lab;
                     }
                  }
               }
               if( lastLabel ) {
                  ++( regions.Value( lastLabel ));
               } else {
                  lastLabel = regions.Create( 1 );
               }
               *img = lastLabel;
            }
         } else {
            lastLabel = 0;
         }
      } while(( img += stride ) != end );

      // The last pixel:
      if( *img ) {
         // TODO: if lastLabel, then we only need to check forwardNeighbors, but we'd need coordinates attached to them, which we don't have
         coords[ procDim ] = length - 1;
         nl = neighborList.begin();
         no = neighborOffsets.begin();
         for( ; nl != neighborList.end(); ++no, ++nl ) {
            if( nl.IsInImage( coords, c_img.Sizes() )) {
               LabelType lab = img[ *no ];
               if( lab ) {
                  if( lastLabel ) {
                     lastLabel = regions.Union( lastLabel, lab );
                  } else {
                     lastLabel = lab;
                  }
               }
            }
         }
         if( lastLabel ) {
            ++( regions.Value( lastLabel ));
         } else {
            lastLabel = regions.Create( 1 );
         }
         *img = lastLabel;
      }

   } while( ++it );

}

} // namespace

dip::uint Label(
      Image const& c_in,
      Image& out,
      dip::uint connectivity,
      dip::uint minSize,
      dip::uint maxSize,
      StringArray const& boundaryCondition
) {
   DIP_THROW_IF( !c_in.IsForged(), E::IMAGE_NOT_FORGED );
   DIP_THROW_IF( !c_in.IsScalar(), E::IMAGE_NOT_SCALAR );
   DIP_THROW_IF( !c_in.DataType().IsBinary(), E::IMAGE_NOT_BINARY );
   dip::uint nDims = c_in.Dimensionality();
   DIP_THROW_IF( connectivity > nDims, E::PARAMETER_OUT_OF_RANGE );

   Image in = c_in.QuickCopy();
   auto pixelSize = in.PixelSize();
   out.ReForge( in, DT_LABEL );
   out.SetPixelSize( pixelSize );

   LabelRegionList regions{ std::plus< dip::uint >{} };
   NeighborList neighborList( { Metric::TypeCode::CONNECTED, connectivity }, nDims );

   // First scan
   if(( nDims == 2 ) && ( connectivity == 2 )) {
      out.Fill( 0 );
      LabelFirstPass_Grana2016( in, out, regions );
      // TODO: Do we want to keep the Grana2016 code?
      // This actually only saves ~10% on an image 2k x 2k pixels: 0.071s vs 0.078s
      // (including MATLAB overhead, probably slightly larger relative difference without that overhead).
   } else {
      out.Copy( in );
      LabelFirstPass( out, regions, neighborList, connectivity );
      regions.Union( 0, 1 ); // This gets rid of label 1, which we used internally, but otherwise causes the first region to get label 2.
   }

   // Handle boundary condition
   if( !boundaryCondition.empty() ) {
      BoundaryConditionArray bc;
      DIP_START_STACK_TRACE
         bc = StringArrayToBoundaryConditionArray( boundaryCondition );
         ArrayUseParameter( bc, nDims, BoundaryCondition::ADD_ZEROS ); // any default that is not PERIODIC will do...
      DIP_END_STACK_TRACE
      for( dip::uint ii = 0; ii < nDims; ++ii ) {
         if( bc[ ii ] == BoundaryCondition::PERIODIC ) {
            // Merge labels for objects touching opposite sides of image along this dimension
            // TODO: take connectivity into account, now we do connectivity==1 only.
            dip::sint lastPixelOffset = out.Stride( ii ) * static_cast< dip::sint >( out.Size( ii ) - 1 );
            auto it = ImageIterator< LabelType >( out, ii );
            do {
               LabelType lab1 = *it;
               LabelType lab2 = *( it.Pointer() + lastPixelOffset );
               if(( lab1 > 0 ) && ( lab2 > 0 )) {
                  regions.Union( lab1, lab2 );
               }
            } while( ++it );

         }
      }
   }

   // Relabel
   dip::uint nLabel;
   if(( minSize > 0 ) && ( maxSize > 0 )) {
      nLabel = regions.Relabel(
            [ & ]( dip::uint size ){ return ( size >= minSize ) && ( size <= maxSize ); }
      );
   } else if( minSize > 0 ) {
      nLabel = regions.Relabel(
            [ & ]( dip::uint size ){ return size >= minSize; }
      );
   } else if( maxSize > 0 ) {
      nLabel = regions.Relabel(
            [ & ]( dip::uint size ){ return size <= maxSize; }
      );
   } else {
      nLabel = regions.Relabel();
   }

   // Second scan
   auto it = ImageIterator< LabelType >( out );
   do {
      if( *it > 0 ) {
         *it = regions.Label( *it );
      }
   } while( ++it );

   return nLabel;
}

} // namespace dip
