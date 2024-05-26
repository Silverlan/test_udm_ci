/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "udm_definitions.hpp"
#include <sharedutils/util.h>
#include <sharedutils/magic_enum.hpp>
#include <cinttypes>
#include <string_view>
#include <optional>
#include <vector>
#include <map>

export module udm.property_wrapper;
import udm.types;
import udm.trivial_types;
import udm.enums;
export namespace udm {
	template<typename T>
	class ArrayIterator;
	struct DLLUDM PropertyWrapper {
		PropertyWrapper() = default;
		PropertyWrapper(Property &o);
		PropertyWrapper(const PropertyWrapper &other);
		PropertyWrapper(Array &array, uint32_t idx);
		template<typename T>
		void operator=(T &&v) const;
		void operator=(const PropertyWrapper &other);
		void operator=(PropertyWrapper &other);
		void operator=(PropertyWrapper &&other);
		void operator=(const LinkedPropertyWrapper &other);
		void operator=(LinkedPropertyWrapper &other);
		void operator=(Property &other);
		//template<typename T>
		//	operator T() const;
		LinkedPropertyWrapper Add(const std::string_view &path, Type type = Type::Element, bool pathToElements = false) const;
		LinkedPropertyWrapper AddArray(const std::string_view &path, std::optional<uint32_t> size = {}, Type type = Type::Element, ArrayType arrayType = ArrayType::Raw, bool pathToElements = false) const;
		LinkedPropertyWrapper AddArray(const std::string_view &path, StructDescription &&strct, std::optional<uint32_t> size = {}, ArrayType arrayType = ArrayType::Raw, bool pathToElements = false) const;
		LinkedPropertyWrapper AddArray(const std::string_view &path, const StructDescription &strct, std::optional<uint32_t> size = {}, ArrayType arrayType = ArrayType::Raw, bool pathToElements = false) const;
		template<typename T>
		LinkedPropertyWrapper AddArray(const std::string_view &path, const StructDescription &strct, const T *data, uint32_t strctItems, ArrayType arrayType = ArrayType::Raw, bool pathToElements = false) const;
		template<typename T>
		LinkedPropertyWrapper AddArray(const std::string_view &path, const StructDescription &strct, const std::vector<T> &values, ArrayType arrayType = ArrayType::Raw, bool pathToElements = false) const;
		template<typename T>
		LinkedPropertyWrapper AddArray(const std::string_view &path, const std::vector<T> &values, ArrayType arrayType = ArrayType::Raw, bool pathToElements = false) const;
		template<typename T>
		LinkedPropertyWrapper AddArray(const std::string_view &path, uint32_t size, const T *data, ArrayType arrayType = ArrayType::Raw, bool pathToElements = false) const;
		bool IsArrayItem() const;
		bool IsType(Type type) const;
		Type GetType() const;
		void Merge(const PropertyWrapper &other, MergeFlags mergeFlags = MergeFlags::OverwriteExisting) const;

