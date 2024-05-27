/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <type_traits>
#include <sharedutils/util.h>
#include <sharedutils/magic_enum.hpp>

export module udm.property_wrapper_impl;

import udm.property_wrapper;
import udm.element;
import udm.trivial_types;
import udm.property;
import udm.array;

export namespace udm {
	template<typename T>
	void property_wrapper_operator_equal(const udm::PropertyWrapper &wrapper, T &&v)
	{
		if(wrapper.prop == nullptr)
			throw LogicError {"Cannot assign property value: Property is invalid!"};
		using TBase = std::remove_cv_t<std::remove_reference_t<T>>;
		if constexpr(util::is_specialization<TBase, std::optional>::value) {
			// Value is std::optional
			if(!v) {
				// nullopt case
				// TODO: This code is somewhat redundant (see other cases below) and should be streamlined
				if(is_array_type(wrapper.prop->type)) {
					if(wrapper.arrayIndex == std::numeric_limits<uint32_t>::max())
						throw LogicError {"Cannot assign propety value to array: No index has been specified!"};
					if(wrapper.ImplIsLinked() && !static_cast<const LinkedPropertyWrapper *>(&wrapper)->propName.empty()) {
						auto &a = *static_cast<Array *>(wrapper.prop->value);
						if(a.GetValueType() != Type::Element)
							return;
						auto &e = a.GetValue<Element>(wrapper.arrayIndex);
						auto it = e.children.find(static_cast<const LinkedPropertyWrapper *>(&wrapper)->propName);
						if(it != e.children.end())
							e.children.erase(it);
					}
					else
						throw LogicError {"Cannot assign nullopt value to array!"};
					return;
				}
				if constexpr(std::is_same_v<TBase, PProperty>) {
					if(wrapper.ImplIsLinked() && !static_cast<const LinkedPropertyWrapper *>(&wrapper)->propName.empty()) {
						auto &linked = *wrapper.GetLinked();
						if(linked.prev && linked.prev->IsType(Type::Element)) {
							auto &el = linked.prev->GetValue<Element>();
							auto it = el.children.find(linked.propName);
							if(it != el.children.end())
								el.children.erase(it);
							return;
						}
					}
				}
				if(wrapper.prop->type != Type::Element) {
					throw LogicError {"Cannot assign nullopt value to concrete UDM value!"};
					return;
				}
				if constexpr(type_to_enum_s<TBase>() != Type::Invalid) {
					if(wrapper.prop->value == nullptr)
						throw LogicError {"Cannot assign property value: Property is invalid!"};
					auto &el = *static_cast<Element *>(wrapper.prop->value);
					auto &wpParent = el.parentProperty;
					if(!wpParent)
						throw InvalidUsageError {"Attempted to change value of element property without a valid parent, this is not allowed!"};
					auto &parent = wpParent;
					switch(parent->type) {
					case Type::Element:
						static_cast<Element *>(parent->value)->EraseValue<const udm::Element &>(el);
						break;
					/*case Type::Array:
				if(arrayIndex == std::numeric_limits<uint32_t>::max())
					throw std::runtime_error{"Element has parent of type " +std::string{magic_enum::enum_name(parent->type)} +", but is not indexed!"};
				(*static_cast<Array*>(parent->value))[arrayIndex] = v;
				break;*/
					default:
						throw InvalidUsageError {"Element has parent of type " + std::string {magic_enum::enum_name(parent->type)} + ", but only " + std::string {magic_enum::enum_name(Type::Element)} /* +" and " +std::string{magic_enum::enum_name(Type::Array)}*/ + " types are allowed!"};
					}
				}
				else
					throw LogicError {"Cannot assign custom type to non-struct property!"};
				return;
			}
			return wrapper.operator=(*v);
		}
		else if constexpr(std::is_enum_v<std::remove_reference_t<TBase>>)
			return wrapper.operator=(magic_enum::enum_name(v));
		else {
			if(is_array_type(wrapper.prop->type)) {
				if(wrapper.arrayIndex == std::numeric_limits<uint32_t>::max())
					throw LogicError {"Cannot assign propety value to array: No index has been specified!"};
				if(wrapper.ImplIsLinked() && !static_cast<const LinkedPropertyWrapper *>(&wrapper)->propName.empty()) {
					auto &a = *static_cast<Array *>(wrapper.prop->value);
					if(a.GetValueType() != Type::Element)
						return;
					auto &e = a.GetValue<Element>(wrapper.arrayIndex);
					if constexpr(type_to_enum_s<TBase>() != Type::Invalid)
						e.children[static_cast<const LinkedPropertyWrapper *>(&wrapper)->propName] = Property::Create(v);
					else if constexpr(std::is_same_v<TBase, PProperty>)
						e.children[static_cast<const LinkedPropertyWrapper *>(&wrapper)->propName] = v;
					else if constexpr(std::is_same_v<TBase, Property>)
						e.children[static_cast<const LinkedPropertyWrapper *>(&wrapper)->propName] = std::make_shared<Property>(v);
					else {
						auto it = e.children.find(static_cast<const LinkedPropertyWrapper *>(&wrapper)->propName);
						if(it == e.children.end() || it->second->IsType(Type::Struct) == false)
							throw LogicError {"Cannot assign custom type to non-struct property!"};
						it->second->GetValue<Struct>() = std::move(v);
					}
				}
				else
					static_cast<Array *>(wrapper.prop->value)->SetValue(wrapper.arrayIndex, v);
				return;
			}
			if constexpr(std::is_same_v<TBase, PProperty>) {
				if(wrapper.ImplIsLinked() && !static_cast<const LinkedPropertyWrapper *>(&wrapper)->propName.empty()) {
					auto &linked = *wrapper.GetLinked();
					if(linked.prev && linked.prev->IsType(Type::Element)) {
						auto &el = linked.prev->GetValue<Element>();
						el.children[linked.propName] = v;
						return;
					}
				}
			}
			if(wrapper.prop->type != Type::Element) {
				*wrapper.prop = v;
				return;
			}
			if constexpr(type_to_enum_s<TBase>() != Type::Invalid) {
				if(wrapper.prop->value == nullptr)
					throw LogicError {"Cannot assign property value: Property is invalid!"};
				auto &el = *static_cast<Element *>(wrapper.prop->value);
				auto &wpParent = el.parentProperty;
				if(!wpParent)
					throw InvalidUsageError {"Attempted to change value of element property without a valid parent, this is not allowed!"};
				auto &parent = wpParent;
				switch(parent->type) {
				case Type::Element:
					static_cast<Element *>(parent->value)->ImplSetValue(el, v);
					break;
				/*case Type::Array:
				if(wrapper.arrayIndex == std::numeric_limits<uint32_t>::max())
					throw std::runtime_error{"Element has parent of type " +std::string{magic_enum::enum_name(parent->type)} +", but is not indexed!"};
				(*static_cast<Array*>(parent->value))[wrapper.arrayIndex] = v;
				break;*/
				default:
					throw InvalidUsageError {"Element has parent of type " + std::string {magic_enum::enum_name(parent->type)} + ", but only " + std::string {magic_enum::enum_name(Type::Element)} /* +" and " +std::string{magic_enum::enum_name(Type::Array)}*/ + " types are allowed!"};
				}
			}
			else
				throw LogicError {"Cannot assign custom type to non-struct property!"};
		}
	}

