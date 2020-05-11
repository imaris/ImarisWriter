/***************************************************************************
 *   Copyright (c) 2020-present Bitplane AG Zuerich                        *
 *                                                                         *
 *   Licensed under the Apache License, Version 2.0 (the "License");       *
 *   you may not use this file except in compliance with the License.      *
 *   You may obtain a copy of the License at                               *
 *                                                                         *
 *       http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                         *
 *   Unless required by applicable law or agreed to in writing, software   *
 *   distributed under the License is distributed on an "AS IS" BASIS,     *
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or imp   *
 *   See the License for the specific language governing permissions and   *
 *   limitations under the License.                                        *
 ***************************************************************************/
#include "bpDeriche.h"


#include <cmath>


void bpDeriche::FilterGauss(const float* aIn, float* aOut, size_t aCount, float aSigma)
{
  const float* data_in = aIn;
  float* data_out = aOut;

  const float alpha = 1.695f / aSigma;
  float e = static_cast<float>(exp(-alpha));
  float e2 = static_cast<float>(exp(-2 * alpha));
  float norm = (1 - e) * (1 - e) / (1 + 2 * alpha * e - e2);

  float in0, in1, in2, out0, out1, out2;

  {
    // forward loop
    size_t count = aCount;

    float d2 = -e2;
    float d1 = 2 * e;
    float n0 = norm;
    float n1 = norm * (alpha - 1) * e;
    float n2 = 0;
    in1 = *data_in;
    in2 = *data_in;
    out1 = (n2 + n1 + n0)*in1 / (1.0f - d1 - d2);
    out2 = (n2 + n1 + n0)*in1 / (1.0f - d1 - d2);
    for (; count > 0; count--, data_out++, data_in++) {
      in0 = *data_in;
      out0 = n2*in2 + n1*in1 + n0*in0 + d1*out1 + d2*out2;
      in2 = in1;
      in1 = in0;
      out2 = out1;
      out1 = out0;
      *data_out = out0;
    }
  }

  data_in = aIn + (aCount - 1);
  data_out = aOut + (aCount - 1);

  {
    // backward loop
    size_t count = aCount;

    float d2 = -e2;
    float d1 = 2 * e;
    float n0 = 0;
    float n1 = norm * (alpha + 1) * e;
    float n2 = -norm * e2;
    in1 = *data_in;
    in2 = *data_in;
    out1 = (n2 + n1 + n0)*in1 / (1.0f - d1 - d2);
    out2 = (n2 + n1 + n0)*in1 / (1.0f - d1 - d2);
    for (; count > 0; count--, data_out--, data_in--) {
      in0 = *data_in;
      out0 = n2*in2 + n1*in1 + n0*in0 + d1*out1 + d2*out2;
      in2 = in1;
      in1 = in0;
      out2 = out1;
      out1 = out0;
      *data_out += out0;
    }
  }
}