		Array *GetOwningArray() const;
		BlobResult GetBlobData(void *outBuffer, size_t bufferSize, uint64_t *optOutRequiredSize = nullptr) const;
		BlobResult GetBlobData(void *outBuffer, size_t bufferSize, Type type, uint64_t *optOutRequiredSize = nullptr) const;
		Blob GetBlobData(Type &outType) const;
		template<class T>
		BlobResult GetBlobData(T &v) const;
		template<typename T>
		T &GetValue() const;
		template<typename T>
		T *GetValuePtr() const;
		void *GetValuePtr(Type &outType) const;
		template<typename T>
		T ToValue(const T &defaultValue, bool *optOutIsDefined = nullptr) const;
		template<typename T>
		std::optional<T> ToValue() const;
		template<typename T>
		bool operator>>(T &valOut) const
		{
			return (*this)(valOut);
		}
		template<typename T>
		T operator()(const T &defaultValue) const
		{
			return ToValue<T>(defaultValue);
		}
		template<typename T>
		T operator()(const T &defaultValue, bool &outIsDefined) const
		{
			return ToValue<T>(defaultValue, &outIsDefined);
		}
		template<typename T>
		bool operator()(T &valOut) const
		{
			using TBase = std::remove_reference_t<T>;
			if constexpr(util::is_specialization<TBase, std::optional>::value) {
				typename TBase::value_type v;
				if(!(*this)(v)) {
					valOut = {};
					return false;
				}
				valOut = v;
				return true;
			}
			else if constexpr(util::is_specialization<TBase, std::vector>::value || util::is_specialization<TBase, std::map>::value || util::is_specialization<TBase, std::unordered_map>::value) {
				bool isDefined;
				valOut = std::move((*this)(const_cast<const T &>(valOut), isDefined));
				return isDefined;
			}
			else if constexpr(std::is_enum_v<TBase>) {
				using TEnum = TBase;
				auto *ptr = GetValuePtr<std::string>();
				if(ptr) {
					auto e = magic_enum::enum_cast<TEnum>(*ptr);
					if(!e.has_value())
						return false;
					valOut = *e;
					return true;
				}
				bool isDefined;
				valOut = static_cast<TEnum>((*this)(reinterpret_cast<const std::underlying_type_t<TEnum> &>(valOut), isDefined));
				return isDefined;
			}
			else if constexpr(std::is_same_v<TBase, PProperty>) {
				if(!prop)
					return false;
				valOut->Assign<false>(*prop);
				return true;
			}
			else if constexpr(std::is_same_v<TBase, Property>) {
				if(!prop)
					return false;
				valOut.Assign<false>(*prop);
				return true;
			}
			else {
				auto *ptr = GetValuePtr<T>();
				if(ptr) {
					valOut = *ptr;
					return true;
				}
				bool isDefined;
				valOut = (*this)(const_cast<const T &>(valOut), isDefined);
				return isDefined;
			}
		}

		// For array properties
		uint32_t GetSize() const;
		void Resize(uint32_t size) const;
		template<typename T>
		ArrayIterator<T> begin() const;
		template<typename T>
		ArrayIterator<T> end() const;
		ArrayIterator<LinkedPropertyWrapper> begin() const;
		ArrayIterator<LinkedPropertyWrapper> end() const;
		LinkedPropertyWrapper operator[](uint32_t idx) const;
		LinkedPropertyWrapper operator[](int32_t idx) const;
		LinkedPropertyWrapper operator[](size_t idx) const;
		//

		// For element properties
		ElementIterator begin_el() const;
		ElementIterator end_el() const;
		uint32_t GetChildCount() const;
		//

		LinkedPropertyWrapper GetFromPath(const std::string_view &key) const;
		LinkedPropertyWrapper operator[](const std::string_view &key) const;
		LinkedPropertyWrapper operator[](const std::string &key) const;
		LinkedPropertyWrapper operator[](const char *key) const;
		bool operator==(const PropertyWrapper &other) const;
		bool operator!=(const PropertyWrapper &other) const;
		template<typename T>
		bool operator==(const T &other) const;
		template<typename T>
		bool operator!=(const T &other) const
		{
			return !operator==(other);
		}
		Property *operator*() const;
		Property *operator->() const { return operator*(); }
		operator bool() const;
		Property *prop = nullptr;
		uint32_t arrayIndex = std::numeric_limits<uint32_t>::max();

		LinkedPropertyWrapper *GetLinked();
		const LinkedPropertyWrapper *GetLinked() const { return const_cast<PropertyWrapper *>(this)->GetLinked(); };

		bool ImplIsLinked() const { return linked; }
		bool ImplIsArrayItem(bool includeIfElementOfArrayItem) const { return IsArrayItem(includeIfElementOfArrayItem); }
	  protected:
		bool IsArrayItem(bool includeIfElementOfArrayItem) const;
		bool linked = false;
	};

