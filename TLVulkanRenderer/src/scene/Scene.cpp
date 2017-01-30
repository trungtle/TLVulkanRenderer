/* Parsing code adapted from glTF loader example from
 * https://github.com/syoyo/tinygltfloader/blob/master/loader_example.cc
 * 
 * Parsing code adapted from GPU Programming, University of Pennsylvania 2016,
 * https://github.com/CIS565-Fall-2016/Project4-CUDA-Rasterizer
 *
 */

#define TINYGLTF_LOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Scene.h"

static std::map<int, int> GLTF_COMPONENT_LENGTH_LOOKUP = {
	{TINYGLTF_TYPE_SCALAR, 1},
	{TINYGLTF_TYPE_VEC2, 2},
	{TINYGLTF_TYPE_VEC3, 3},
	{TINYGLTF_TYPE_VEC4, 4},
	{TINYGLTF_TYPE_MAT2, 4},
	{TINYGLTF_TYPE_MAT3, 9},
	{TINYGLTF_TYPE_MAT4, 16}
};

static std::map<int, int> GLTF_COMPONENT_BYTE_SIZE_LOOKUP = {
	{TINYGLTF_COMPONENT_TYPE_BYTE , 1},
	{TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, 1},
	{TINYGLTF_COMPONENT_TYPE_SHORT, 2},
	{TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, 2},
	{TINYGLTF_COMPONENT_TYPE_FLOAT, 4}
};

static std::string PrintMode(int mode) {
	if (mode == TINYGLTF_MODE_POINTS) {
		return "POINTS";
	}
	else if (mode == TINYGLTF_MODE_LINE) {
		return "LINE";
	}
	else if (mode == TINYGLTF_MODE_LINE_LOOP) {
		return "LINE_LOOP";
	}
	else if (mode == TINYGLTF_MODE_TRIANGLES) {
		return "TRIANGLES";
	}
	else if (mode == TINYGLTF_MODE_TRIANGLE_FAN) {
		return "TRIANGLE_FAN";
	}
	else if (mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
		return "TRIANGLE_STRIP";
	}
	return "**UNKNOWN**";
}

static std::string PrintTarget(int target) {
	if (target == 34962) {
		return "GL_ARRAY_BUFFER";
	}
	else if (target == 34963) {
		return "GL_ELEMENT_ARRAY_BUFFER";
	}
	else {
		return "**UNKNOWN**";
	}
}

static std::string PrintType(int ty) {
	if (ty == TINYGLTF_TYPE_SCALAR) {
		return "SCALAR";
	}
	else if (ty == TINYGLTF_TYPE_VECTOR) {
		return "VECTOR";
	}
	else if (ty == TINYGLTF_TYPE_VEC2) {
		return "VEC2";
	}
	else if (ty == TINYGLTF_TYPE_VEC3) {
		return "VEC3";
	}
	else if (ty == TINYGLTF_TYPE_VEC4) {
		return "VEC4";
	}
	else if (ty == TINYGLTF_TYPE_MATRIX) {
		return "MATRIX";
	}
	else if (ty == TINYGLTF_TYPE_MAT2) {
		return "MAT2";
	}
	else if (ty == TINYGLTF_TYPE_MAT3) {
		return "MAT3";
	}
	else if (ty == TINYGLTF_TYPE_MAT4) {
		return "MAT4";
	}
	return "**UNKNOWN**";
}

static std::string PrintShaderType(int ty) {
	if (ty == TINYGLTF_SHADER_TYPE_VERTEX_SHADER) {
		return "VERTEX_SHADER";
	}
	else if (ty == TINYGLTF_SHADER_TYPE_FRAGMENT_SHADER) {
		return "FRAGMENT_SHADER";
	}
	return "**UNKNOWN**";
}

static std::string PrintComponentType(int ty) {
	if (ty == TINYGLTF_COMPONENT_TYPE_BYTE) {
		return "BYTE";
	}
	else if (ty == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
		return "UNSIGNED_BYTE";
	}
	else if (ty == TINYGLTF_COMPONENT_TYPE_SHORT) {
		return "SHORT";
	}
	else if (ty == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
		return "UNSIGNED_SHORT";
	}
	else if (ty == TINYGLTF_COMPONENT_TYPE_INT) {
		return "INT";
	}
	else if (ty == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
		return "UNSIGNED_INT";
	}
	else if (ty == TINYGLTF_COMPONENT_TYPE_FLOAT) {
		return "FLOAT";
	}
	else if (ty == TINYGLTF_COMPONENT_TYPE_DOUBLE) {
		return "DOUBLE";
	}

	return "**UNKNOWN**";
}

