/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "udm_definitions.hpp"
#include <sharedutils/util.h>
#include <sharedutils/util_string_hash.hpp>

export module udm.iterators;

import udm.property_wrapper;

export namespace udm {
	class ElementIterator;
	struct DLLUDM ElementIteratorWrapper {
		ElementIteratorWrapper(LinkedPropertyWrapper &prop);
		ElementIterator begin();
		ElementIterator end();
	  private:
		LinkedPropertyWrapper m_prop;
	};
	using ElIt = ElementIteratorWrapper;

	struct DLLUDM ElementIteratorPair {
		ElementIteratorPair(util::StringMap<PProperty>::iterator &it);
		ElementIteratorPair();
		bool operator==(const ElementIteratorPair &other) const;
		bool operator!=(const ElementIteratorPair &other) const;
		std::string_view key;
		LinkedPropertyWrapper property;
	};

	class DLLUDM ElementIterator {
	  public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = ElementIteratorPair &;
		using difference_type = std::ptrdiff_t;
		using pointer = ElementIteratorPair *;
		using reference = ElementIteratorPair &;

		ElementIterator();
		ElementIterator(udm::Element &e);
		ElementIterator(udm::Element &e, util::StringMap<PProperty> &c, util::StringMap<PProperty>::iterator it);
		ElementIterator(const ElementIterator &other);
		ElementIterator &operator++();
		ElementIterator operator++(int);
		reference operator*();
		pointer operator->();
		bool operator==(const ElementIterator &other) const;
		bool operator!=(const ElementIterator &other) const;
	  private:
		util::StringMap<PProperty> *m_propertyMap = nullptr;
		util::StringMap<PProperty>::iterator m_iterator {};
		ElementIteratorPair m_pair;
	};

	template<typename T>
	class ArrayIterator {
	  public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T &;
		using difference_type = std::ptrdiff_t;
		using pointer = T *;
		using reference = T &;

		ArrayIterator();
		ArrayIterator(udm::Array &a);
		ArrayIterator(udm::Array &a, uint32_t idx);
		ArrayIterator(const ArrayIterator &other);
		ArrayIterator &operator++();
		ArrayIterator operator++(int);
		ArrayIterator operator+(uint32_t n);
		reference operator*();
		pointer operator->();
		bool operator==(const ArrayIterator &other) const;
		bool operator!=(const ArrayIterator &other) const;

		udm::LinkedPropertyWrapper &GetProperty() { return m_curProperty; }
	  private:
		udm::LinkedPropertyWrapper m_curProperty;
	};

	template<typename T>
	ArrayIterator<T>::ArrayIterator() : m_curProperty {}
	{
	}

	template<typename T>
	ArrayIterator<T>::ArrayIterator(Array &a, uint32_t idx) : m_curProperty {a, idx}
	{
	}

	template<typename T>
	ArrayIterator<T>::ArrayIterator(Array &a) : ArrayIterator {a, 0u}
	{
	}

	template<typename T>
	ArrayIterator<T>::ArrayIterator(const ArrayIterator &other) : m_curProperty {other.m_curProperty}
	{
	}

	template<typename T>
	ArrayIterator<T> &ArrayIterator<T>::operator++()
	{
		++m_curProperty.arrayIndex;
		return *this;
	}

	template<typename T>
	ArrayIterator<T> ArrayIterator<T>::operator++(int)
	{
		auto it = *this;
		it.operator++();
		return it;
	}

	template<typename T>
	ArrayIterator<T> ArrayIterator<T>::operator+(uint32_t n)
	{
		auto it = *this;
		for(auto i = decltype(n) {0u}; i < n; ++i)
			it.operator++();
		return it;
	}

	template<typename T>
	typename ArrayIterator<T>::reference ArrayIterator<T>::operator*()
	{
		if constexpr(std::is_same_v<T, LinkedPropertyWrapper>)
			return m_curProperty;
		else
			return m_curProperty.GetValue<T>();
	}

	template<typename T>
	typename ArrayIterator<T>::pointer ArrayIterator<T>::operator->()
	{
		if constexpr(std::is_same_v<T, LinkedPropertyWrapper>)
			return &m_curProperty;
		else
			return m_curProperty.GetValuePtr<T>();
	}

	template<typename T>
	bool ArrayIterator<T>::operator==(const ArrayIterator &other) const
	{
		auto res = (m_curProperty == other.m_curProperty);
		// UDM_ASSERT_COMPARISON(res);
		return res;
	}

	template<typename T>
	bool ArrayIterator<T>::operator!=(const ArrayIterator &other) const
	{
		return !operator==(other);
	}
}
