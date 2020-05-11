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
#include "bpH5LZ4.h"


static const int H5Z_FILTER_LZ4 = 32004;

static size_t H5Z_filter_lz4(unsigned int flags, size_t cd_nelmts,
  const unsigned int cd_values[], size_t nbytes,
  size_t *buf_size, void **buf)
{
  return 0; // please execute externally and call H5Dwrite_chunk with filters 0
}


const H5Z_class2_t H5Z_LZ4[1] = { {
    H5Z_CLASS_T_VERS,       /* H5Z_class_t version */
    (H5Z_filter_t)H5Z_FILTER_LZ4,         /* Filter id number             */
    1,              /* encoder_present flag (set to true) */
    1,              /* decoder_present flag (set to true) */
    "HDF5 lz4 filter; see http://www.hdfgroup.org/services/contributions.html",
    /* Filter name for debugging    */
    NULL,                       /* The "can apply" callback     */
    NULL,                       /* The "set local" callback     */
    (H5Z_func_t)H5Z_filter_lz4,         /* The actual filter function   */
  }
};

static htri_t H5Zregister_lz4_impl()
{
  if (H5Zfilter_avail(H5Z_FILTER_LZ4) > 0) {
    // already dynamically loaded?
    // https://support.hdfgroup.org/HDF5/doc/Advanced/DynamicallyLoadedFilters/HDF5DynamicallyLoadedFilters.pdf
    return 1;
  }
  if (H5Zregister(H5Z_LZ4) < 0) {
    return -1;
  }
  return H5Zfilter_avail(H5Z_FILTER_LZ4);
}


htri_t H5Zregister_lz4()
{
  static htri_t vRegistered = H5Zregister_lz4_impl();
  return vRegistered;
}


herr_t H5Pset_lz4(hid_t aPListId, unsigned int aBlockSize)
{
  if (H5Zregister_lz4() < 0) {
    return -1;
  }
  return H5Pset_filter(aPListId, H5Z_FILTER_LZ4, H5Z_FLAG_MANDATORY, 1, &aBlockSize);
}

