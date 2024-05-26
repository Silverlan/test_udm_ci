/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <mathutil/umath.h>
#include <sharedutils/util.h>
#include <sharedutils/magic_enum.hpp>
#include <variant>
#include <map>

export module udm.util;

import udm.enums;
import udm.trivial_types;
import udm.basic_types;
import udm.types;
import udm.exceptions;
import udm.element;
import udm.array;

export namespace udm {
	template<typename T>
	constexpr Type type_to_enum()
	{
		constexpr auto type = type_to_enum_s<T>();
		if constexpr(umath::to_integral(type) > umath::to_integral(Type::Last))
			[]<bool flag = false>() { static_assert(flag, "Unsupported type!"); }
		();
		return type;
	}

	template<typename T>
	constexpr Type type_to_enum_s()
	{
		if constexpr(std::is_enum_v<T>)
			return type_to_enum_s<std::underlying_type_t<T>>();

		if constexpr(util::is_specialization<T, std::vector>::value)
			return Type::Array;
		else if constexpr(util::is_specialization<T, std::unordered_map>::value || util::is_specialization<T, std::map>::value)
			return Type::Element;
		else if constexpr(std::is_same_v<T, Nil> || std::is_same_v<T, void>)
			return Type::Nil;
		else if constexpr(util::is_string<T>::value || std::is_same_v<T, std::string_view>)
			return Type::String;
		else if constexpr(std::is_same_v<T, Utf8String>)
			return Type::Utf8String;
		else if constexpr(std::is_same_v<T, Int8>)
			return Type::Int8;
		else if constexpr(std::is_same_v<T, UInt8>)
			return Type::UInt8;
		else if constexpr(std::is_same_v<T, Int16>)
			return Type::Int16;
		else if constexpr(std::is_same_v<T, UInt16>)
			return Type::UInt16;
		else if constexpr(std::is_same_v<T, Int32>)
			return Type::Int32;
		else if constexpr(std::is_same_v<T, UInt32>)
			return Type::UInt32;
		else if constexpr(std::is_same_v<T, Int64>)
			return Type::Int64;
		else if constexpr(std::is_same_v<T, UInt64>)
			return Type::UInt64;
		else if constexpr(std::is_same_v<T, Float>)
			return Type::Float;
		else if constexpr(std::is_same_v<T, Double>)
			return Type::Double;
		else if constexpr(std::is_same_v<T, Vector2>)
			return Type::Vector2;
		else if constexpr(std::is_same_v<T, Vector2i>)
			return Type::Vector2i;
		else if constexpr(std::is_same_v<T, Vector3>)
			return Type::Vector3;
		else if constexpr(std::is_same_v<T, Vector3i>)
			return Type::Vector3i;
		else if constexpr(std::is_same_v<T, Vector4>)
			return Type::Vector4;
		else if constexpr(std::is_same_v<T, Vector4i>)
			return Type::Vector4i;
		else if constexpr(std::is_same_v<T, Quaternion>)
			return Type::Quaternion;
		else if constexpr(std::is_same_v<T, EulerAngles>)
			return Type::EulerAngles;
		else if constexpr(std::is_same_v<T, Srgba>)
			return Type::Srgba;
		else if constexpr(std::is_same_v<T, HdrColor>)
			return Type::HdrColor;
		else if constexpr(std::is_same_v<T, Boolean>)
			return Type::Boolean;
		else if constexpr(std::is_same_v<T, Transform>)
			return Type::Transform;
		else if constexpr(std::is_same_v<T, ScaledTransform>)
			return Type::ScaledTransform;
		else if constexpr(std::is_same_v<T, Mat4>)
			return Type::Mat4;
		else if constexpr(std::is_same_v<T, Mat3x4>)
			return Type::Mat3x4;
		else if constexpr(std::is_same_v<T, Blob>)
			return Type::Blob;
		else if constexpr(std::is_same_v<T, BlobLz4>)
			return Type::BlobLz4;
		else if constexpr(std::is_same_v<T, Element>)
			return Type::Element;
		else if constexpr(std::is_same_v<T, Array>)
			return Type::Array;
		else if constexpr(std::is_same_v<T, ArrayLz4>)
			return Type::ArrayLz4;
		else if constexpr(std::is_same_v<T, Reference>)
			return Type::Reference;
		else if constexpr(std::is_same_v<T, Half>)
			return Type::Half;
		else if constexpr(std::is_same_v<T, Struct>)
			return Type::Struct;
		static_assert(umath::to_integral(Type::Count) == 36, "Update this list when new types are added!");
		return Type::Invalid;
	}

	constexpr size_t size_of(Type t)
	{
		if(is_numeric_type(t)) {
			auto tag = get_numeric_tag(t);
			return std::visit([&](auto tag) { return sizeof(typename decltype(tag)::type); }, tag);
		}

		if(is_generic_type(t)) {
			auto tag = get_generic_tag(t);
			return std::visit(
			  [&](auto tag) {
				  if constexpr(std::is_same_v<typename decltype(tag)::type, std::monostate>)
					  return static_cast<uint64_t>(0);
				  return sizeof(typename decltype(tag)::type);
			  },
			  tag);
		}
		throw InvalidUsageError {std::string {"UDM type "} + std::string {magic_enum::enum_name(t)} + " has non-constant size!"};
		static_assert(umath::to_integral(Type::Count) == 36, "Update this list when new types are added!");
		return 0;
	}

	constexpr size_t size_of_base_type(Type t)
	{
		if(is_non_trivial_type(t)) {
			auto tag = get_non_trivial_tag(t);
			return std::visit([&](auto tag) { return sizeof(typename decltype(tag)::type); }, tag);
		}
		return size_of(t);
	}

	template<typename T>
	constexpr Type array_value_type_to_enum()
	{
		static_assert(util::is_specialization<T, std::vector>::value);
		return type_to_enum<T::value_type>();
	}
};