static std::string PrintParameterType(int ty) {
	if (ty == TINYGLTF_PARAMETER_TYPE_BYTE) {
		return "BYTE";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) {
		return "UNSIGNED_BYTE";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_SHORT) {
		return "SHORT";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) {
		return "UNSIGNED_SHORT";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_INT) {
		return "INT";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT) {
		return "UNSIGNED_INT";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT) {
		return "FLOAT";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC2) {
		return "FLOAT_VEC2";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3) {
		return "FLOAT_VEC3";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC4) {
		return "FLOAT_VEC4";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_INT_VEC2) {
		return "INT_VEC2";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_INT_VEC3) {
		return "INT_VEC3";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_INT_VEC4) {
		return "INT_VEC4";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL) {
		return "BOOL";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL_VEC2) {
		return "BOOL_VEC2";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL_VEC3) {
		return "BOOL_VEC3";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL_VEC4) {
		return "BOOL_VEC4";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_MAT2) {
		return "FLOAT_MAT2";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_MAT3) {
		return "FLOAT_MAT3";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_MAT4) {
		return "FLOAT_MAT4";
	}
	else if (ty == TINYGLTF_PARAMETER_TYPE_SAMPLER_2D) {
		return "SAMPLER_2D";
	}

	return "**UNKNOWN**";
}

static std::string PrintWrapMode(int mode) {
	if (mode == TINYGLTF_TEXTURE_WRAP_RPEAT) {
		return "REPEAT";
	}
	else if (mode == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) {
		return "CLAMP_TO_EDGE";
	}
	else if (mode == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) {
		return "MIRRORED_REPEAT";
	}

	return "**UNKNOWN**";
}

static std::string PrintFilterMode(int mode) {
	if (mode == TINYGLTF_TEXTURE_FILTER_NEAREST) {
		return "NEAREST";
	}
	else if (mode == TINYGLTF_TEXTURE_FILTER_LINEAR) {
		return "LINEAR";
	}
	else if (mode == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST) {
		return "NEAREST_MIPMAP_NEAREST";
	}
	else if (mode == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR) {
		return "NEAREST_MIPMAP_LINEAR";
	}
	else if (mode == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST) {
		return "LINEAR_MIPMAP_NEAREST";
	}
	else if (mode == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR) {
		return "LINEAR_MIPMAP_LINEAR";
	}
	return "**UNKNOWN**";
}

static std::string PrintFloatArray(const std::vector<double>& arr) {
	if (arr.size() == 0) {
		return "";
	}

	std::stringstream ss;
	ss << "[ ";
	for (size_t i = 0; i < arr.size(); i++) {
		ss << arr[i] << ((i != arr.size() - 1) ? ", " : "");
	}
	ss << " ]";

	return ss.str();
}

static std::string PrintStringArray(const std::vector<std::string>& arr) {
	if (arr.size() == 0) {
		return "";
	}

	std::stringstream ss;
	ss << "[ ";
	for (size_t i = 0; i < arr.size(); i++) {
		ss << arr[i] << ((i != arr.size() - 1) ? ", " : "");
	}
	ss << " ]";

	return ss.str();
}

static std::string Indent(const int indent) {
	std::string s;
	for (int i = 0; i < indent; i++) {
		s += "  ";
	}

	return s;
}

static std::string PrintParameterValue(const tinygltf::Parameter& param) {
	if (!param.number_array.empty()) {
		return PrintFloatArray(param.number_array);
	}
	else {
		return param.string_value;
	}
}

static std::string PrintValue(const std::string& name, const tinygltf::Value& value, const int indent) {
	std::stringstream ss;

	if (value.IsObject()) {
		const tinygltf::Value::Object& o = value.Get<tinygltf::Value::Object>();
		tinygltf::Value::Object::const_iterator it(o.begin());
		tinygltf::Value::Object::const_iterator itEnd(o.end());
		for (; it != itEnd; it++) {
			ss << PrintValue(name, it->second, indent + 1);
		}
	}
	else if (value.IsString()) {
		ss << Indent(indent) << name << " : " << value.Get<std::string>() << std::endl;
	}
	else if (value.IsBool()) {
		ss << Indent(indent) << name << " : " << value.Get<bool>() << std::endl;
	}
	else if (value.IsNumber()) {
		ss << Indent(indent) << name << " : " << value.Get<double>() << std::endl;
	}
	else if (value.IsInt()) {
		ss << Indent(indent) << name << " : " << value.Get<int>() << std::endl;
	}
	// @todo { binary, array }

	return ss.str();
}

