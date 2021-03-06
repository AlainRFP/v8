// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_DESCRIPTOR_ARRAY_H_
#define V8_OBJECTS_DESCRIPTOR_ARRAY_H_

#include "src/objects.h"
#include "src/objects/fixed-array.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

template <typename T>
class Handle;

class Isolate;

// An EnumCache is a pair used to hold keys and indices caches.
class EnumCache : public Tuple2 {
 public:
  DECL_ACCESSORS2(keys, FixedArray)
  DECL_ACCESSORS2(indices, FixedArray)

  DECL_CAST(EnumCache)

  // Layout description.
  static const int kKeysOffset = kValue1Offset;
  static const int kIndicesOffset = kValue2Offset;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(EnumCache);
};

// A DescriptorArray is a custom array that holds instance descriptors.
// It has the following layout:
//   Header:
//     [16:0  bits]: number_of_all_descriptors (including slack)
//     [32:16 bits]: number_of_descriptors
//     [48:32 bits]: number_of_marked_descriptors (used by GC)
//     [64:48 bits]: alignment filler
//     [kEnumCacheOffset]: enum cache
//   Elements:
//     [kHeaderSize + 0]: first key (and internalized String)
//     [kHeaderSize + 1]: first descriptor details (see PropertyDetails)
//     [kHeaderSize + 2]: first value for constants / Smi(1) when not used
//   Slack:
//     [kHeaderSize + number of descriptors * 3]: start of slack
// The "value" fields store either values or field types. A field type is either
// FieldType::None(), FieldType::Any() or a weak reference to a Map. All other
// references are strong.
class DescriptorArray : public HeapObjectPtr {
 public:
  DECL_INT16_ACCESSORS(number_of_all_descriptors)
  DECL_INT16_ACCESSORS(number_of_descriptors)
  inline int16_t number_of_slack_descriptors() const;
  inline int number_of_entries() const;
  DECL_ACCESSORS(enum_cache, EnumCache)

  void ClearEnumCache();
  inline void CopyEnumCacheFrom(DescriptorArray array);
  static void InitializeOrChangeEnumCache(Handle<DescriptorArray> descriptors,
                                          Isolate* isolate,
                                          Handle<FixedArray> keys,
                                          Handle<FixedArray> indices);

  // Accessors for fetching instance descriptor at descriptor number.
  inline Name GetKey(int descriptor_number) const;
  inline Object* GetStrongValue(int descriptor_number);
  inline void SetValue(int descriptor_number, Object* value);
  inline MaybeObject GetValue(int descriptor_number);
  inline PropertyDetails GetDetails(int descriptor_number);
  inline int GetFieldIndex(int descriptor_number);
  inline FieldType GetFieldType(int descriptor_number);

  inline Name GetSortedKey(int descriptor_number);
  inline int GetSortedKeyIndex(int descriptor_number);
  inline void SetSortedKey(int pointer, int descriptor_number);

  // Accessor for complete descriptor.
  inline void Set(int descriptor_number, Descriptor* desc);
  inline void Set(int descriptor_number, Name key, MaybeObject value,
                  PropertyDetails details);
  void Replace(int descriptor_number, Descriptor* descriptor);

  // Generalizes constness, representation and field type of all field
  // descriptors.
  void GeneralizeAllFields();

  // Append automatically sets the enumeration index. This should only be used
  // to add descriptors in bulk at the end, followed by sorting the descriptor
  // array.
  inline void Append(Descriptor* desc);

  static Handle<DescriptorArray> CopyUpTo(Isolate* isolate,
                                          Handle<DescriptorArray> desc,
                                          int enumeration_index, int slack = 0);

  static Handle<DescriptorArray> CopyUpToAddAttributes(
      Isolate* isolate, Handle<DescriptorArray> desc, int enumeration_index,
      PropertyAttributes attributes, int slack = 0);

  static Handle<DescriptorArray> CopyForFastObjectClone(
      Isolate* isolate, Handle<DescriptorArray> desc, int enumeration_index,
      int slack = 0);

  // Sort the instance descriptors by the hash codes of their keys.
  void Sort();

  // Search the instance descriptors for given name.
  V8_INLINE int Search(Name name, int number_of_own_descriptors);
  V8_INLINE int Search(Name name, Map map);

  // As the above, but uses DescriptorLookupCache and updates it when
  // necessary.
  V8_INLINE int SearchWithCache(Isolate* isolate, Name name, Map map);

