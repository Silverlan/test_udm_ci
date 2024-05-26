/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "udm_definitions.hpp"
#include <sharedutils/util.h>
#include <sharedutils/util_string_hash.hpp>
#include <sharedutils/magic_enum.hpp>
#include <mathutil/uvec.h>
#include <mathutil/transform.hpp>
#include <cinttypes>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

export module udm.type_structs;
import udm.exceptions;
import udm.enums;
import udm.types;
import udm.basic_types;
export namespace udm {
#pragma pack(push, 1)
	struct DLLUDM Half {
		Half() = default;
		Half(uint16_t value) : value {value} {}
		Half(const Half &other) = default;
		Half(float value);
		operator float() const;
		Half &operator=(float value);
		Half &operator=(uint16_t value);
		Half &operator=(const Half &other) = default;
		uint16_t value;
	};
#pragma pack(pop)
	static_assert(sizeof(Half) == sizeof(uint16_t));

	struct DLLUDM Blob {
		Blob() = default;
		Blob(const Blob &) = default;
		Blob(Blob &&) = default;
		Blob(std::vector<uint8_t> &&data) : data {data} {}
		std::vector<uint8_t> data;

		Blob &operator=(Blob &&other);
		Blob &operator=(const Blob &other);

		bool operator==(const Blob &other) const
		{
			auto res = (data == other.data);
			UDM_ASSERT_COMPARISON(res);
			return res;
		}
		bool operator!=(const Blob &other) const { return !operator==(other); }
	};

	struct DLLUDM BlobLz4 {
		BlobLz4() = default;
		BlobLz4(const BlobLz4 &) = default;
		BlobLz4(BlobLz4 &&) = default;
		BlobLz4(std::vector<uint8_t> &&compressedData, size_t uncompressedSize) : compressedData {compressedData}, uncompressedSize {uncompressedSize} {}
		size_t uncompressedSize = 0;
		std::vector<uint8_t> compressedData;

		BlobLz4 &operator=(BlobLz4 &&other);
		BlobLz4 &operator=(const BlobLz4 &other);

		bool operator==(const BlobLz4 &other) const
		{
			auto res = (uncompressedSize == other.uncompressedSize && compressedData == other.compressedData);
			UDM_ASSERT_COMPARISON(res);
			return res;
		}
		bool operator!=(const BlobLz4 &other) const { return !operator==(other); }
	};

	struct DLLUDM Utf8String {
		Utf8String() = default;
		Utf8String(std::vector<uint8_t> &&data) : data {data} {}
		Utf8String(const Utf8String &str) : data {str.data} {}
		std::vector<uint8_t> data;

		Utf8String &operator=(Utf8String &&other);
		Utf8String &operator=(const Utf8String &other);

		bool operator==(const Utf8String &other) const
		{
			auto res = (data == other.data);
			UDM_ASSERT_COMPARISON(res);
			return res;
		}
		bool operator!=(const Utf8String &other) const { return !operator==(other); }
	};

	struct DLLUDM Reference {
		Reference() = default;
		Reference(const std::string &path) : path {path} {}
		Reference(const Reference &other) : property {other.property}, path {other.path} {}
		Reference(Reference &&other) : property {other.property}, path {std::move(other.path)} {}
		Property *property = nullptr;
		std::string path;

		Reference &operator=(Reference &&other);
		Reference &operator=(const Reference &other);

		bool operator==(const Reference &other) const
		{
			auto res = (property == other.property);
			UDM_ASSERT_COMPARISON(res);
			return res;
		}
		bool operator!=(const Reference &other) const { return !operator==(other); }
	  private:
		friend Data;
		void InitializeProperty(const LinkedPropertyWrapper &root);
	};

	struct DLLUDM StructDescription {
		using SizeType = uint16_t;
		using MemberCountType = uint8_t;
		std::string GetTemplateArgumentList() const;
		SizeType GetDataSizeRequirement() const;
		MemberCountType GetMemberCount() const;

		// TODO: Use these once C++20 is available
		// bool operator==(const Struct&) const=default;
		// bool operator!=(const Struct&) const=default;
		bool operator==(const StructDescription &other) const;
		bool operator!=(const StructDescription &other) const { return !operator==(other); }

		template<typename T1, typename T2, typename... T>
		void DefineTypes(std::initializer_list<std::string> names)
		{
			Clear();
			constexpr auto n = sizeof...(T) + 2;
			if(names.size() != n)
				throw InvalidUsageError {"Number of member names has to match number of member types!"};
			types.reserve(n);
			this->names.reserve(n);
			DefineTypes<T1, T2, T...>(names.begin());
		}

		template<typename T1, typename T2, typename... T>
		static StructDescription Define(std::initializer_list<std::string> names)
		{
			StructDescription strct {};
			strct.DefineTypes<T1, T2, T...>(names);
			return strct;
		}

		void Clear()
		{
			types.clear();
			names.clear();
		}
		std::vector<Type> types;
		std::vector<String> names;
	  private:
		template<typename T1, typename T2, typename... T>
		void DefineTypes(std::initializer_list<std::string>::iterator it);
		template<typename T>
		void DefineTypes(std::initializer_list<std::string>::iterator it);
	};

	struct DLLUDM Struct {
		static constexpr auto MAX_SIZE = std::numeric_limits<StructDescription::SizeType>::max();
		static constexpr auto MAX_MEMBER_COUNT = std::numeric_limits<StructDescription::MemberCountType>::max();
		Struct() = default;
		Struct(const Struct &) = default;
		Struct(Struct &&) = default;
		Struct(const StructDescription &desc);
		Struct(StructDescription &&desc);
		Struct &operator=(const Struct &) = default;
		Struct &operator=(Struct &&) = default;
		template<class T>
		Struct &operator=(const T &other);
		// TODO: Use these once C++20 is available
		// bool operator==(const Struct&) const=default;
		// bool operator!=(const Struct&) const=default;
		bool operator==(const Struct &other) const;
		bool operator!=(const Struct &other) const { return !operator==(other); }

		StructDescription &operator*() { return description; }
		const StructDescription &operator*() const { return const_cast<Struct *>(this)->operator*(); }
		StructDescription *operator->() { return &description; }
		const StructDescription *operator->() const { return const_cast<Struct *>(this)->operator->(); }

		void Clear();
		void UpdateData();
		void SetDescription(const StructDescription &desc);
		void SetDescription(StructDescription &&desc);
		StructDescription description;
		std::vector<uint8_t> data;
	};
};

export namespace udm {
	template<class T>
	Struct &Struct::operator=(const T &other)
	{
		auto sz = description.GetDataSizeRequirement();
		if(sizeof(T) != sz)
			throw LogicError {"Attempted to assign data of size " + std::to_string(sizeof(T)) + " to struct of size " + std::to_string(sz) + "!"};
		if(data.size() != sz)
			throw ImplementationError {"Size of struct data does not match its types!"};
		memcpy(data.data(), &other, sizeof(T));
		return *this;
	}

	template<typename T1, typename T2, typename... T>
	void StructDescription::DefineTypes(std::initializer_list<std::string>::iterator it)
	{
		DefineTypes<T1>(it);
		DefineTypes<T2, T...>(it + 1);
	}
	template<typename T>
	void StructDescription::DefineTypes(std::initializer_list<std::string>::iterator it)
	{
		types.push_back(type_to_enum<T>());
		names.push_back(*it);
	}
};