static void DumpNode(const tinygltf::Node& node, int indent) {
	std::cout << Indent(indent) << "name        : " << node.name << std::endl;
	std::cout << Indent(indent) << "camera      : " << node.camera << std::endl;
	if (!node.rotation.empty()) {
		std::cout << Indent(indent)
				<< "rotation    : " << PrintFloatArray(node.rotation)
				<< std::endl;
	}
	if (!node.scale.empty()) {
		std::cout << Indent(indent)
				<< "scale       : " << PrintFloatArray(node.scale) << std::endl;
	}
	if (!node.translation.empty()) {
		std::cout << Indent(indent)
				<< "translation : " << PrintFloatArray(node.translation)
				<< std::endl;
	}

	if (!node.matrix.empty()) {
		std::cout << Indent(indent)
				<< "matrix      : " << PrintFloatArray(node.matrix) << std::endl;
	}

	std::cout << Indent(indent)
			<< "meshes      : " << PrintStringArray(node.meshes) << std::endl;

	std::cout << Indent(indent)
			<< "children    : " << PrintStringArray(node.children) << std::endl;
}

static void DumpStringMap(const std::map<std::string, std::string>& map,
                          int indent) {
	std::map<std::string, std::string>::const_iterator it(map.begin());
	std::map<std::string, std::string>::const_iterator itEnd(map.end());
	for (; it != itEnd; it++) {
		std::cout << Indent(indent) << it->first << ": " << it->second << std::endl;
	}
}

static void DumpPrimitive(const tinygltf::Primitive& primitive, int indent) {
	std::cout << Indent(indent) << "material : " << primitive.material
			<< std::endl;
	std::cout << Indent(indent) << "indices : " << primitive.indices << std::endl;
	std::cout << Indent(indent) << "mode     : " << PrintMode(primitive.mode)
			<< "(" << primitive.mode << ")" << std::endl;
	std::cout << Indent(indent)
			<< "attributes(items=" << primitive.attributes.size() << ")"
			<< std::endl;
	DumpStringMap(primitive.attributes, indent + 1);

	std::cout << Indent(indent)
			<< "extras :" << std::endl
			<< PrintValue("extras", primitive.extras, indent + 1) << std::endl;
}

static void DumpTechniqueParameter(const tinygltf::TechniqueParameter& param,
                                   int indent) {
	std::cout << Indent(indent) << "count    : " << param.count << std::endl;
	std::cout << Indent(indent) << "node     : " << param.node << std::endl;
	std::cout << Indent(indent) << "semantic : " << param.semantic << std::endl;
	std::cout << Indent(indent) << "type     : " << PrintParameterType(param.type)
			<< std::endl;
	std::cout << Indent(indent)
			<< "value    : " << PrintParameterValue(param.value) << std::endl;
}