	template<typename T>
	LinkedPropertyWrapper property_wrapper_add_array(const PropertyWrapper &wrapper, const std::string_view &path, const StructDescription &strct, const T *data, uint32_t strctItems, ArrayType arrayType, bool pathToElements)
	{
		auto prop = wrapper.AddArray(path, strct, strctItems, arrayType, pathToElements);
		auto &a = prop.template GetValue<Array>();
		auto sz = a.GetValueSize() * a.GetSize();
		auto *ptr = a.GetValues();
		memcpy(ptr, data, sz);
		return prop;
	}

	template<class T>
	BlobResult property_wrapper_get_blob_data(const PropertyWrapper &wrapper, T &v)
	{
		uint64_t reqBufferSize = 0;
		auto result = wrapper.GetBlobData(v.data(), v.size() * sizeof(v[0]), &reqBufferSize);
		if(result == BlobResult::InsufficientSize) {
			if(v.size() * sizeof(v[0]) != reqBufferSize) {
				if((reqBufferSize % sizeof(v[0])) > 0)
					return BlobResult::ValueTypeMismatch;
				v.resize(reqBufferSize / sizeof(v[0]));
				return wrapper.GetBlobData<T>(v);
			}
			return result;
		}
		if(result != BlobResult::NotABlobType)
			return result;
		if(wrapper.ImplIsArrayItem(true)) {
			if(wrapper.ImplIsLinked() && !static_cast<const LinkedPropertyWrapper &>(wrapper).propName.empty()) {
				auto &a = wrapper.prop->GetValue<Array>();
				auto *el = a.GetValuePtr<Element>(wrapper.arrayIndex);
				if(!el)
					return BlobResult::InvalidProperty;
				auto it = el->children.find(static_cast<const LinkedPropertyWrapper &>(wrapper).propName);
				if(it != el->children.end())
					return it->second->GetBlobData<T>(v);
				return BlobResult::InvalidProperty;
			}
			return BlobResult::NotABlobType;
		}
		return (*wrapper)->GetBlobData(v);
	}

