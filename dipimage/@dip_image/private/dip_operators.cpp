/*
 * DIPimage 3.0
 * This MEX-file implements all monadic and diadic operators
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

/*
 * Interface:
 *
 * out = dip_operators(operator,lhs,rhs)
 *
 * operator = one or two characters (see switch statement below)
 * lhs = first operand
 * rhs = second operand (some operators use only one operand)
 */

#define DOCTEST_CONFIG_IMPLEMENT
#include "dip_matlab_interface.h"
#include "diplib/math.h"

void mexFunction( int nlhs, mxArray* plhs[], int nrhs, mxArray const* prhs[] ) {
   try {

      dml::MatlabInterface mi;

      dip::Image lhs;
      dip::Image rhs;
      dip::Image out = mi.NewImage();

      // Get operator
      DML_MIN_ARGS( 2 );
      DIP_THROW_IF( !mxIsChar( prhs[ 0 ] ), "First argument must be a string.");
      mxChar* ch = mxGetChars( prhs[ 0 ] );

      // Get images
      lhs = dml::GetImage( prhs[ 1 ] );
      if(( *ch == '~' ) || ( *ch == 'u' ) || ( *ch == '\'' ) || ( *ch == 'm' )) {
         DIP_THROW_IF( nrhs != 2, "Wrong number of input arguments." );
      } else {
         DIP_THROW_IF( nrhs != 3, "Wrong number of input arguments." );
         rhs = dml::GetImage( prhs[ 2 ] );
      }

      // Apply operator
      switch( *ch ) {
      // Arithmetic operators
         case '+': // +
            dip::Add( lhs, rhs, out, dip::DataType::SuggestArithmetic( lhs.DataType(), rhs.DataType() ));
            break;
         case '-': // -
            dip::Subtract( lhs, rhs, out, dip::DataType::SuggestArithmetic( lhs.DataType(), rhs.DataType() ));
            break;
         case '*': // *
            dip::Multiply( lhs, rhs, out, dip::DataType::SuggestArithmetic( lhs.DataType(), rhs.DataType() ));
            break;
         case '.': // .*
            dip::MultiplySampleWise( lhs, rhs, out, dip::DataType::SuggestArithmetic( lhs.DataType(), rhs.DataType() ));
            break;
         case '/': // ./
            dip::Divide( lhs, rhs, out, dip::DataType::SuggestArithmetic( lhs.DataType(), rhs.DataType() ));
            break;
         case '%': // mod
            dip::Modulo( lhs, rhs, out, lhs.DataType() );
            break;
         case '^': // .^
            dip::Power( lhs, rhs, out, lhs.DataType() );
            break;
      // Comparison operators
         case '=': // ==
            dip::Equal( lhs, rhs, out );
            break;
         case '>': // >
            dip::Greater( lhs, rhs, out );
            break;
         case '<': // <
            dip::Lesser( lhs, rhs, out );
            break;
         case 'g': // >=
            dip::NotLesser( lhs, rhs, out );
            break;
         case 'l': // <=
            dip::NotGreater( lhs, rhs, out );
            break;
         case 'n': // ~=
            dip::NotEqual( lhs, rhs, out );
            break;
      // Boolean (bit-wise) operators
         case '&': // and : for binary images only (but does bit-wise for integers too)
            dip::And( lhs, rhs, out );
            break;
         case '|': // or : for binary images only (but does bit-wise for integers too)
            dip::Or( lhs, rhs, out );
            break;
         case 'x': // xor : for binary images only (but does bit-wise for integers too)
            dip::Xor( lhs, rhs, out );
            break;
      // Unary operators
         case '~': // unary not : negate
            dip::Not( lhs, out );
            break;
         case 'u': // unary - : invert
            dip::Invert( lhs, out );
            break;
         case '\'': // unary (post) ' : transpose
         {
            dip::Image tmp = lhs.QuickCopy();
            tmp.Transpose();
            out = tmp; // triggers data copy!
            break;
         }
      // Monadic operators
         case 'm': // These are all the monadic operators, defined by the second letter
            switch( ch[ 1 ] ) {
               case 'a': // abs
                  dip::Abs( lhs, out );
                  break;
               case 'c': // phase
                  dip::Phase( lhs, out );
                  break;
               case 'd': // round
                  dip::Round( lhs, out );
                  break;
               case 'e': // ceil
                  dip::Ceil( lhs, out );
                  break;
               case 'f': // floor
                  dip::Floor( lhs, out );
                  break;
               case 'g': // fix
                  dip::Truncate( lhs, out );
                  break;
               case 'h': // sign
                  dip::Sign( lhs, out );
                  break;
               case 'A': // cos
                  dip::Cos( lhs, out );
                  break;
               case 'B': // sin
                  dip::Sin( lhs, out );
                  break;
               case 'C': // tan
                  dip::Tan( lhs, out );
                  break;
               case 'D': // acos
                  dip::Acos( lhs, out );
                  break;
               case 'E': // asin
                  dip::Asin( lhs, out );
                  break;
               case 'F': // atan
                  dip::Atan( lhs, out );
                  break;
               case 'G': // cosh
                  dip::Cosh( lhs, out );
                  break;
               case 'H': // sinh
                  dip::Sinh( lhs, out );
                  break;
               case 'I': // tanh
                  dip::Tanh( lhs, out );
                  break;
               case '1': // sqrt
                  dip::Sqrt( lhs, out );
                  break;
               case '2': // exp
                  dip::Exp( lhs, out );
                  break;
               case '3': // pow10
                  dip::Exp10( lhs, out );
                  break;
               case '4': // pow2
                  dip::Exp2( lhs, out );
                  break;
               case '5': // log
                  dip::Ln( lhs, out );
                  break;
               case '6': // log10
                  dip::Log10( lhs, out );
                  break;
               case '7': // log2
                  dip::Log2( lhs, out );
                  break;
               case '!': // erf
                  dip::Erf( lhs, out );
                  break;
               case '@': // erfc
                  dip::Erfc( lhs, out );
                  break;
               case '#': // gammaln
                  dip::LnGamma( lhs, out );
                  break;
               default:
                  DIP_THROW( "Unknown operator." );
            }
            break;
      // That's it!
         default:
            DIP_THROW( "Unknown operator." );
            break;
      }

      // Done
      plhs[ 0 ] = mi.GetArray( out );

   } catch( const dip::Error& e ) {
      mexErrMsgTxt( e.what() );
   }
}