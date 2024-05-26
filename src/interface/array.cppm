/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "udm_definitions.hpp"
#include <sharedutils/magic_enum.hpp>
#include <iterator>

export module udm.array;
import udm.types;
import udm.type_structs;
import udm.trivial_types;
import udm.property_wrapper;
import udm.iterators;
export namespace udm {
	template<typename T>
	class ArrayIterator;

	struct DLLUDM Array {
		virtual ~Array();
		PropertyWrapper fromProperty {};

		bool operator==(const Array &other) const;
		bool operator!=(const Array &other) const { return !operator==(other); }

		void Merge(const Array &other, MergeFlags mergeFlags = MergeFlags::OverwriteExisting);

		virtual Array &operator=(Array &&other);
		virtual Array &operator=(const Array &other);

		virtual void SetValueType(Type valueType);
		virtual ArrayType GetArrayType() const { return ArrayType::Raw; }
		bool IsValueType(Type pvalueType) const { return pvalueType == m_valueType; }
		Type GetValueType() const { return m_valueType; }
		uint32_t GetSize() const { return m_size; }
		uint32_t GetValueSize() const;
		virtual void *GetValues() { return GetValuePtr(); }
		const void *GetValues() const { return const_cast<Array *>(this)->GetValues(); }
		void Resize(uint32_t newSize);
		void AddValueRange(uint32_t startIndex, uint32_t count);
		void RemoveValueRange(uint32_t startIndex, uint32_t count);
		void *GetValuePtr(uint32_t idx);
		template<typename T>
		T *GetValuePtr(uint32_t idx);
		template<typename T>
		const T *GetValuePtr(uint32_t idx) const
		{
			return const_cast<Array *>(this)->GetValuePtr<T>(idx);
		}
		PropertyWrapper operator[](uint32_t idx);
		const PropertyWrapper operator[](uint32_t idx) const { return const_cast<Array *>(this)->operator[](idx); }
		template<typename T>
		T &GetValue(uint32_t idx);
		template<typename T>
		const T &GetValue(uint32_t idx) const
		{
			return const_cast<Array *>(this)->GetValue<T>(idx);
		}
		template<typename T>
		void SetValue(uint32_t idx, T &&value);
		template<typename T>
		void InsertValue(uint32_t idx, T &&value);
		void RemoveValue(uint32_t idx);
		// The caller is responsible to ensure that the type of value matches the value type of the array!
		void SetValue(uint32_t idx, const void *value);
		void InsertValue(uint32_t idx, void *value);

		bool IsEmpty() const { return m_size == 0; }
		template<typename T>
		T *GetFront()
		{
			return !IsEmpty() ? GetValuePtr<T>(0u) : nullptr;
		}
		template<typename T>
		const T *GetFront() const
		{
			return const_cast<Array *>(this)->GetFront<T>();
		}
		template<typename T>
		T *GetBack()
		{
			return !IsEmpty() ? GetValuePtr<T>(m_size - 1) : nullptr;
		}
		template<typename T>
		const T *GetBack() const
		{
			return const_cast<Array *>(this)->GetBack<T>();
		}

		template<typename T>
		ArrayIterator<T> begin()
		{
			return ArrayIterator<T> {*this};
		}
		template<typename T>
		ArrayIterator<T> end()
		{
			return ArrayIterator<T> {*this, GetSize()};
		}
		ArrayIterator<LinkedPropertyWrapper> begin() { return begin<LinkedPropertyWrapper>(); }
		ArrayIterator<LinkedPropertyWrapper> end() { return end<LinkedPropertyWrapper>(); }

		uint64_t GetByteSize() const;
		const StructDescription *GetStructuredDataInfo() const { return const_cast<Array *>(this)->GetStructuredDataInfo(); }
		virtual StructDescription *GetStructuredDataInfo();

		static constexpr bool IsValueTypeSupported(Type type) { return true; }

		using Range = std::tuple<uint32_t, uint32_t, uint32_t>;
		void Resize(uint32_t newSize, Range r0, Range r1, bool defaultInitializeNewValues);
	  protected:
		friend Property;
		friend PropertyWrapper;
		virtual void Clear();

		void *GetValuePtr();
		const void *GetValuePtr() const { return const_cast<Array *>(this)->GetValuePtr(); }
		void *GetHeaderPtr();
		const void *GetHeaderPtr() const { return const_cast<Array *>(this)->GetHeaderPtr(); }
		uint64_t GetHeaderSize() const;
		void ReleaseValues();
		uint8_t *AllocateData(uint64_t size) const;

		void *m_values = nullptr;
		uint32_t m_size = 0;
		Type m_valueType = Type::Nil;
	};

