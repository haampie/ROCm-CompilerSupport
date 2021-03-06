/*******************************************************************************
 *
 * University of Illinois/NCSA
 * Open Source License
 *
 * Copyright (c) 2018 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimers.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimers in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the names of Advanced Micro Devices, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
 * THE SOFTWARE.
 *
 ******************************************************************************/

#include "amd_comgr.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  amd_comgr_status_t status;

  // how many isa_names do we support?
  size_t isaCounts;
  status = amd_comgr_get_isa_count(&isaCounts);
  checkError(status, "amd_comgr_get_isa_count");
  printf("isa count = %zu\n\n", isaCounts);

  // print the list
  printf("*** List of ISA names supported:\n");
  for (size_t i = 0; i < isaCounts; i++) {
    const char *name;
    status = amd_comgr_get_isa_name(i, &name);
    checkError(status, "amd_comgr_get_isa_name");
    printf("%zu: %s\n", i, name);
    amd_comgr_metadata_node_t meta;
    status = amd_comgr_get_isa_metadata(name, &meta);
    checkError(status, "amd_comgr_get_isa_metadata");
    int indent = 1;
    status = amd_comgr_iterate_map_metadata(meta, printEntry, (void *)&indent);
    checkError(status, "amd_comgr_iterate_map_metadata");
    status = amd_comgr_destroy_metadata(meta);
    checkError(status, "amd_comgr_destroy_metadata");
  }

  return 0;
}
