/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2020, Linus Groh <mail@linusgroh.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <AK/Function.h>
#include <AK/StringBuilder.h>
#include <LibJS/Heap/Heap.h>
#include <LibJS/Interpreter.h>
#include <LibJS/Runtime/Array.h>
#include <LibJS/Runtime/ArrayPrototype.h>
#include <LibJS/Runtime/Error.h>
#include <LibJS/Runtime/Function.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/MarkedValueList.h>
#include <LibJS/Runtime/Value.h>

namespace JS {

ArrayPrototype::ArrayPrototype()
    : Object(interpreter().global_object().object_prototype())
{
    put_native_function("filter", filter, 1);
    put_native_function("forEach", for_each, 1);
    put_native_function("map", map, 1);
    put_native_function("pop", pop, 0);
    put_native_function("push", push, 1);
    put_native_function("shift", shift, 0);
    put_native_function("toString", to_string, 0);
    put_native_function("unshift", unshift, 1);
    put_native_function("join", join, 1);
    put_native_function("concat", concat, 1);
    put_native_function("slice", slice, 2);
    put_native_function("indexOf", index_of, 1);
    put("length", Value(0));
}

ArrayPrototype::~ArrayPrototype()
{
}

static Array* array_from(Interpreter& interpreter)
{
    auto* this_object = interpreter.this_value().to_object(interpreter.heap());
    if (!this_object)
        return {};
    if (!this_object->is_array()) {
        interpreter.throw_exception<TypeError>("Not an Array");
        return nullptr;
    }
    return static_cast<Array*>(this_object);
}

static Function* callback_from_args(Interpreter& interpreter, const String& name)
{
    if (interpreter.argument_count() < 1) {
        interpreter.throw_exception<TypeError>(String::format("Array.prototype.%s() requires at least one argument", name.characters()));
        return nullptr;
    }
    auto callback = interpreter.argument(0);
    if (!callback.is_object() || !callback.as_object().is_function()) {
        interpreter.throw_exception<TypeError>(String::format("%s is not a function", callback.to_string().characters()));
        return nullptr;
    }
    return static_cast<Function*>(&callback.as_object());
}

Value ArrayPrototype::filter(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};
    auto* callback = callback_from_args(interpreter, "filter");
    if (!callback)
        return {};
    auto this_value = interpreter.argument(1);
    auto initial_array_size = array->elements().size();
    auto* new_array = Array::create(interpreter.global_object());

    for (size_t i = 0; i < initial_array_size; ++i) {
        if (i >= array->elements().size())
            break;
        auto value = array->elements()[i];
        if (value.is_empty())
            continue;
        MarkedValueList arguments(interpreter.heap());
        arguments.append(value);
        arguments.append(Value((i32)i));
        arguments.append(array);
        auto result = interpreter.call(callback, this_value, move(arguments));
        if (interpreter.exception())
            return {};
        if (result.to_boolean())
            new_array->elements().append(value);
    }
    return Value(new_array);
}

Value ArrayPrototype::for_each(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};
    auto* callback = callback_from_args(interpreter, "forEach");
    if (!callback)
        return {};
    auto this_value = interpreter.argument(1);
    auto initial_array_size = array->elements().size();
    for (size_t i = 0; i < initial_array_size; ++i) {
        if (i >= array->elements().size())
            break;
        auto value = array->elements()[i];
        if (value.is_empty())
            continue;
        MarkedValueList arguments(interpreter.heap());
        arguments.append(value);
        arguments.append(Value((i32)i));
        arguments.append(array);
        interpreter.call(callback, this_value, move(arguments));
        if (interpreter.exception())
            return {};
    }
    return js_undefined();
}

Value ArrayPrototype::map(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};
    auto* callback = callback_from_args(interpreter, "map");
    if (!callback)
        return {};
    auto this_value = interpreter.argument(1);
    auto initial_array_size = array->elements().size();
    auto* new_array = Array::create(interpreter.global_object());
    for (size_t i = 0; i < initial_array_size; ++i) {
        if (i >= array->elements().size())
            break;
        auto value = array->elements()[i];
        if (value.is_empty())
            continue;
        MarkedValueList arguments(interpreter.heap());
        arguments.append(value);
        arguments.append(Value((i32)i));
        arguments.append(array);
        auto result = interpreter.call(callback, this_value, move(arguments));
        if (interpreter.exception())
            return {};
        new_array->elements().append(result);
    }
    return Value(new_array);
}