static void Dump(const tinygltf::Scene& scene) {
	std::cout << "=== Dump glTF ===" << std::endl;
	std::cout << "asset.generator          : " << scene.asset.generator
			<< std::endl;
	std::cout << "asset.premultipliedAlpha : " << scene.asset.premultipliedAlpha
			<< std::endl;
	std::cout << "asset.version            : " << scene.asset.version
			<< std::endl;
	std::cout << "asset.profile.api        : " << scene.asset.profile_api
			<< std::endl;
	std::cout << "asset.profile.version    : " << scene.asset.profile_version
			<< std::endl;
	std::cout << std::endl;
	std::cout << "=== Dump scene ===" << std::endl;
	std::cout << "defaultScene: " << scene.defaultScene << std::endl; {
		std::map<std::string, std::vector<std::string>>::const_iterator it(
			scene.scenes.begin());
		std::map<std::string, std::vector<std::string>>::const_iterator itEnd(
			scene.scenes.end());
		std::cout << "scenes(items=" << scene.scenes.size() << ")" << std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name  : " << it->first << std::endl;
			std::cout << Indent(2) << "nodes : [ ";
			for (size_t i = 0; i < it->second.size(); i++) {
				std::cout << it->second[i]
						<< ((i != (it->second.size() - 1)) ? ", " : "");
			}
			std::cout << " ] " << std::endl;
		}
	} {
		std::map<std::string, tinygltf::Mesh>::const_iterator it(
			scene.meshes.begin());
		std::map<std::string, tinygltf::Mesh>::const_iterator itEnd(
			scene.meshes.end());
		std::cout << "meshes(item=" << scene.meshes.size() << ")" << std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name     : " << it->second.name << std::endl;
			std::cout << Indent(1)
					<< "primitives(items=" << it->second.primitives.size()
					<< "): " << std::endl;

			for (size_t i = 0; i < it->second.primitives.size(); i++) {
				DumpPrimitive(it->second.primitives[i], 2);
			}
		}
	} {
		std::map<std::string, tinygltf::Accessor>::const_iterator it(
			scene.accessors.begin());
		std::map<std::string, tinygltf::Accessor>::const_iterator itEnd(
			scene.accessors.end());
		std::cout << "accessors(items=" << scene.accessors.size() << ")"
				<< std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name         : " << it->first << std::endl;
			std::cout << Indent(2) << "bufferView   : " << it->second.bufferView
					<< std::endl;
			std::cout << Indent(2) << "byteOffset   : " << it->second.byteOffset
					<< std::endl;
			std::cout << Indent(2) << "byteStride   : " << it->second.byteStride
					<< std::endl;
			std::cout << Indent(2) << "componentType: "
					<< PrintComponentType(it->second.componentType) << "("
					<< it->second.componentType << ")" << std::endl;
			std::cout << Indent(2) << "count        : " << it->second.count
					<< std::endl;
			std::cout << Indent(2) << "type         : " << PrintType(it->second.type)
					<< std::endl;
			if (!it->second.minValues.empty()) {
				std::cout << Indent(2) << "min          : [";
				for (size_t i = 0; i < it->second.minValues.size(); i++) {
					std::cout << it->second.minValues[i]
							<< ((i != it->second.minValues.size() - 1) ? ", " : "");
				}
				std::cout << "]" << std::endl;
			}
			if (!it->second.maxValues.empty()) {
				std::cout << Indent(2) << "max          : [";
				for (size_t i = 0; i < it->second.maxValues.size(); i++) {
					std::cout << it->second.maxValues[i]
							<< ((i != it->second.maxValues.size() - 1) ? ", " : "");
				}
				std::cout << "]" << std::endl;
			}
		}
	} {
		std::map<std::string, tinygltf::Animation>::const_iterator it(
			scene.animations.begin());
		std::map<std::string, tinygltf::Animation>::const_iterator itEnd(
			scene.animations.end());
		std::cout << "animations(items=" << scene.animations.size() << ")"
				<< std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name         : " << it->first << std::endl;

			std::cout << Indent(1) << "channels : [ " << std::endl;
			for (size_t i = 0; i < it->second.channels.size(); i++) {
				std::cout << Indent(2)
						<< "sampler     : " << it->second.channels[i].sampler
						<< std::endl;
				std::cout << Indent(2)
						<< "target.id   : " << it->second.channels[i].target_id
						<< std::endl;
				std::cout << Indent(2)
						<< "target.path : " << it->second.channels[i].target_path
						<< std::endl;
				std::cout << ((i != (it->second.channels.size() - 1)) ? "  , " : "");
			}
			std::cout << "  ]" << std::endl;

			std::map<std::string, tinygltf::AnimationSampler>::const_iterator
					samplerIt(it->second.samplers.begin());
			std::map<std::string, tinygltf::AnimationSampler>::const_iterator
					samplerItEnd(it->second.samplers.end());
			std::cout << Indent(1) << "samplers(items=" << it->second.samplers.size()
					<< ")" << std::endl;
			for (; samplerIt != samplerItEnd; samplerIt++) {
				std::cout << Indent(1) << "name          : " << samplerIt->first
						<< std::endl;
				std::cout << Indent(2) << "input         : " << samplerIt->second.input
						<< std::endl;
				std::cout << Indent(2)
						<< "interpolation : " << samplerIt->second.interpolation
						<< std::endl;
				std::cout << Indent(2) << "output        : " << samplerIt->second.output
						<< std::endl;
			} {
				std::cout << Indent(1)
						<< "parameters(items=" << it->second.parameters.size() << ")"
						<< std::endl;
				tinygltf::ParameterMap::const_iterator p(it->second.parameters.begin());
				tinygltf::ParameterMap::const_iterator pEnd(
					it->second.parameters.end());
				for (; p != pEnd; p++) {
					std::cout << Indent(2) << p->first << ": "
							<< PrintParameterValue(p->second) << std::endl;
				}
			}
		}
	} {
		std::map<std::string, tinygltf::BufferView>::const_iterator it(
			scene.bufferViews.begin());
		std::map<std::string, tinygltf::BufferView>::const_iterator itEnd(
			scene.bufferViews.end());
		std::cout << "bufferViews(items=" << scene.bufferViews.size() << ")"
				<< std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name         : " << it->first << std::endl;
			std::cout << Indent(2) << "buffer       : " << it->second.buffer
					<< std::endl;
			std::cout << Indent(2) << "byteLength   : " << it->second.byteLength
					<< std::endl;
			std::cout << Indent(2) << "byteOffset   : " << it->second.byteOffset
					<< std::endl;
			std::cout << Indent(2)
					<< "target       : " << PrintTarget(it->second.target)
					<< std::endl;
		}
	} {
		std::map<std::string, tinygltf::Buffer>::const_iterator it(
			scene.buffers.begin());
		std::map<std::string, tinygltf::Buffer>::const_iterator itEnd(
			scene.buffers.end());
		std::cout << "buffers(items=" << scene.buffers.size() << ")" << std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name         : " << it->first << std::endl;
			std::cout << Indent(2) << "byteLength   : " << it->second.data.size()
					<< std::endl;
		}
	} {
		std::map<std::string, tinygltf::Material>::const_iterator it(
			scene.materials.begin());
		std::map<std::string, tinygltf::Material>::const_iterator itEnd(
			scene.materials.end());
		std::cout << "materials(items=" << scene.materials.size() << ")"
				<< std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name         : " << it->first << std::endl;
			std::cout << Indent(1) << "technique    : " << it->second.technique
					<< std::endl;
			std::cout << Indent(1) << "values(items=" << it->second.values.size()
					<< ")" << std::endl;

			tinygltf::ParameterMap::const_iterator p(it->second.values.begin());
			tinygltf::ParameterMap::const_iterator pEnd(it->second.values.end());
			for (; p != pEnd; p++) {
				std::cout << Indent(2) << p->first << ": "
						<< PrintParameterValue(p->second) << std::endl;
			}
		}
	} {
		std::map<std::string, tinygltf::Node>::const_iterator it(
			scene.nodes.begin());
		std::map<std::string, tinygltf::Node>::const_iterator itEnd(
			scene.nodes.end());
		std::cout << "nodes(items=" << scene.nodes.size() << ")" << std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name         : " << it->first << std::endl;

			DumpNode(it->second, 2);
		}
	} {
		std::map<std::string, tinygltf::Image>::const_iterator it(
			scene.images.begin());
		std::map<std::string, tinygltf::Image>::const_iterator itEnd(
			scene.images.end());
		std::cout << "images(items=" << scene.images.size() << ")" << std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name         : " << it->first << std::endl;

			std::cout << Indent(2) << "width     : " << it->second.width << std::endl;
			std::cout << Indent(2) << "height    : " << it->second.height
					<< std::endl;
			std::cout << Indent(2) << "component : " << it->second.component
					<< std::endl;
			std::cout << Indent(2) << "name      : " << it->second.name << std::endl;
		}
	} {
		std::map<std::string, tinygltf::Texture>::const_iterator it(
			scene.textures.begin());
		std::map<std::string, tinygltf::Texture>::const_iterator itEnd(
			scene.textures.end());
		std::cout << "textures(items=" << scene.textures.size() << ")" << std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name           : " << it->first << std::endl;
			std::cout << Indent(1) << "format         : " << it->second.format
					<< std::endl;
			std::cout << Indent(1) << "internalFormat : " << it->second.internalFormat
					<< std::endl;
			std::cout << Indent(1) << "sampler        : " << it->second.sampler
					<< std::endl;
			std::cout << Indent(1) << "source         : " << it->second.source
					<< std::endl;
			std::cout << Indent(1) << "target         : " << it->second.target
					<< std::endl;
			std::cout << Indent(1) << "type           : " << it->second.type
					<< std::endl;
		}
	} {
		std::map<std::string, tinygltf::Shader>::const_iterator it(
			scene.shaders.begin());
		std::map<std::string, tinygltf::Shader>::const_iterator itEnd(
			scene.shaders.end());

		std::cout << "shaders(items=" << scene.shaders.size() << ")" << std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name (id)      : " << it->first << std::endl;
			std::cout << Indent(2)
					<< "type           : " << PrintShaderType(it->second.type)
					<< std::endl;

			std::cout << Indent(2) << "name (json)    : " << it->second.name
					<< std::endl;

			// Indent shader source nicely.
			std::string shader_source(Indent(3));
			shader_source.resize(shader_source.size() + it->second.source.size());

			std::vector<unsigned char>::const_iterator sourceIt(
				it->second.source.begin());
			std::vector<unsigned char>::const_iterator sourceItEnd(
				it->second.source.end());

			for (; sourceIt != sourceItEnd; ++sourceIt) {
				shader_source += static_cast<char>(*sourceIt);
				if (*sourceIt == '\n') {
					shader_source += Indent(3);
				}
			}
			std::cout << Indent(2) << "source         :\n"
					<< shader_source << std::endl;
		}
	} {
		std::map<std::string, tinygltf::Program>::const_iterator it(
			scene.programs.begin());
		std::map<std::string, tinygltf::Program>::const_iterator itEnd(
			scene.programs.end());

		std::cout << "programs(items=" << scene.programs.size() << ")" << std::endl;
		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name           : " << it->first << std::endl;
			std::cout << Indent(2) << "vertexShader   : " << it->second.vertexShader
					<< std::endl;
			std::cout << Indent(2) << "fragmentShader : " << it->second.fragmentShader
					<< std::endl;
			std::cout << Indent(2) << "attributes     : "
					<< PrintStringArray(it->second.attributes) << std::endl;
			std::cout << Indent(2) << "name           : " << it->second.name
					<< std::endl;
		}
	} {
		std::map<std::string, tinygltf::Technique>::const_iterator it(
			scene.techniques.begin());
		std::map<std::string, tinygltf::Technique>::const_iterator itEnd(
			scene.techniques.end());

		std::cout << "techniques(items=" << scene.techniques.size() << ")"
				<< std::endl;

		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name (id)    : " << it->first << std::endl;
			std::cout << Indent(2) << "program      : " << it->second.program
					<< std::endl;

			std::cout << Indent(2) << "name (json)  : " << it->second.name
					<< std::endl;

			std::cout << Indent(2)
					<< "parameters(items=" << it->second.parameters.size() << ")"
					<< std::endl;

			std::map<std::string, tinygltf::TechniqueParameter>::const_iterator
					paramIt(it->second.parameters.begin());
			std::map<std::string, tinygltf::TechniqueParameter>::const_iterator
					paramItEnd(it->second.parameters.end());

			for (; paramIt != paramItEnd; ++paramIt) {
				std::cout << Indent(3) << "name     : " << paramIt->first << std::endl;
				DumpTechniqueParameter(paramIt->second, 4);
			}

			std::cout << Indent(2)
					<< "attributes(items=" << it->second.attributes.size() << ")"
					<< std::endl;

			DumpStringMap(it->second.attributes, 3);

			std::cout << Indent(2) << "uniforms(items=" << it->second.uniforms.size()
					<< ")" << std::endl;
			DumpStringMap(it->second.uniforms, 3);
		}
	} {
		std::map<std::string, tinygltf::Sampler>::const_iterator it(
			scene.samplers.begin());
		std::map<std::string, tinygltf::Sampler>::const_iterator itEnd(
			scene.samplers.end());

		std::cout << "samplers(items=" << scene.samplers.size() << ")" << std::endl;

		for (; it != itEnd; it++) {
			std::cout << Indent(1) << "name (id)    : " << it->first << std::endl;
			std::cout << Indent(2)
					<< "minFilter    : " << PrintFilterMode(it->second.minFilter)
					<< std::endl;
			std::cout << Indent(2)
					<< "magFilter    : " << PrintFilterMode(it->second.magFilter)
					<< std::endl;
			std::cout << Indent(2)
					<< "wrapS        : " << PrintWrapMode(it->second.wrapS)
					<< std::endl;
			std::cout << Indent(2)
					<< "wrapT        : " << PrintWrapMode(it->second.wrapT)
					<< std::endl;
		}
	}
}

