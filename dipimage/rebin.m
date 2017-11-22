%REBIN   Rebinning of an image
%
%  Reduces size of an image by an integer factor, by grouping (binning)
%  pixels together. Each output pixel is the sum of a group of input
%  pixels.
%
% SYNOPSIS:
%  out = rebin(in, binning)
%
% PARAMETERS:
%  in:  input image
%  binning: integer numbers (array) that divides the image without remainder
%
% DEFAULTS:
%  binning = 2
%
% SEE ALSO:
%  resample, subsample

% (c)2017, Cris Luengo.
% Based on original DIPimage code: (c)1999-2014, Delft University of Technology.
%
% Licensed under the Apache License, Version 2.0 (the "License");
% you may not use this file except in compliance with the License.
% You may obtain a copy of the License at
%
%    http://www.apache.org/licenses/LICENSE-2.0
%
% Unless required by applicable law or agreed to in writing, software
% distributed under the License is distributed on an "AS IS" BASIS,
% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
% See the License for the specific language governing permissions and
% limitations under the License.

function out = rebin(in,binning)
if nargin<2
   binning = 2;
end
if ~isa(in,'dip_image')
   in = dip_image(in);
end
szI = imsize(in);
N = numel(szI);
if numel(binning)==N
   binning = reshape(binning,1,N);
elseif numel(binning)==1
   binning = repmat(binning,1,N);
else
   error('BINNING must be a scalar or have one element per image dimension');
end
if any(rem(binning,1))
   error('BINNING must be integer');
end
if any(rem(szI,binning))
   error('Binning must be divider of all image dimensions.');
end
if N>1
   % X and Y dims are swapped in memory
   szI = szI([2,1,3:end]);
   binning = binning([2,1,3:end]);
end
szO = szI./binning;
szX = [binning;szO];
szX = szX(:)';
szX = szX([2,1,3:end]); % swap X and Y
out = reshape(in,szX);
dims = 1:2:2*N;
dims(1) = 2;
out = sum(out,[],dims);
if N>1
   szO = szO([2,1,3:end]); % swap X and Y
end
out = reshape(out,szO);