	template<typename T>
	T &property_wrapper_get_value(const PropertyWrapper &wrapper)
	{
		if(wrapper.arrayIndex != std::numeric_limits<uint32_t>::max()) {
			auto *a = wrapper.prop->GetValuePtr<Array>();
			if(a) {
				if(wrapper.ImplIsLinked() && !static_cast<const LinkedPropertyWrapper &>(wrapper).propName.empty()) {
					auto &children = const_cast<Element &>(a->GetValue<Element>(wrapper.arrayIndex)).children;
					auto &propName = static_cast<const LinkedPropertyWrapper &>(wrapper).propName;
					auto it = children.find(propName);
					if(it == children.end())
						throw LogicError {"Attempted to retrieve value of property '" + propName + "' from array element at index " + std::to_string(wrapper.arrayIndex) + ", but property does not exist!"};
					return it->second->GetValue<T>();
				}
				if(a->IsValueType(type_to_enum<T>()) == false)
					throw LogicError {"Type mismatch, requested type is " + std::string {magic_enum::enum_name(type_to_enum<T>())} + ", but actual type is " + std::string {magic_enum::enum_name(a->GetValueType())} + "!"};
				return static_cast<T *>(a->GetValues())[wrapper.arrayIndex];
			}
		}
		return (*wrapper)->GetValue<T>();
	}

	template<typename T>
	T *property_wrapper_get_value_ptr(const PropertyWrapper &wrapper)
	{
		if(wrapper.arrayIndex != std::numeric_limits<uint32_t>::max()) {
			auto *a = wrapper.prop->GetValuePtr<Array>();
			if(a) {
				if(wrapper.ImplIsLinked() && !static_cast<const LinkedPropertyWrapper &>(wrapper).propName.empty()) {
					auto &children = const_cast<Element &>(a->GetValue<Element>(wrapper.arrayIndex)).children;
					auto it = children.find(static_cast<const LinkedPropertyWrapper &>(wrapper).propName);
					if(it == children.end())
						return nullptr;
					return it->second->GetValuePtr<T>();
				}
				if(a->IsValueType(type_to_enum<T>()) == false)
					return nullptr;
				return &static_cast<T *>(a->GetValues())[wrapper.arrayIndex];
			}
		}
		return wrapper.prop ? (*wrapper)->GetValuePtr<T>() : nullptr;
	}

	template<typename T>
	ArrayIterator<T> property_wrapper_begin(const PropertyWrapper &wrapper)
	{
		if(!static_cast<bool>(*wrapper))
			return ArrayIterator<T> {};
		auto *a = wrapper.GetValuePtr<Array>();
		if(a == nullptr)
			return ArrayIterator<T> {};
		auto it = a->begin<T>();
		if(wrapper.ImplIsLinked())
			it.GetProperty().prev = std::make_unique<LinkedPropertyWrapper>(*static_cast<LinkedPropertyWrapper *>(const_cast<PropertyWrapper *>(&wrapper)));
		return it;
	}

	template<typename T>
	ArrayIterator<T> property_wrapper_end(const PropertyWrapper &wrapper)
	{
		if(!static_cast<bool>(*wrapper))
			return ArrayIterator<T> {};
		auto *a = wrapper.GetValuePtr<Array>();
		if(a == nullptr)
			return ArrayIterator<T> {};
		return a->end<T>();
	}

	template<typename T>
	std::optional<T> property_wrapper_to_value(const PropertyWrapper *wrapper)
	{
		if(!wrapper) // This can happen in chained expressions. TODO: This is technically undefined behavior and should be implemented differently!
			return {};
		if(wrapper->ImplIsArrayItem(true)) {
			auto &a = wrapper->prop->GetValue<Array>();
			if(wrapper->ImplIsLinked() && !static_cast<const LinkedPropertyWrapper &>(*wrapper).propName.empty()) {
				auto &children = const_cast<Element &>(a.GetValue<Element>(wrapper->arrayIndex)).children;
				auto it = children.find(static_cast<const LinkedPropertyWrapper &>(*wrapper).propName);
				if(it == children.end())
					return {};
				return it->second->ToValue<T>();
			}
			auto vs = [&](auto tag) -> std::optional<T> {
				if constexpr(is_convertible<typename decltype(tag)::type, T>())
					return std::optional<T> {convert<typename decltype(tag)::type, T>(const_cast<PropertyWrapper *>(wrapper)->GetValue<typename decltype(tag)::type>())};
				return {};
			};
			auto valueType = a.GetValueType();
			return visit(valueType, vs);
		}
		if(wrapper->prop)
			return (*wrapper)->ToValue<T>();
		return std::optional<T> {};
	}
}
