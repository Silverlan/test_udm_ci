/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "udm_definitions.hpp"
#include <array>
#include <cinttypes>
#include <vector>
#include <memory>
#include <string>
#include <cassert>
#include <optional>
#include <variant>
#include <map>
#include <sstream>
#include <mathutil/uvec.h>
#include <mathutil/transform.hpp>
#include <sharedutils/util.h>
#include <sharedutils/magic_enum.hpp>
#include <sharedutils/util_ifile.hpp>
#include <fsys/filesystem.h>
#undef VERSION

#pragma warning(push)
#pragma warning(disable : 4715)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

export module udm;
export import udm.array;
export import udm.element;
export import udm.element_impl;
export import udm.property;
export import udm.property_wrapper;
export import udm.property_wrapper_impl;
export import udm.conversions;
export import udm.enums;
export import udm.exceptions;
export import udm.trivial_types;
export import udm.type_structs;
export import udm.types;
export import udm.basic_types;
export import udm.util;
export import udm.iterators;
export namespace udm {
	constexpr std::string CONTROL_CHARACTERS = "{}[]<>$,:;";
	constexpr std::string WHITESPACE_CHARACTERS = " \t\f\v\n\r";
	constexpr auto PATH_SEPARATOR = '/';
	DLLUDM bool is_whitespace_character(char c);
	DLLUDM bool is_control_character(char c);
	DLLUDM bool does_key_require_quotes(const std::string_view &key);

	struct DLLUDM AsciiException : public Exception {
		AsciiException(const std::string &msg, uint32_t lineIdx, uint32_t charIdx);
		uint32_t lineIndex = 0;
		uint32_t charIndex = 0;
	};

	struct DLLUDM SyntaxError : public AsciiException {
		using AsciiException::AsciiException;
	};
	struct DLLUDM DataError : public AsciiException {
		using AsciiException::AsciiException;
	};

	constexpr const char *enum_type_to_ascii(Type t)
	{
		// Note: These have to match ascii_type_to_enum
		switch(t) {
		case Type::Nil:
			return "nil";
		case Type::String:
			return "string";
		case Type::Utf8String:
			return "utf8";
		case Type::Int8:
			return "int8";
		case Type::UInt8:
			return "uint8";
		case Type::Int16:
			return "int16";
		case Type::UInt16:
			return "uint16";
		case Type::Int32:
			return "int32";
		case Type::UInt32:
			return "uint32";
		case Type::Int64:
			return "int64";
		case Type::UInt64:
			return "uint64";
		case Type::Float:
			return "float";
		case Type::Double:
			return "double";
		case Type::Boolean:
			return "bool";
		case Type::Vector2:
			return "vec2";
		case Type::Vector2i:
			return "vec2i";
		case Type::Vector3:
			return "vec3";
		case Type::Vector3i:
			return "vec3i";
		case Type::Vector4:
			return "vec4";
		case Type::Vector4i:
			return "vec4i";
		case Type::Quaternion:
			return "quat";
		case Type::EulerAngles:
			return "ang";
		case Type::Srgba:
			return "srgba";
		case Type::HdrColor:
			return "hdr";
		case Type::Transform:
			return "transform";
		case Type::ScaledTransform:
			return "stransform";
		case Type::Mat4:
			return "mat4";
		case Type::Mat3x4:
			return "mat3x4";
		case Type::Blob:
			return "blob";
		case Type::BlobLz4:
			return "lz4";
		case Type::Array:
			return "array";
		case Type::ArrayLz4:
			return "arrayLz4";
		case Type::Element:
			return "element";
		case Type::Reference:
			return "ref";
		case Type::Half:
			return "half";
		case Type::Struct:
			return "struct";
		}
		static_assert(umath::to_integral(Type::Count) == 36, "Update this list when new types are added!");
	}
	DLLUDM Type ascii_type_to_enum(const std::string_view &type);
	DLLUDM void sanitize_key_name(std::string &key);

	DLLUDM Blob decompress_lz4_blob(const BlobLz4 &data);
	DLLUDM Blob decompress_lz4_blob(const void *compressedData, uint64_t compressedSize, uint64_t uncompressedSize);
	DLLUDM void decompress_lz4_blob(const void *compressedData, uint64_t compressedSize, uint64_t uncompressedSize, void *outData);
	DLLUDM BlobLz4 compress_lz4_blob(const Blob &data);
	DLLUDM BlobLz4 compress_lz4_blob(const void *data, uint64_t size);
	template<class T>
	BlobLz4 compress_lz4_blob(const T &v)
	{
		return compress_lz4_blob(v.data(), v.size() * sizeof(v[0]));
	}