	struct DLLUDM LinkedPropertyWrapper : public PropertyWrapper {
		LinkedPropertyWrapper() : PropertyWrapper {} { linked = true; }
		LinkedPropertyWrapper(const LinkedPropertyWrapper &other);
		LinkedPropertyWrapper(const PropertyWrapper &other) : PropertyWrapper {other} { linked = true; }
		LinkedPropertyWrapper(Property &o) : PropertyWrapper {o} { linked = true; }
		LinkedPropertyWrapper(Array &array, uint32_t idx) : PropertyWrapper {array, idx} { linked = true; }
		bool operator==(const LinkedPropertyWrapper &other) const;
		bool operator!=(const LinkedPropertyWrapper &other) const;
		using PropertyWrapper::operator==;
		using PropertyWrapper::operator!=;
		template<typename T>
		void operator=(T &&v) const;
		void operator=(PropertyWrapper &&v);
		void operator=(LinkedPropertyWrapper &&v);
		void operator=(const PropertyWrapper &v);
		void operator=(const LinkedPropertyWrapper &v);

		// Alias
		template<typename T>
		void operator<<(T &&v);

		LinkedPropertyWrapper operator[](uint32_t idx) const;
		LinkedPropertyWrapper operator[](int32_t idx) const;
		LinkedPropertyWrapper operator[](size_t idx) const;

		LinkedPropertyWrapper operator[](const std::string_view &key) const;
		LinkedPropertyWrapper operator[](const std::string &key) const;
		LinkedPropertyWrapper operator[](const char *key) const;

		std::string GetPath() const;
		PProperty ClaimOwnership() const;
		std::unique_ptr<LinkedPropertyWrapper> prev = nullptr;
		std::string propName;

		// For internal use only!
		void InitializeProperty(Type type = Type::Element, bool getOnly = false);
		Property *GetProperty(std::vector<uint32_t> *optOutArrayIndices = nullptr) const;
	};
}

export namespace udm {
	// msvc currently has a bug where template methods cannot be defined in a separate implementation file than the one that contains
	// the class definition. We can't do that because it would cause a cyclic dependency.
	// As a work-around, we define some standalone functions (which are defined in property_wrapper_impl.cppm)
	// TODO: Remove these once the issue has been fixed in msvc
	// TODO: Also remove the methods prefixed with "Impl" from the UDM types.
	template<class T>
	BlobResult property_wrapper_get_blob_data(const PropertyWrapper &wrapper, T &v);
	template<typename T>
	void property_wrapper_operator_equal(const udm::PropertyWrapper &wrapper, T &&v);
	template<typename T>
	LinkedPropertyWrapper property_wrapper_add_array(const PropertyWrapper &wrapper, const std::string_view &path, const StructDescription &strct, const T *data, uint32_t strctItems, ArrayType arrayType, bool pathToElements);
	template<typename T>
	T &property_wrapper_get_value(const PropertyWrapper &wrapper);
	template<typename T>
	T *property_wrapper_get_value_ptr(const PropertyWrapper &wrapper);
	template<typename T>
	ArrayIterator<T> property_wrapper_begin(const PropertyWrapper &wrapper);
	template<typename T>
	ArrayIterator<T> property_wrapper_end(const PropertyWrapper &wrapper);
	template<typename T>
	std::optional<T> property_wrapper_to_value(const PropertyWrapper *wrapper);

	template<typename T>
	void udm::LinkedPropertyWrapper::operator=(T &&v) const
	{
		using TBase = std::remove_cv_t<std::remove_reference_t<T>>;
		if(prop == nullptr) {
			if constexpr(util::is_specialization<TBase, std::optional>::value) {
				if(!v) // nullopt
					return;
			}
			const_cast<LinkedPropertyWrapper *>(this)->InitializeProperty();
		}
		/*if(prev && prev->arrayIndex != std::numeric_limits<uint32_t>::max() && prev->prev && prev->prev->prop && prev->prev->prop->type == Type::Array)
		{
			(*static_cast<Array*>(prev->prev->prop->value))[prev->arrayIndex][propName] = v;
			return;
		}*/
		PropertyWrapper::operator=(v);
	}
	template<typename T>
	void udm::PropertyWrapper::operator=(T &&v) const
	{
		property_wrapper_operator_equal<T>(*this, std::forward<T>(v));
	}