Value ArrayPrototype::push(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};
    for (size_t i = 0; i < interpreter.argument_count(); ++i)
        array->elements().append(interpreter.argument(i));
    return Value(array->length());
}

Value ArrayPrototype::unshift(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};
    for (size_t i = 0; i < interpreter.argument_count(); ++i)
        array->elements().insert(i, interpreter.argument(i));
    return Value(array->length());
}

Value ArrayPrototype::pop(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};
    if (array->elements().is_empty())
        return js_undefined();
    return array->elements().take_last();
}

Value ArrayPrototype::shift(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};
    if (array->elements().is_empty())
        return js_undefined();
    return array->elements().take_first();
}

static Value join_array_with_separator(Interpreter& interpreter, const Array& array, StringView separator)
{
    StringBuilder builder;
    for (size_t i = 0; i < array.elements().size(); ++i) {
        if (i != 0)
            builder.append(separator);
        if (!array.elements()[i].is_empty())
            builder.append(array.elements()[i].to_string());
    }
    return js_string(interpreter, builder.to_string());
}

Value ArrayPrototype::to_string(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};

    return join_array_with_separator(interpreter, *array, ",");
}

Value ArrayPrototype::join(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};

    String separator = ",";
    if (interpreter.argument_count() == 1)
        separator = interpreter.argument(0).to_string();

    return join_array_with_separator(interpreter, *array, separator);
}

Value ArrayPrototype::concat(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};

    auto* new_array = Array::create(interpreter.global_object());
    new_array->elements().append(array->elements());

    for (size_t i = 0; i < interpreter.argument_count(); ++i) {
        auto argument = interpreter.argument(i);
        if (argument.is_array()) {
            auto& argument_object = argument.as_object();
            new_array->elements().append(argument_object.elements());
        } else {
            new_array->elements().append(argument);
        }
    }

    return Value(new_array);
}

Value ArrayPrototype::slice(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};

    auto* new_array = Array::create(interpreter.global_object());
    if (interpreter.argument_count() == 0) {
        new_array->elements().append(array->elements());
        return new_array;
    }

    ssize_t array_size = static_cast<ssize_t>(array->elements().size());
    auto start_slice = interpreter.argument(0).to_i32();
    auto end_slice = array_size;

    if (start_slice > array_size)
        return new_array;

    if (start_slice < 0)
        start_slice = end_slice + start_slice;

    if (interpreter.argument_count() >= 2) {
        end_slice = interpreter.argument(1).to_i32();

        if (end_slice < 0)
            end_slice = array_size + end_slice;
        else if (end_slice > array_size)
            end_slice = array_size;
    }

    size_t array_capacity = start_slice + array_size - end_slice;
    new_array->elements().ensure_capacity(array_capacity);
    for (ssize_t i = start_slice; i < end_slice; ++i) {
        new_array->elements().append(array->elements().at(i));
    }

    return new_array;
}

Value ArrayPrototype::index_of(Interpreter& interpreter)
{
    auto* array = array_from(interpreter);
    if (!array)
        return {};

    i32 array_size = static_cast<i32>(array->elements().size());
    if (interpreter.argument_count() == 0 || array_size == 0)
        return Value(-1);

    i32 from_index = 0;
    if (interpreter.argument_count() >= 2) {
        from_index = interpreter.argument(1).to_number().to_i32();

        if (from_index >= array_size)
            return Value(-1);

        auto negative_min_index = ((array_size - 1) * -1);
        if (from_index < negative_min_index)
            from_index = 0;
        else if (from_index < 0)
            from_index = (array_size - 1) + from_index;
    }

    auto search_element = interpreter.argument(0);
    for (i32 i = from_index; i < array_size; ++i) {
        auto& element = array->elements().at(i);
        if (typed_eq(interpreter, element, search_element).as_bool())
            return Value(i);
    }

    return Value(-1);
}
}