	using Version = uint32_t;
	/* Version history:
	* 1: Initial version
	* 2: Added types: reference, arrayLz4, struct, half, vector2i, vector3i, vector4i
	*/
	constexpr Version VERSION = 2;
	constexpr auto *HEADER_IDENTIFIER = "UDMB";
#pragma pack(push, 1)
	struct DLLUDM Header {
		Header() = default;
		std::array<char, 4> identifier = {HEADER_IDENTIFIER[0], HEADER_IDENTIFIER[1], HEADER_IDENTIFIER[2], HEADER_IDENTIFIER[3]};
		Version version = VERSION;
	};
#pragma pack(pop)

	struct DLLUDM AssetData : public LinkedPropertyWrapper {
		std::string GetAssetType() const;
		Version GetAssetVersion() const;
		void SetAssetType(const std::string &assetType) const;
		void SetAssetVersion(Version version) const;

		LinkedPropertyWrapper GetData() const;
		LinkedPropertyWrapper operator*() const { return GetData(); }
		LinkedPropertyWrapper operator->() const { return GetData(); }
	};

	class DLLUDM Data {
	  public:
		static constexpr auto KEY_ASSET_TYPE = "assetType";
		static constexpr auto KEY_ASSET_VERSION = "assetVersion";
		static constexpr auto KEY_ASSET_DATA = "assetData";
		static std::optional<FormatType> GetFormatType(const std::string &fileName, std::string &outErr);
		static std::optional<FormatType> GetFormatType(std::unique_ptr<IFile> &&f, std::string &outErr);
		static std::optional<FormatType> GetFormatType(const ::VFilePtr &f, std::string &outErr);
		static std::shared_ptr<Data> Load(const std::string &fileName);
		static std::shared_ptr<Data> Load(std::unique_ptr<IFile> &&f);
		static std::shared_ptr<Data> Load(const ::VFilePtr &f);
		static std::shared_ptr<Data> Open(const std::string &fileName);
		static std::shared_ptr<Data> Open(std::unique_ptr<IFile> &&f);
		static std::shared_ptr<Data> Open(const ::VFilePtr &f);
		static std::shared_ptr<Data> Create(const std::string &assetType, Version assetVersion);
		static std::shared_ptr<Data> Create();
		static bool DebugTest();

		PProperty LoadProperty(const std::string_view &path) const;
		void ResolveReferences();

		bool Save(const std::string &fileName) const;
		bool Save(IFile &f) const;
		bool Save(const ::VFilePtr &f);
		bool SaveAscii(const std::string &fileName, AsciiSaveFlags flags = AsciiSaveFlags::Default) const;
		bool SaveAscii(IFile &f, AsciiSaveFlags flags = AsciiSaveFlags::Default) const;
		bool SaveAscii(const ::VFilePtr &f, AsciiSaveFlags flags = AsciiSaveFlags::Default) const;
		Element &GetRootElement() { return *static_cast<Element *>(m_rootProperty->value); }
		const Element &GetRootElement() const { return const_cast<Data *>(this)->GetRootElement(); }
		AssetData GetAssetData() const;

		bool operator==(const Data &other) const;
		bool operator!=(const Data &other) const { return !operator==(other); }

		LinkedPropertyWrapper operator[](const std::string &key) const;
		Element *operator->();
		const Element *operator->() const;
		Element &operator*();
		const Element &operator*() const;

		std::string GetAssetType() const;
		Version GetAssetVersion() const;
		void SetAssetType(const std::string &assetType);
		void SetAssetVersion(Version version);

		void ToAscii(std::stringstream &ss, AsciiSaveFlags flags = AsciiSaveFlags::Default) const;

		const Header &GetHeader() const { return m_header; }

		static std::string ReadKey(IFile &f);
		static void WriteKey(IFile &f, const std::string &key);
	  private:
		friend AsciiReader;
		friend ArrayLz4;
		bool ValidateHeaderProperties();
		static void SkipProperty(IFile &f, Type type);
		PProperty LoadProperty(Type type, const std::string_view &path) const;
		static PProperty ReadProperty(IFile &f);
		static void WriteProperty(IFile &f, const Property &o);
		Data() = default;
		Header m_header;
		std::unique_ptr<IFile> m_file = nullptr;
		PProperty m_rootProperty = nullptr;
	};