  bool IsEqualUpTo(DescriptorArray desc, int nof_descriptors);

  // Allocates a DescriptorArray, but returns the singleton
  // empty descriptor array object if number_of_descriptors is 0.
  static Handle<DescriptorArray> Allocate(Isolate* isolate, int nof_descriptors,
                                          int slack);

  void Initialize(EnumCache* enum_cache, HeapObject* undefined_value,
                  int nof_descriptors, int slack);

  DECL_CAST2(DescriptorArray)

  // Constant for denoting key was not found.
  static const int kNotFound = -1;

  // Layout description.
#define DESCRIPTOR_ARRAY_FIELDS(V)                 \
  V(kNumberOfAllDescriptorsOffset, kUInt16Size)    \
  V(kNumberOfDescriptorsOffset, kUInt16Size)       \
  V(kNumberOfMarkedDescriptorsOffset, kUInt16Size) \
  V(kFiller16BitsOffset, kUInt16Size)              \
  V(kPointersStartOffset, 0)                       \
  V(kEnumCacheOffset, kTaggedSize)                 \
  V(kHeaderSize, 0)

  DEFINE_FIELD_OFFSET_CONSTANTS(HeapObject::kHeaderSize,
                                DESCRIPTOR_ARRAY_FIELDS)
#undef DESCRIPTOR_ARRAY_FIELDS

  STATIC_ASSERT(IsAligned(kPointersStartOffset, kTaggedSize));
  STATIC_ASSERT(IsAligned(kHeaderSize, kTaggedSize));

  // Garbage collection support.
  DECL_INT16_ACCESSORS(number_of_marked_descriptors)
  static constexpr int SizeFor(int number_of_all_descriptors) {
    return offset(number_of_all_descriptors * kEntrySize);
  }
  static constexpr int OffsetOfDescriptorAt(int descriptor) {
    return offset(descriptor * kEntrySize);
  }
  inline ObjectSlot GetFirstPointerSlot();
  inline ObjectSlot GetDescriptorSlot(int descriptor);
  inline ObjectSlot GetKeySlot(int descriptor);
  inline MaybeObjectSlot GetValueSlot(int descriptor);

  typedef FlexibleWeakBodyDescriptor<kPointersStartOffset> BodyDescriptor;

  // Layout of descriptor.
  // Naming is consistent with Dictionary classes for easy templating.
  static const int kEntryKeyIndex = 0;
  static const int kEntryDetailsIndex = 1;
  static const int kEntryValueIndex = 2;
  static const int kEntrySize = 3;

  // Print all the descriptors.
  void PrintDescriptors(std::ostream& os);
  void PrintDescriptorDetails(std::ostream& os, int descriptor,
                              PropertyDetails::PrintMode mode);

  DECL_PRINTER(DescriptorArray)
  DECL_VERIFIER(DescriptorArray)

#ifdef DEBUG
  // Is the descriptor array sorted and without duplicates?
  bool IsSortedNoDuplicates(int valid_descriptors = -1);

  // Are two DescriptorArrays equal?
  bool IsEqualTo(DescriptorArray other);
#endif

  static constexpr int ToDetailsIndex(int descriptor_number) {
    return (descriptor_number * kEntrySize) + kEntryDetailsIndex;
  }

  // Conversion from descriptor number to array indices.
  static constexpr int ToKeyIndex(int descriptor_number) {
    return (descriptor_number * kEntrySize) + kEntryKeyIndex;
  }

  static constexpr int ToValueIndex(int descriptor_number) {
    return (descriptor_number * kEntrySize) + kEntryValueIndex;
  }

 private:
  DECL_INT16_ACCESSORS(filler16bits)
  // Low-level per-element accessors.
  static constexpr int offset(int index) {
    return kHeaderSize + index * kTaggedSize;
  }
  inline int length() const;
  inline MaybeObject get(int index) const;
  inline void set(int index, MaybeObject value);

  // Transfer a complete descriptor from the src descriptor array to this
  // descriptor array.
  void CopyFrom(int index, DescriptorArray src);

  // Swap first and second descriptor.
  inline void SwapSortedKeys(int first, int second);

  OBJECT_CONSTRUCTORS(DescriptorArray, HeapObjectPtr);
};

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_DESCRIPTOR_ARRAY_H_
