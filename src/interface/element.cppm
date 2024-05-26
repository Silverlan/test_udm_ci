/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "udm_definitions.hpp"
#include <sharedutils/util.h>
#include <sharedutils/util_string_hash.hpp>

export module udm.element;
import udm.types;
import udm.property_wrapper;
export namespace udm {
	struct DLLUDM Element {
		void AddChild(std::string &&key, const PProperty &o);
		void AddChild(const std::string &key, const PProperty &o);
		void Copy(const Element &other);
		util::StringMap<PProperty> children;
		PropertyWrapper fromProperty {};
		PropertyWrapper parentProperty {};

		LinkedPropertyWrapper operator[](const std::string &key) { return fromProperty[key]; }
		LinkedPropertyWrapper operator[](const char *key) { return operator[](std::string {key}); }

		LinkedPropertyWrapper Add(const std::string_view &path, Type type = Type::Element, bool pathToElements = false);
		LinkedPropertyWrapper AddArray(const std::string_view &path, std::optional<uint32_t> size = {}, Type type = Type::Element, ArrayType arrayType = ArrayType::Raw, bool pathToElements = false);
		void ToAscii(AsciiSaveFlags flags, std::stringstream &ss, const std::optional<std::string> &prefix = {}) const;

		void Merge(const Element &other, MergeFlags mergeFlags = MergeFlags::OverwriteExisting);

		bool operator==(const Element &other) const;
		bool operator!=(const Element &other) const { return !operator==(other); }
		Element &operator=(Element &&other);
		Element &operator=(const Element &other);

		explicit operator PropertyWrapper &() { return fromProperty; }

		ElementIterator begin();
		ElementIterator end();

		template<typename T>
		void ImplSetValue(Element &child, T &&v)
		{
			SetValue<T>(child, v);
		}
	  private:
		friend PropertyWrapper;
		template<typename T>
		void SetValue(Element &child, T &&v);
		template<typename T>
		void EraseValue(const Element &child);
	};

	void element_erase_value(Element &el, const Element &child);
	template<typename T>
	void Element::EraseValue(const Element &child)
	{
		element_erase_value(*this, child);
	}

	template<typename T>
	void element_set_value(Element &el, Element &child, T &&v);
	template<typename T>
	void Element::SetValue(Element &child, T &&v)
	{
		element_set_value<T>(*this, child, std::forward<T>(v));
	}
}