	template<typename T>
	void udm::LinkedPropertyWrapper::operator<<(T &&v)
	{
		operator=(*this, v);
	}

	template<typename T>
	LinkedPropertyWrapper PropertyWrapper::AddArray(const std::string_view &path, const StructDescription &strct, const T *data, uint32_t strctItems, ArrayType arrayType, bool pathToElements) const
	{
		return property_wrapper_add_array<T>(*this, path, strct, data, strctItems, arrayType, pathToElements);
	}

	template<typename T>
	LinkedPropertyWrapper PropertyWrapper::AddArray(const std::string_view &path, const StructDescription &strct, const std::vector<T> &values, ArrayType arrayType, bool pathToElements) const
	{
		auto prop = AddArray(path, strct, values.size(), arrayType, pathToElements);
		auto &a = prop.template GetValue<Array>();
		auto sz = a.GetValueSize() * a.GetSize();
		auto szValues = util::size_of_container(values);
		if(szValues != sz)
			throw InvalidUsageError {"Size of values does not match expected size of defined struct!"};
		auto *ptr = a.GetValues();
		memcpy(ptr, values.data(), szValues);
		return prop;
	}

	template<typename T>
	LinkedPropertyWrapper PropertyWrapper::AddArray(const std::string_view &path, const std::vector<T> &values, ArrayType arrayType, bool pathToElements) const
	{
		return AddArray<T>(path, values.size(), values.data(), arrayType, pathToElements);
	}

	template<typename T>
	LinkedPropertyWrapper PropertyWrapper::AddArray(const std::string_view &path, uint32_t size, const T *data, ArrayType arrayType, bool pathToElements) const
	{
		constexpr auto valueType = type_to_enum<T>();
		auto prop = AddArray(path, size, valueType, arrayType, pathToElements);
		auto &a = prop.template GetValue<Array>();
		if constexpr(is_non_trivial_type(valueType) && valueType != Type::Struct) {
			for(auto i = decltype(size) {0u}; i < size; ++i)
				a[i] = data[i];
		}
		else
			memcpy(a.GetValues(), data, sizeof(T) * size);
		return prop;
	}

	template<class T>
	BlobResult PropertyWrapper::GetBlobData(T &v) const
	{
		return property_wrapper_get_blob_data<T>(*this, v);
	}
	template<typename T>
	T &PropertyWrapper::GetValue() const
	{
		return property_wrapper_get_value<T>(*this);
	}

	template<typename T>
	T *PropertyWrapper::GetValuePtr() const
	{
		return property_wrapper_get_value_ptr<T>(*this);
	}

	template<typename T>
	T PropertyWrapper::ToValue(const T &defaultValue, bool *optOutIsDefined) const
	{
		if(!this) // This can happen in chained expressions. TODO: This is technically undefined behavior and should be implemented differently!
		{
			if(optOutIsDefined)
				*optOutIsDefined = false;
			return defaultValue;
		}
		auto val = ToValue<T>();
		if(val.has_value()) {
			if(optOutIsDefined)
				*optOutIsDefined = true;
			return std::move(val.value());
		}
		if(optOutIsDefined)
			*optOutIsDefined = false;
		return defaultValue;
	}

	template<typename T>
	bool PropertyWrapper::operator==(const T &other) const
	{
		if constexpr(util::is_c_string<T>())
			return operator==(std::string {other});
		else {
			auto *val = GetValuePtr<T>();
			if(val)
				return *val == other;
			auto valConv = ToValue<T>();
			return valConv.has_value() ? *valConv == other : false;
		}
		return false;
	}

	template<typename T>
	ArrayIterator<T> PropertyWrapper::begin() const
	{
		return property_wrapper_begin<T>(*this);
	}
	template<typename T>
	ArrayIterator<T> PropertyWrapper::end() const
	{
		return property_wrapper_end<T>(*this);
	}

	template<typename T>
	std::optional<T> PropertyWrapper::ToValue() const
	{
		return property_wrapper_to_value<T>(this);
	}
}
