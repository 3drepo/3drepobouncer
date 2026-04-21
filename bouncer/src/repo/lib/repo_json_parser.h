/**
*  Copyright (C) 2025 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "repo/lib/rapidjson/reader.h"
#include "repo/lib/rapidjson/error/en.h"

#include <stack>
#include <map>

#include <repo/lib/datastructure/repo_uuid.h>
#include <repo/lib/datastructure/repo_bounds.h>
#include <repo/lib/datastructure/repo_matrix.h>
#include <repo/lib/datastructure/repo_vector.h>
#include <repo/lib/datastructure/repo_structs.h>
#include <repo/lib/datastructure/repo_variant.h>

/**
* This file contains a set of types to help process tree structured data using
* RapidJson.
* 
* RapidJson provides a JAX, or, event-based, API that can operate on streams
* and so has better memory behaviour when parsing large files. The event-based
* API issues callbacks to a Handler when encountering different tokens or values
* in a Json stream.
* 
* This file provides a subclass of the RapidJson Handler that maintains a stack
* of objects, each of which can customise how the primitives are handled to
* build different concrete types. 
*/

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace json {
				/*
				* Parsers map primitives from the Json to members of more complex structures.
				*
				* Parsers typically hold a reference to the variable they are meant to update,
				* and when they receive the value via one of the base methods, they can perform
				* any necessary post-processing, and assign it.
				*
				* Parser objects can also respond to object & array delimiters and keys; if a
				* parser returns another Parser pointer, the new Parser will be added to the
				* stack, and used for any subsequent values/tokens, until the scope (object or
				* array) exits. In this way a tree of Parser objects can be constructed
				* to handle complex types with multiple nested types, such as Nodes or
				* Bounding Boxes.
				*
				* It is expected the Parser tree is created once per-application, and the
				* memory sections that it updates are written to over and over as new objects
				* are read from the Json.
				*/

				struct Parser
				{
					virtual void Null() { SetError("Null"); }
					virtual void Bool(bool b) { SetError("Bool"); }
					virtual void Int(int i) { SetError("Int"); }
					virtual void Uint(unsigned i) { SetError("Unsigned Int"); }
					virtual void Int64(int64_t i) { SetError("Int64"); }
					virtual void Uint64(uint64_t i) { SetError("Unsigned Int64"); }
					virtual void Double(double d) { SetError("Double"); }
					virtual void String(const std::string_view& string) { SetError("String"); }
					virtual void StartObject() { SetError("Object"); }
					virtual Parser* Key(const std::string_view& key) { SetError("Key"); return nullptr; }
					virtual void EndObject() {}
					virtual Parser* StartArray() { SetError("Array"); }
					virtual void EndArray() {}

					virtual std::string GetExpected() const {
						return "Unknown";
					}

					virtual void SetError(std::string actual) {
						error = "Json does not match expected schema - expected " + GetExpected() + " but found " + actual;
					}

					/*
					* This is set to true by the default implementations, which will cause
					* the handler to terminate in strict mode. Override all possibly expected
					* tokens to avoid this being set.
					*/
					std::string error;
				};

				struct ObjectParser : public Parser
				{
					std::map<std::string, Parser*, std::less<>> parsers;

					virtual Parser* Key(const std::string_view& key) override {
						auto it = parsers.find(key);
						if (it != parsers.end()) {
							return it->second;
						}
						else {
							return nullptr;
						}
					}

					virtual void StartObject() override {
					}

					virtual std::string GetExpected() const override {
						return "Object";
					}
				};

				/*
				* Tells the handler to call elementParser for each element of the array
				*/
				struct ArrayParser : public Parser
				{
					Parser* elementParser;

					ArrayParser(Parser* elementParser)
						:elementParser(elementParser) {
					}

					virtual Parser* StartArray() {
						return elementParser;
					}

					virtual std::string GetExpected() const override {
						return "Array";
					}
				};

				/*
				* Casts any number received to the desired type and outputs it via the single
				* abstract method.
				*/
				template<typename T>
				struct NumberParserBase : public Parser
				{
					virtual void Int(int i) override {
						Number((T)i);
					}

					virtual void Uint(unsigned i) override {
						Number((T)i);
					}

					virtual void Int64(int64_t i) override {
						Number((T)i);
					}

					virtual void Uint64(uint64_t i) override {
						Number((T)i);
					}

					virtual void Double(double d) override {
						Number((T)d);
					}

					virtual void Number(T v) = 0;

					virtual std::string GetExpected() const override {
						return "Number";
					}
				};

				template<typename T>
				struct NumberParser : public NumberParserBase<T>
				{
					T& v;

					NumberParser(T& v)
						:v(v)
					{
					}

					virtual void Number(T v) override {
						this->v = v;
					}
				};

				struct StringParser : public Parser
				{
					std::string& v;

					StringParser(std::string& v)
						:v(v)
					{
					}

					virtual void String(const std::string_view& s) override {
						v = s;
					}

					virtual std::string GetExpected() const override {
						return "String";
					}
				};

				// Reads an array as a RepoVector
				struct VectorParser : public NumberParserBase<double>
				{
					repo::lib::RepoVector3D64& vec;
					int i;

					VectorParser(repo::lib::RepoVector3D64& vec)
						:i(0),
						vec(vec)
					{
					}

					virtual Parser* StartArray() override {
						vec = repo::lib::RepoVector3D64();
						i = 0;
						return this;
					}

					virtual void Number(double d) override {
						if (i < 3) {
							((double*)&vec)[i++] = d;
						}
					}

					virtual std::string GetExpected() const override {
						return "RepoVector in Array form";
					}
				};

				struct BoundsParser : public ObjectParser
				{
					repo::lib::RepoVector3D64 min;
					repo::lib::RepoVector3D64 max;

					repo::lib::RepoBounds& bounds;

					BoundsParser(repo::lib::RepoBounds& bounds) :
						bounds(bounds)
					{
						parsers["min"] = new VectorParser(min);
						parsers["max"] = new VectorParser(max);
					}

					void EndObject() override {
						bounds = repo::lib::RepoBounds(min, max);
					}

					virtual std::string GetExpected() const override {
						return "Bounds in Object Form";
					}
				};

				struct RepoVariantParser : public Parser
				{
					repo::lib::RepoVariant* v;

					RepoVariantParser() {
						v = nullptr;
					}

					virtual void Bool(bool b) { *v = b; }
					virtual void Int(int i) { *v = i; }
					virtual void Uint(unsigned i) { *v = (int)i; }
					virtual void Int64(int64_t i) { *v = i; }
					virtual void Uint64(uint64_t i) { *v = (int64_t)i; }
					virtual void Double(double d) { *v = d; }
					virtual void String(const std::string_view& string) { *v = std::string(string); }

					virtual std::string GetExpected() const override {
						return "Variant (one of Bool, Number or String)";
					}
				};

				struct RepoUUIDParser : public Parser
				{
					repo::lib::RepoUUID& uuid;

					RepoUUIDParser(repo::lib::RepoUUID& uuid)
						:uuid(uuid)
					{
					}

					virtual void String(const std::string_view& string) override {
						uuid = repo::lib::RepoUUID(std::string(string));
					}

					virtual std::string GetExpected() const override {
						return "RepoUUID";
					}
				};

				struct RepoMatrixParser : public NumberParserBase<float>
				{
					repo::lib::RepoMatrix& matrix;
					int i;

					RepoMatrixParser(repo::lib::RepoMatrix& matrix)
						:matrix(matrix),
						i(0)
					{
					}

					virtual Parser* StartArray() {
						matrix = repo::lib::RepoMatrix();
						i = 0;
						return this;
					}

					void Number(float d) override {
						if (i < 16) {
							matrix.getData()[i++] = d;
						}
					}

					virtual std::string GetExpected() const override {
						return "Matrix in Array Form";
					}
				};

				struct ColourParser : public NumberParserBase<float>
				{
					int i;
					repo::lib::repo_color3d_t& colour;

					ColourParser(repo::lib::repo_color3d_t& colour) :
						colour(colour),
						i(0)
					{
					}

					virtual Parser* StartArray() {
						i = 0;
						return this;
					}

					virtual void Number(float d) override {
						if (i < 3) {
							((float*)&colour)[i++] = d;
						}
					}

					virtual std::string GetExpected() const override {
						return "Colour in Array Form";
					}
				};

				struct BoolParser : public Parser
				{
					bool& v;
					BoolParser(bool& v)
						:v(v)
					{
					}

					virtual void Bool(bool b) override {
						v = b;
					}

					virtual std::string GetExpected() const override {
						return "Boolean";
					}
				};

				/*
				* Calls the parser at the top of the stack for any value. When arrays or objects
				* are encountered, the Parser has the opportunity to push a new parser to the
				* stack for the duration of that scope.
				* Objects are additionally special in that they have two stack entries: one
				* for the object Parser itself, and one for the Parser for each key. When
				* key is called inside an Object, the top of the stack is *swapped* for the new
				* Parser. When the object terminates, the top Parser is discarded and EndObject
				* called on the Parser on which StartObject was called.
				*/

				struct MyHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, MyHandler>
				{
					bool StartObject() {
						if (stack.top()) {
							stack.top()->StartObject();
						}
						stack.push(nullptr); // This one is to hold the Parsers for the objects' members. This should be updated after the call to Key before any values are read.
						return checkTopParser();
					}

					bool Key(const Ch* str, rapidjson::SizeType length, bool copy) {
						stack.pop();
						stack.push(stack.top() ? stack.top()->Key(std::string_view(str, length)) : nullptr);
						return checkTopParser();
					}

					bool EndObject(rapidjson::SizeType) {
						stack.pop(); // (The key's parser, or nullptr if one was never set.)
						if (stack.top()) {
							stack.top()->EndObject();
						}
						return checkTopParser();
					}

					bool StartArray() {
						stack.push(stack.top() ? stack.top()->StartArray() : nullptr);
						return checkTopParser();
					}

					bool EndArray(rapidjson::SizeType) {
						stack.pop();
						if (stack.top()) {
							stack.top()->EndArray();
						}
						return checkTopParser();
					}

					bool String(const Ch* str, rapidjson::SizeType length, bool copy) {
						if (stack.top()) {
							stack.top()->String(std::string_view(str, length));
						}
						return checkTopParser();
					}

					bool Bool(bool b) {
						if (stack.top()) {
							stack.top()->Bool(b);
						}
						return checkTopParser();
					}

					bool Int(int v) {
						if (stack.top()) {
							stack.top()->Int(v);
						}
						return checkTopParser();
					}

					bool Uint(unsigned v) {
						if (stack.top()) {
							stack.top()->Uint(v);
						}
						return checkTopParser();
					}

					bool Int64(int64_t v) {
						if (stack.top()) {
							stack.top()->Int64(v);
						}
						return checkTopParser();
					}

					bool Uint64(uint64_t v) {
						if (stack.top()) {
							stack.top()->Uint64(v);
						}
						return checkTopParser();
					}

					bool Double(double v) {
						if (stack.top()) {
							stack.top()->Double(v);
						}
						return checkTopParser();
					}

					std::stack<Parser*> stack;

					bool checkTopParser() {
						if (stack.top() && stack.top()->error.size()) {
							error = stack.top()->error;
							return false; // Copy the error to return it
						}
						else {
							return true;
						}
					}

					std::string error;
				};

				struct JsonParser : public ObjectParser
				{
					static void ParseJson(const char* buf, ObjectParser* parser)
					{
						rapidjson::StringStream ss(buf);
						MyHandler jsonHandler;
						jsonHandler.stack.push(parser);
						rapidjson::Reader reader;
						auto result = reader.Parse(ss, jsonHandler);
						if(result != rapidjson::kParseErrorNone) {
							throw JsonParsingException(result, jsonHandler);
						}
					}

					/*
					* Thrown when encountering a token that the current parser cannot handle.
					* The way the SAX parser is designed, this covers both specification
					* violations (e.g. invalid characters), and also schema violations (e.g.
					* expecting a number but finding a string).
					* 
					* As the JsonParser is used in a number of locations, this is not a
					* RepoException as we cannot say here if this is a config file, or a BIM
					* file, etc - the caller should catch this and re-throw with the appropriate
					* error code where necessary.
					*/
					struct REPO_API_EXPORT JsonParsingException : std::runtime_error
					{
						JsonParsingException(const rapidjson::ParseResult& result, const MyHandler& handlerError)
							:std::runtime_error(
								"JsonParser error: " +
								(handlerError.error.size() ? handlerError.error : rapidjson::GetParseError_En(result.Code())) +
								" at location " +
								std::to_string(result.Offset())
							)
						{
						}
					};
				};
			}
		}
	}
}