	template<typename TEnum>
	constexpr std::string_view enum_to_string(TEnum e)
	{
		return magic_enum::enum_name(e);
	}

	template<typename TEnum>
	constexpr std::string flags_to_string(TEnum e)
	{
		return magic_enum::flags::enum_name(e);
	}

	template<typename TEnum>
	TEnum string_to_enum(udm::LinkedPropertyWrapperArg udmEnum, TEnum def)
	{
		std::string str;
		udmEnum(str);
		auto e = magic_enum::enum_cast<TEnum>(str);
		return e.has_value() ? *e : def;
	}

	template<typename TEnum>
	TEnum string_to_flags(udm::LinkedPropertyWrapperArg udmEnum, TEnum def)
	{
		std::string str;
		udmEnum(str);
		auto e = magic_enum::flags::enum_cast<TEnum>(str);
		return e.has_value() ? *e : def;
	}

	template<typename TEnum>
	void to_enum_value(udm::LinkedPropertyWrapperArg udmEnum, TEnum &def)
	{
		std::string str;
		udmEnum(str);
		auto e = magic_enum::enum_cast<TEnum>(str);
		def = e.has_value() ? *e : def;
	}

	template<typename TEnum>
	void to_flags(udm::LinkedPropertyWrapperArg udmEnum, TEnum &def)
	{
		std::string str;
		udmEnum(str);
		auto e = magic_enum::flags::enum_cast<TEnum>(str);
		def = e.has_value() ? *e : def;
	}

	template<typename TEnum>
	void write_flag(udm::LinkedPropertyWrapperArg udm, TEnum flags, TEnum flag, const std::string_view &name)
	{
		if(umath::is_flag_set(flags, flag) == false)
			return;
		udm[name] = true;
	}
	template<typename TEnum>
	void read_flag(LinkedPropertyWrapperArg udm, TEnum &flags, TEnum flag, const std::string_view &name)
	{
		if(!udm)
			return;
		umath::set_flag(flags, flag, udm[name](false));
	}

	template<typename T>
	void lerp_value(const T &value0, const T &value1, float f, T &outValue, ::udm::Type type)
	{
		using TBase = ::udm::base_type<T>;
		if constexpr(std::is_same_v<TBase, ::udm::Transform> || std::is_same_v<TBase, ::udm::ScaledTransform>) {
			outValue.SetOrigin(uvec::lerp(value0.GetOrigin(), value1.GetOrigin(), f));
			outValue.SetRotation(uquat::slerp(value0.GetRotation(), value1.GetRotation(), f));
			if constexpr(std::is_same_v<TBase, ::udm::ScaledTransform>)
				outValue.SetScale(uvec::lerp(value0.GetScale(), value1.GetScale(), f));
		}
		else if constexpr(std::is_same_v<TBase, ::udm::Half>)
			outValue = static_cast<float>(umath::lerp(static_cast<float>(value0), static_cast<float>(value1), f));
		else if constexpr(::udm::is_arithmetic<TBase>)
			outValue = umath::lerp(value0, value1, f);
		else if constexpr(::udm::is_vector_type<TBase>) {
			if constexpr(std::is_integral_v<typename TBase::value_type>)
				; // TODO
			else
				outValue = value0 + (value1 - value0) * f;
		}
		else if constexpr(std::is_same_v<TBase, ::udm::EulerAngles>) {
			auto q0 = uquat::create(value0);
			auto q1 = uquat::create(value1);
			auto qr = uquat::slerp(q0, q1, f);
			outValue = EulerAngles {qr};
		}
		else if constexpr(std::is_same_v<TBase, ::udm::Quaternion>)
			outValue = uquat::slerp(value0, value1, f);
		else {
			outValue = value0;
			auto n = ::udm::get_numeric_component_count(type);
			for(auto i = decltype(n) {0u}; i < n; ++i) {
				auto &f0 = *(reinterpret_cast<const float *>(&value0) + i);
				auto &f1 = *(reinterpret_cast<const float *>(&value1) + i);

				*(reinterpret_cast<float *>(&outValue) + i) = umath::lerp(f0, f1, f);
			}
		}
	}

	void to_json(LinkedPropertyWrapperArg prop, std::stringstream &ss);

	namespace detail {
		DLLUDM void test_c_wrapper();
	};
};
export { REGISTER_BASIC_BITWISE_OPERATORS(udm::AsciiSaveFlags) REGISTER_BASIC_BITWISE_OPERATORS(udm::MergeFlags) }
#pragma GCC diagnostic pop
#pragma warning(pop)