	struct DLLUDM ArrayLz4 : public Array {
		enum class State : uint8_t { Compressed = 0, Uncompressed };

		ArrayLz4() = default;
		virtual ArrayLz4 &operator=(Array &&other) override;
		virtual ArrayLz4 &operator=(const Array &other) override;
		ArrayLz4 &operator=(ArrayLz4 &&other);
		ArrayLz4 &operator=(const ArrayLz4 &other);
		const BlobLz4 &GetCompressedBlob() const { return const_cast<ArrayLz4 *>(this)->GetCompressedBlob(); }
		BlobLz4 &GetCompressedBlob();
		virtual void *GetValues() override;
		virtual void SetValueType(Type valueType) override;
		virtual ArrayType GetArrayType() const { return ArrayType::Compressed; }
		void ClearUncompressedMemory();
		using Array::GetStructuredDataInfo;

		static constexpr bool IsValueTypeSupported(Type type);
	  private:
		friend Property;
		friend PropertyWrapper;
		friend AsciiReader;
		virtual StructDescription *GetStructuredDataInfo() override;
		void InitializeSize(uint32_t size);
		void Decompress();
		void Compress();
		virtual void Clear() override;
		std::unique_ptr<StructDescription> m_structuredDataInfo = nullptr;
		State m_state = State::Uncompressed;
		BlobLz4 m_compressedBlob {};
	};

	template<typename T>
	void Array::InsertValue(uint32_t idx, T &&value)
	{
		auto size = GetSize();
		if(idx > size)
			return;
		Range r0 {0 /* src */, 0 /* dst */, idx};
		Range r1 {idx /* src */, idx + 1 /* dst */, size - idx};
		Resize(size + 1, r0, r1, false);
		if constexpr(std::is_rvalue_reference_v<T>)
			(*this)[idx] = std::move(value);
		else
			(*this)[idx] = value;
	}

	template<typename T>
	T *Array::GetValuePtr(uint32_t idx)
	{
		if(type_to_enum<T>() != m_valueType)
			return nullptr;
		return &GetValue<T>(idx);
	}

	template<typename T>
	T &Array::GetValue(uint32_t idx)
	{
		if(idx >= m_size)
			throw OutOfBoundsError {"Array index " + std::to_string(idx) + " out of bounds of array of size " + std::to_string(m_size) + "!"};
		using TBase = std::remove_cv_t<std::remove_reference_t<T>>;
		auto vs = [this, idx](auto tag) -> T & {
			using TTag = typename decltype(tag)::type;
			if constexpr(std::is_same_v<TTag, TBase>)
				return static_cast<TTag *>(GetValues())[idx];
			throw LogicError {"Attempted to retrieve value of type " + std::string {magic_enum::enum_name(type_to_enum<T>())} + " from array of type " + std::string {magic_enum::enum_name(m_valueType)} + "!"};
		};
		auto valueType = GetValueType();
		return visit(valueType, vs);
	}

	template<typename T>
	void Array::SetValue(uint32_t idx, T &&v)
	{
		using TBase = std::remove_cv_t<std::remove_reference_t<T>>;
		auto valueType = GetValueType();
		if(valueType == Type::Struct) {
			if constexpr(!std::is_fundamental_v<std::remove_extent_t<TBase>>) {
				auto sz = GetStructuredDataInfo()->GetDataSizeRequirement();
				if(sizeof(T) != sz)
					throw LogicError {"Attempted to assign data of size " + std::to_string(sizeof(T)) + " to struct of size " + std::to_string(sz) + "!"};
				if constexpr(std::is_rvalue_reference_v<T>)
					static_cast<TBase *>(GetValues())[idx] = std::move(v);
				else
					static_cast<TBase *>(GetValues())[idx] = v;
			}
			else
				throw LogicError {"Attempted to assign fundamental type '" + std::string {typeid(T).name()} + "' to struct, this is not allowed!"};
			return;
		}
		if(!is_convertible<TBase>(valueType)) {
			throw LogicError {"Attempted to insert value of type " + std::string {magic_enum::enum_name(type_to_enum_s<TBase>())} + " into array of type " + std::string {magic_enum::enum_name(valueType)} + ", which are not compatible!"};
		}

		auto vs = [this, idx, &v](auto tag) {
			using TTag = typename decltype(tag)::type;
			if constexpr(is_convertible<TBase, TTag>())
				static_cast<TTag *>(GetValues())[idx] = convert<TBase, TTag>(v);
		};
		visit(valueType, vs);
	}

	constexpr bool ArrayLz4::IsValueTypeSupported(Type type) { return is_numeric_type(type) || is_generic_type(type) || type == Type::Struct || type == Type::Element || type == Type::String; }
}