glm::mat4 GetMatrixFromGLTFNode(const tinygltf::Node& node) {

	glm::mat4 curMatrix(1.0);

	const std::vector<double>& matrix = node.matrix;
	if (matrix.size() > 0) {
		// matrix, copy it

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				curMatrix[i][j] = (float)matrix.at(4 * i + j);
			}
		}
	}
	else {
		// no matrix, use rotation, scale, translation

		if (node.translation.size() > 0) {
			curMatrix[3][0] = node.translation[0];
			curMatrix[3][1] = node.translation[1];
			curMatrix[3][2] = node.translation[2];
		}

		if (node.rotation.size() > 0) {
			glm::mat4 R;
			glm::quat q;
			q[0] = node.rotation[0];
			q[1] = node.rotation[1];
			q[2] = node.rotation[2];

			R = glm::mat4_cast(q);
			curMatrix = curMatrix * R;
		}

		if (node.scale.size() > 0) {
			curMatrix = curMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
		}
	}

	return curMatrix;
}

void TraverseGLTFNode(
	std::map<std::string, glm::mat4>& n2m,
	const tinygltf::Scene& scene,
	const std::string& nodeString,
	const glm::mat4& parentMatrix
) {
	const tinygltf::Node& node = scene.nodes.at(nodeString);
	glm::mat4 worldTransformationMatrix = parentMatrix * GetMatrixFromGLTFNode(node);
	n2m.insert(std::pair<std::string, glm::mat4>(nodeString, worldTransformationMatrix));

	for (auto& child : node.children) {
		TraverseGLTFNode(n2m, scene, child, worldTransformationMatrix);
	}
}

