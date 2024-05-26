/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "udm_definitions.hpp"
#include <sharedutils/base64.hpp>
#include <cassert>

module udm;

static void to_json(udm::LinkedPropertyWrapperArg prop, std::stringstream &ss, const std::string &t)
{
	auto type = prop.GetType();
	auto tsub = t + '\t';
	if(udm::is_array_type(type)) {
		ss << "[";
		auto first = true;
		auto addNewLine = false;
		for(auto &item : prop) {
			if(first)
				first = false;
			else
				ss << ",";
			if(item.IsType(udm::Type::Element)) {
				addNewLine = true;
				ss << "\n" << tsub;
			}
			to_json(item, ss, tsub);
		}
		if(addNewLine)
			ss << "\n" << t;
		ss << "]";
		return;
	}

	if(prop.IsType(udm::Type::Element)) {
		ss << "{\n";
		auto first = true;
		for(auto &pair : udm::ElIt {const_cast<udm::LinkedPropertyWrapper &>(prop)}) {
			if(first)
				first = false;
			else
				ss << ",\n";
			ss << tsub << "\"" << pair.key << "\": ";
			to_json(pair.property, ss, tsub);
		}
		ss << "\n" << t << "}";
		return;
	}

	if(prop.GetType() == udm::Type::Blob || prop.GetType() == udm::Type::BlobLz4) {
		uint64_t blobSize;
		auto res = prop.GetBlobData(nullptr, 0ull, &blobSize);
		if(res == udm::BlobResult::InsufficientSize) {
			std::vector<uint8_t> blobData;
			blobData.resize(blobSize);
			auto res = prop.GetBlobData(blobData.data(), blobData.size());
			if(res == udm::BlobResult::Success) {
				auto encoded = util::base64_encode(blobData.data(), blobData.size());
				ss << "\"" << encoded << "\"";
				return;
			}
		}
		ss << "\"\"";
		return;
	}

	auto strVal = prop.ToValue<udm::String>();
	assert(strVal.has_value());
	if(strVal.has_value())
		ss << "\"" << *strVal << "\"";
}

void udm::to_json(LinkedPropertyWrapperArg prop, std::stringstream &ss) { ::to_json(prop, ss, ""); }
