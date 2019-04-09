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

#ifndef COMGR_DATA_H_
#define COMGR_DATA_H_

#include "amd_comgr.h"
#include "comgr-msgpack.h"
#include "comgr-symbol.h"
#include "yaml-cpp/yaml.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Object/ObjectFile.h"

namespace COMGR {
struct DataMeta;
struct DataSymbol;

/// Update @p Dest to point to a newly allocated C-style (null terminated)
/// string with the contents of @p Src, optionally updating @p Size with the
/// length of the string (not including the null terminator).
///
/// If @p Dest is non-null, it will first be freed.
///
/// @p Src may contain null bytes.
amd_comgr_status_t setCStr(char *&Dest, llvm::StringRef Src,
                           size_t *Size = nullptr);

/// Components of a "Code Object Target Identification" string.
///
/// See https://llvm.org/docs/AMDGPUUsage.html#code-object-target-identification
/// for details.
struct TargetIdentifier {
  llvm::StringRef Arch;
  llvm::StringRef Vendor;
  llvm::StringRef OS;
  llvm::StringRef Environ;
  llvm::StringRef Processor;
  llvm::SmallVector<llvm::StringRef, 2> Features;
};

/// Parse a "Code Object Target Identification" string into it's components.
///
/// See https://llvm.org/docs/AMDGPUUsage.html#code-object-target-identification
/// for details.
///
/// @param IdentStr [in] The string to parse.
/// @param Ident [out] The components of the identification string.
amd_comgr_status_t parseTargetIdentifier(llvm::StringRef IdentStr,
                                         TargetIdentifier &Ident);

/// Ensure all required LLVM initialization functions have been invoked at least
/// once in this process.
void ensureLLVMInitialized();

/// Reset all `llvm::cl` options to their default values.
void clearLLVMOptions();

/// Return `true` if the kind is valid, or false otherwise.
bool isDataKindValid(amd_comgr_data_kind_t DataKind);

struct DataObject {

  // Allocate a new DataObject and return a pointer to it.
  static DataObject *allocate(amd_comgr_data_kind_t DataKind);

  // Decrement the refcount of this DataObject, and free it when it reaches 0.
  void release();

  static amd_comgr_data_t convert(DataObject *Data) {
    amd_comgr_data_t Handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Data))};
    return Handle;
  }

  static const amd_comgr_data_t convert(const DataObject *Data) {
    const amd_comgr_data_t Handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Data))};
    return Handle;
  }

  static DataObject *convert(amd_comgr_data_t Data) {
    return reinterpret_cast<DataObject *>(Data.handle);
  }

  bool hasValidDataKind() { return isDataKindValid(DataKind); }

  amd_comgr_status_t setName(llvm::StringRef Name);
  amd_comgr_status_t setData(llvm::StringRef Data);
  void setMetadata(DataMeta *Metadata);

  amd_comgr_data_kind_t DataKind;
  char *Data;
  char *Name;
  size_t Size;
  int RefCount;
  DataSymbol *DataSym;

private:
  // We require this type be allocated via new, specifically through calling
  // allocate, because we want to be able to `delete this` in release. To make
  // sure the type is not constructed without new, or destructed without
  // checking the reference count, we mark the constructor and destructor
  // private.
  DataObject(amd_comgr_data_kind_t Kind);
  ~DataObject();
};

/// Should be used to ensure references to transient data objects are properly
/// released when they go out of scope.
class ScopedDataObjectReleaser {
  DataObject *Obj;

public:
  ScopedDataObjectReleaser(DataObject *Obj) : Obj(Obj) {}

  ScopedDataObjectReleaser(amd_comgr_data_t Obj)
      : Obj(DataObject::convert(Obj)) {}

  ~ScopedDataObjectReleaser() { Obj->release(); }
};

struct DataSet {

  DataSet();
  ~DataSet();

  static amd_comgr_data_set_t convert(DataSet *Set) {
    amd_comgr_data_set_t Handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Set))};
    return Handle;
  }

  static const amd_comgr_data_set_t convert(const DataSet *Set) {
    const amd_comgr_data_set_t Handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Set))};
    return Handle;
  }

  static DataSet *convert(amd_comgr_data_set_t Set) {
    return reinterpret_cast<DataSet *>(Set.handle);
  }

  llvm::SmallSetVector<DataObject *, 8> DataObjects;
};

struct DataAction {
  // Some actions involving llvm we want to do it only once for the entire
  // duration of the COMGR library. Once initialized, they should never be
  // reset.

  DataAction();
  ~DataAction();

  static amd_comgr_action_info_t convert(DataAction *Action) {
    amd_comgr_action_info_t Handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Action))};
    return Handle;
  }

  static const amd_comgr_action_info_t convert(const DataAction *Action) {
    const amd_comgr_action_info_t Handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Action))};
    return Handle;
  }

  static DataAction *convert(amd_comgr_action_info_t Action) {
    return reinterpret_cast<DataAction *>(Action.handle);
  }

  amd_comgr_status_t setIsaName(llvm::StringRef IsaName);
  amd_comgr_status_t setActionOptions(llvm::StringRef ActionOptions);
  amd_comgr_status_t setActionPath(llvm::StringRef ActionPath);

  char *IsaName;
  char *Options;
  char *Path;
  amd_comgr_language_t Language;
  bool Logging;
};

struct DataMeta {
  static amd_comgr_metadata_node_t convert(DataMeta *Meta) {
    amd_comgr_metadata_node_t Handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Meta))};
    return Handle;
  }

  static const amd_comgr_metadata_node_t convert(const DataMeta *Meta) {
    const amd_comgr_metadata_node_t Handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Meta))};
    return Handle;
  }

  static DataMeta *convert(amd_comgr_metadata_node_t Meta) {
    return reinterpret_cast<DataMeta *>(Meta.handle);
  }

  amd_comgr_metadata_kind_t getMetadataKind();

  YAML::Node YAMLNode;
  std::shared_ptr<msgpack::Node> MsgPackNode;
};

struct DataSymbol {
  DataSymbol(SymbolContext *DataSym);
  ~DataSymbol();

  static amd_comgr_symbol_t convert(DataSymbol *Sym) {
    amd_comgr_symbol_t Handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Sym))};
    return Handle;
  }

  static const amd_comgr_symbol_t convert(const DataSymbol *Sym) {
    const amd_comgr_symbol_t Handle = {
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Sym))};
    return Handle;
  }

  static DataSymbol *convert(amd_comgr_symbol_t Sym) {
    return reinterpret_cast<DataSymbol *>(Sym.handle);
  }

  SymbolContext *DataSym;
};

} // namespace COMGR

#endif // header guard
