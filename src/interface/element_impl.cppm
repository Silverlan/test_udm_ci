/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "udm_definitions.hpp"
#include <sharedutils/util.h>
#include <sharedutils/util_string_hash.hpp>

export module udm.element_impl;
import udm.types;
import udm.property_wrapper;
export namespace udm {
	udm::PProperty *element_find_child(Element &el, Element &child);
	template<typename T>
	void element_set_value(Element &el, Element &child, T &&v)
	{
		auto *pChild = element_find_child(el, child);
		if(!pChild)
			return;
		*pChild = Property::Create<T>(std::forward<T>(v));
	}
}