static std::string GetFilePathExtension(const std::string& FileName) {
	if (FileName.find_last_of(".") != std::string::npos)
		return FileName.substr(FileName.find_last_of(".") + 1);
	return "";
}

Scene::Scene(
	std::string fileName
) {
	tinygltf::Scene scene;
	tinygltf::TinyGLTFLoader loader;
	std::string err;
	std::string ext = GetFilePathExtension(fileName);

	bool ret = false;
	if (ext.compare("glb") == 0) {
		// binary glTF.
		ret = loader.LoadBinaryFromFile(&scene, &err, fileName.c_str());
	}
	else {
		// ascii glTF.
		ret = loader.LoadASCIIFromFile(&scene, &err, fileName.c_str());
	}

	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
		return;
	}

	// ----------- Transformation matrix --------- 
	std::map<std::string, glm::mat4> nodeString2Matrix;
	auto rootNodeNamesList = scene.scenes.at(scene.defaultScene);
	for (auto& sceneNode : rootNodeNamesList) {
		TraverseGLTFNode(nodeString2Matrix, scene, sceneNode, glm::mat4(1.0f));
	}

	// -------- For each mesh -----------

	for (auto& nodeString : nodeString2Matrix) {

		const tinygltf::Node& node = scene.nodes.at(nodeString.first);
		const glm::mat4& matrix = nodeString.second;
		const glm::mat3& matrixNormal = glm::transpose(glm::inverse(glm::mat3(matrix)));

		int materialId = 0;
		int idxOffset = 0;
		for (auto& meshName : node.meshes) {
			auto& mesh = scene.meshes.at(meshName);
			for (size_t i = 0; i < mesh.primitives.size(); i++) {
				auto primitive = mesh.primitives[i];
				if (primitive.indices.empty()) {
					return;
				}

				MeshData* geom = new MeshData();

				// -------- Indices ----------
				{
					// Get accessor info
					auto indexAccessor = scene.accessors.at(primitive.indices);
					auto indexBufferView = scene.bufferViews.at(indexAccessor.bufferView);
					auto indexBuffer = scene.buffers.at(indexBufferView.buffer);

					int componentLength = GLTF_COMPONENT_LENGTH_LOOKUP.at(indexAccessor.type);
					int componentTypeByteSize = GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(indexAccessor.componentType);

					// Extra index data
					int bufferOffset = indexBufferView.byteOffset + indexAccessor.byteOffset;
					int bufferLength = indexAccessor.count * componentLength * componentTypeByteSize;
					auto first = indexBuffer.data.begin() + bufferOffset;
					auto last = indexBuffer.data.begin() + bufferOffset + bufferLength;
					std::vector<Byte> data = std::vector<Byte>(first, last);

					VertexAttributeInfo attributeInfo = {
						indexAccessor.byteStride,
						indexAccessor.count,
						componentLength,
						componentTypeByteSize
					};
					geom->vertexAttributes.insert(std::make_pair(EVertexAttributeType::INDEX, attributeInfo));
					geom->vertexData.insert(std::make_pair(EVertexAttributeType::INDEX, data));

					int indicesCount = indexAccessor.count;
					uint16_t* in = reinterpret_cast<uint16_t*>(data.data());
					for (auto iCount = 0; iCount < indicesCount; iCount += 3) {
						indices.push_back(glm::ivec4(in[iCount], in[iCount + 1], in[iCount + 2], materialId));
					}
				}

				// -------- Attributes -----------

				for (auto& attribute : primitive.attributes) {

					// Get accessor info
					auto& accessor = scene.accessors.at(attribute.second);
					auto& bufferView = scene.bufferViews.at(accessor.bufferView);
					auto& buffer = scene.buffers.at(bufferView.buffer);
					int componentLength = GLTF_COMPONENT_LENGTH_LOOKUP.at(accessor.type);
					int componentTypeByteSize = GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(accessor.componentType);

					// Extra vertex data from buffer
					int bufferOffset = bufferView.byteOffset + accessor.byteOffset;
					int bufferLength = accessor.count * componentLength * componentTypeByteSize;
					auto first = buffer.data.begin() + bufferOffset;
					auto last = buffer.data.begin() + bufferOffset + bufferLength;
					std::vector<Byte> data = std::vector<Byte>(first, last);

					EVertexAttributeType attributeType;

					// -------- Position attribute -----------

					if (attribute.first.compare("POSITION") == 0) {
						attributeType = EVertexAttributeType::POSITION;
						int positionCount = accessor.count;
						glm::vec3* positions = reinterpret_cast<glm::vec3*>(data.data());
						for (auto p = 0; p < positionCount; ++p) {
							positions[p] = glm::vec3(matrix * glm::vec4(positions[p], 1.0f));
							verticePositions.push_back(glm::vec4(positions[p], 1.0f));
						}
					}

					// -------- Normal attribute -----------

					else if (attribute.first.compare("NORMAL") == 0) {
						attributeType = EVertexAttributeType::NORMAL;
						int normalCount = accessor.count;
						glm::vec3* normals = reinterpret_cast<glm::vec3*>(data.data());
						for (auto p = 0; p < normalCount; ++p) {
							normals[p] = glm::normalize(matrixNormal * glm::vec4(normals[p], 1.0f));
							verticeNormals.push_back(glm::vec4(normals[p], 0.0f));
						}
					}

					// -------- Texcoord attribute -----------

					else if (attribute.first.compare("TEXCOORD_0") == 0) {
						attributeType = EVertexAttributeType::TEXCOORD;
						int uvCount = accessor.count;
						glm::vec2* uvs = reinterpret_cast<glm::vec2*>(data.data());
						for (auto p = 0; p < uvCount; ++p)
						{
							verticeUVs.push_back(uvs[p]);
						}
					}

					VertexAttributeInfo attributeInfo = {
						accessor.byteStride,
						accessor.count,
						componentLength,
						componentTypeByteSize
					};
					geom->vertexAttributes.insert(std::make_pair(attributeType, attributeInfo));
					geom->vertexData.insert(std::make_pair(attributeType, data));

					// ----------Materials-------------

					//TextureData* dev_diffuseTex = NULL;
					MaterialPacked materialPacked;
					Texture texture;
					if (!primitive.material.empty()) {
						const tinygltf::Material& mat = scene.materials.at(primitive.material);
						printf("material.name = %s\n", mat.name.c_str());

						if (mat.values.find("diffuse") != mat.values.end()) {
							std::string diffuseTexName = mat.values.at("diffuse").string_value;
							if (scene.textures.find(diffuseTexName) != scene.textures.end()) {
								const tinygltf::Texture& tex = scene.textures.at(diffuseTexName);
								if (scene.images.find(tex.source) != scene.images.end()) {
									const tinygltf::Image& image = scene.images.at(tex.source);

									// Texture bytes
									texture.width = image.width;
									texture.height = image.height;
									texture.component = image.component;
									texture.name = image.name;
									texture.image = image.image;
								}
							}
							else {
								auto diff = mat.values.at("diffuse").number_array;
								materialPacked.diffuse = glm::vec4(diff.at(0), diff.at(1), diff.at(2), diff.at(3));
							}
						}

						if (mat.values.find("ambient") != mat.values.end()) {
							auto amb = mat.values.at("ambient").number_array;
							materialPacked.ambient = glm::vec4(amb.at(0), amb.at(1), amb.at(2), amb.at(3));
						}
						if (mat.values.find("emission") != mat.values.end()) {
							auto em = mat.values.at("emission").number_array;
							materialPacked.emission = glm::vec4(em.at(0), em.at(1), em.at(2), em.at(3));

						}
						if (mat.values.find("specular") != mat.values.end()) {
							auto spec = mat.values.at("specular").number_array;
							materialPacked.specular = glm::vec4(spec.at(0), spec.at(1), spec.at(2), spec.at(3));

						}
						if (mat.values.find("shininess") != mat.values.end()) {
							materialPacked.shininess = mat.values.at("shininess").number_array.at(0);
						}

						if (mat.values.find("transparency") != mat.values.end()) {
							materialPacked.transparency = mat.values.at("transparency").number_array.at(0);
						}
						else {
							materialPacked.transparency = 1.0f;
						}

						materials.push_back(LambertMaterial(materialPacked, texture));

						materialPackeds.push_back(materialPacked);
						++materialId;
			
					} // --End of materials
				} // -- End of attributes

				meshesData.push_back(geom);

				Mesh newMesh;
				for (int j = idxOffset; j < indices.size(); j++)
				{
					ivec4 idx = indices[j];

					if (this->verticeUVs.size() == 0) {
						// No UVs
						newMesh.triangles.push_back(
							Triangle(
								this->verticePositions[idx.x],
								this->verticePositions[idx.y],
								this->verticePositions[idx.z],
								this->verticeNormals[idx.x],
								this->verticeNormals[idx.y],
								this->verticeNormals[idx.z],
								&materials[materialId - 1]
							)
						);
					}
					else
					{
						newMesh.triangles.push_back(
							Triangle(
								this->verticePositions[idx.x],
								this->verticePositions[idx.y],
								this->verticePositions[idx.z],
								this->verticeNormals[idx.x],
								this->verticeNormals[idx.y],
								this->verticeNormals[idx.z],
								this->verticeUVs[idx.x],
								this->verticeUVs[idx.y],
								this->verticeUVs[idx.z],
								&materials[materialId - 1]
							)
						);
					}
				}
				meshes.push_back(newMesh);
				idxOffset = indices.size();
			} // -- End of mesh primitives
		} // -- End of meshes
	}

	sbvh = SBVH(
		1,
		SBVH::SAH
		);

	std::vector<Geometry*> geoms;
	for (int m = 0; m < meshes.size(); m++)
	{
		for (int t = 0; t < meshes[m].triangles.size(); t++)
		{
			geoms.push_back(&meshes[m].triangles[t]);
		}
	}

	for (int i = 0; i < 12; i++) {
		geoms.push_back(new Sphere(glm::vec3(
			sin(glm::radians(360.0f * i / 12.0f)) * 2, 
			cos(glm::radians(360.0f * i / 12.0f)) * 2, 
			0), 0.5, &materials[0]));
	}

	sbvh.Build(geoms);

	//Dump(scene);
}


Scene::~Scene() {
	for (MeshData* geom : meshesData) {
		delete geom;
		geom = nullptr;
	}
}
