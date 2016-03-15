/*
 * JOCL - Java bindings for OpenCL
 *
 * Copyright (c) 2009-2016 Marco Hutter - http://www.jocl.org
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <jni.h>
#include <map>

#include "CLJNIUtils.hpp"
#include "Logger.hpp"
#include "PointerUtils.hpp"
#include "JNIUtils.hpp"

// The JVM, used for attaching the calling thread in
// callback functions
JavaVM *globalJvm;

// Field IDs for the cl_image_format class
jfieldID cl_image_format_image_channel_order; // cl_channel_order (cl_uint)
jfieldID cl_image_format_image_channel_data_type; // cl_channel_type (cl_uint)

// Field IDs for the cl_buffer_region class
jfieldID cl_buffer_region_origin; // size_t
jfieldID cl_buffer_region_size; // size_t

// Field IDs for the cl_image_desc class
jfieldID cl_image_desc_image_type; // cl_mem_object_type (cl_uint)
jfieldID cl_image_desc_image_width; // size_t
jfieldID cl_image_desc_image_height; // size_t
jfieldID cl_image_desc_image_depth; // size_t
jfieldID cl_image_desc_image_array_size; // size_t
jfieldID cl_image_desc_image_row_pitch; // size_t
jfieldID cl_image_desc_image_slice_pitch; // size_t
jfieldID cl_image_desc_num_mip_levels; // cl_uint
jfieldID cl_image_desc_num_samples; // cl_uint
jfieldID cl_image_desc_buffer; // cl_mem

// Class and method ID for cl_platform_id and its constructor
jclass cl_platform_id_Class;
jmethodID cl_platform_id_Constructor;

// Class and method ID for cl_device_id and its constructor
jclass cl_device_id_Class;
jmethodID cl_device_id_Constructor;

// Class and method ID for cl_context and its constructor
jclass cl_context_Class;
jmethodID cl_context_Constructor;

// Class and method ID for cl_command_queue and its constructor
jclass cl_command_queue_Class;
jmethodID cl_command_queue_Constructor;

// Class and method ID for cl_mem and its constructor
jclass cl_mem_Class;
jmethodID cl_mem_Constructor;

// Class and method ID for cl_image_format and its constructor
jclass cl_image_format_Class;
jmethodID cl_image_format_Constructor;

// Class and method ID for cl_sampler and its constructor
jclass cl_sampler_Class;
jmethodID cl_sampler_Constructor;

// Class and method ID for cl_program and its constructor
jclass cl_program_Class;
jmethodID cl_program_Constructor;

// Class and method ID for cl_kernel and its constructor
jclass cl_kernel_Class;
jmethodID cl_kernel_Constructor;

// Class and method ID for cl_event and its constructor
jclass cl_event_Class;
jmethodID cl_event_Constructor;

/**
 * The CallbackInfo structures of all contexts that have
 * been created so far and not released yet
 */
std::map<cl_context, CallbackInfo*> contextCallbackMap;


/**
 * Initialize the field- and method IDs for the CLJNIUtils
 */
int initCLJNIUtils(JNIEnv *env)
{
    jclass cls = NULL;

    // Obtain the fieldIDs of the cl_image_format class
    if (!init(env, cls, "org/jocl/cl_image_format")) return JNI_ERR;
    if (!init(env, cls, cl_image_format_image_channel_order,     "image_channel_order",     "I")) return JNI_ERR;
    if (!init(env, cls, cl_image_format_image_channel_data_type, "image_channel_data_type", "I")) return JNI_ERR;

    // Obtain the fieldIDs of the cl_buffer_region class
    if (!init(env, cls, "org/jocl/cl_buffer_region")) return JNI_ERR;
    if (!init(env, cls, cl_buffer_region_origin, "origin", "J")) return JNI_ERR;
    if (!init(env, cls, cl_buffer_region_size,   "size",   "J")) return JNI_ERR;

    // Obtain the fieldIDs of the cl_image_desc class
    if (!init(env, cls, "org/jocl/cl_image_desc")) return JNI_ERR;
    if (!init(env, cls, cl_image_desc_image_type,        "image_type",        "I")) return JNI_ERR;
    if (!init(env, cls, cl_image_desc_image_width,       "image_width",       "J")) return JNI_ERR;
    if (!init(env, cls, cl_image_desc_image_height,      "image_height",      "J")) return JNI_ERR;
    if (!init(env, cls, cl_image_desc_image_depth,       "image_depth",       "J")) return JNI_ERR;
    if (!init(env, cls, cl_image_desc_image_array_size,  "image_array_size",  "J")) return JNI_ERR;
    if (!init(env, cls, cl_image_desc_image_row_pitch,   "image_row_pitch",   "J")) return JNI_ERR;
    if (!init(env, cls, cl_image_desc_image_slice_pitch, "image_slice_pitch", "J")) return JNI_ERR;
    if (!init(env, cls, cl_image_desc_num_mip_levels,    "num_mip_levels",    "I")) return JNI_ERR;
    if (!init(env, cls, cl_image_desc_num_samples,       "num_samples",       "I")) return JNI_ERR;
    if (!init(env, cls, cl_image_desc_buffer,            "buffer",            "Lorg/jocl/cl_mem;")) return JNI_ERR;

    // Obtain the global class references and the constructor methodIDs
    // for classes which will have to be instantiated
    if (!init(env, "org/jocl/cl_platform_id",    cl_platform_id_Class,   cl_platform_id_Constructor  )) return JNI_ERR;
    if (!init(env, "org/jocl/cl_device_id",      cl_device_id_Class,     cl_device_id_Constructor    )) return JNI_ERR;
    if (!init(env, "org/jocl/cl_context",        cl_context_Class,       cl_context_Constructor      )) return JNI_ERR;
    if (!init(env, "org/jocl/cl_command_queue",  cl_command_queue_Class, cl_command_queue_Constructor)) return JNI_ERR;
    if (!init(env, "org/jocl/cl_mem",            cl_mem_Class,           cl_mem_Constructor          )) return JNI_ERR;
    if (!init(env, "org/jocl/cl_image_format",   cl_image_format_Class,  cl_image_format_Constructor )) return JNI_ERR;
    if (!init(env, "org/jocl/cl_sampler",        cl_sampler_Class,       cl_sampler_Constructor      )) return JNI_ERR;
    if (!init(env, "org/jocl/cl_program",        cl_program_Class,       cl_program_Constructor      )) return JNI_ERR;
    if (!init(env, "org/jocl/cl_kernel",         cl_kernel_Class,        cl_kernel_Constructor       )) return JNI_ERR;
    if (!init(env, "org/jocl/cl_event",          cl_event_Class,         cl_event_Constructor        )) return JNI_ERR;

    return JNI_VERSION_1_4;
}



/**
 * Create the cl_context_properties array for the given properties
 * object. The given 'properties' object is a org.jocl.Pointer to
 * a long array (wrapped in a Buffer), which contains pairs of
 * identifiers and values, and is terminated with a 0. This array
 * is converted element-wise to a cl_context_properties array here.
 * The returned array must be freed by the caller! If an error
 * occurs, NULL is returned.
 */
cl_context_properties* createContextPropertiesArray(JNIEnv *env, jobject properties)
{
    if (properties == NULL)
    {
        return NULL;
    }
    PointerData *propertiesPointerData = initPointerData(env, properties);
    if (propertiesPointerData == NULL)
    {
        return NULL;
    }
    size_t javaPropertiesSize = 0;
    jlong *javaPropertyValues = (jlong*)propertiesPointerData->pointer;
    size_t MAX_PROPERTIES = 100;
    for (size_t i = 0; i<MAX_PROPERTIES; i++)
    {
        if (javaPropertyValues[i] == 0)
        {
            break;
        }
        javaPropertiesSize++;
    }
    cl_context_properties *nativeProperties = new cl_context_properties[javaPropertiesSize + 1];
    if (nativeProperties == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory during property array creation");
        return NULL;
    }

    for (size_t i=0; i<javaPropertiesSize; i++)
    {
        nativeProperties[i] = (cl_context_properties)javaPropertyValues[i];
    }
    nativeProperties[javaPropertiesSize] = 0;
    if (!releasePointerData(env, propertiesPointerData, JNI_ABORT)) return NULL;
    return nativeProperties;
}


/**
 * Create the cl_queue_properties array for the given properties
 * object. The given 'properties' object is a org.jocl.Pointer to
 * a long array (wrapped in a Buffer), which contains pairs of
 * identifiers and values, and is terminated with a 0. This array
 * is converted element-wise to a cl_queue_properties array here.
 * The returned array must be freed by the caller! If an error
 * occurs, NULL is returned.
 */
cl_queue_properties* createQueuePropertiesArray(JNIEnv *env, jobject properties)
{
    if (properties == NULL)
    {
        return NULL;
    }
    PointerData *propertiesPointerData = initPointerData(env, properties);
    if (propertiesPointerData == NULL)
    {
        return NULL;
    }
    size_t javaPropertiesSize = 0;
    jlong *javaPropertyValues = (jlong*)propertiesPointerData->pointer;
    size_t MAX_PROPERTIES = 100;
    for (size_t i = 0; i<MAX_PROPERTIES; i++)
    {
        if (javaPropertyValues[i] == 0)
        {
            break;
        }
        javaPropertiesSize++;
    }
    cl_queue_properties *nativeProperties = new cl_queue_properties[javaPropertiesSize + 1];
    if (nativeProperties == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory during property array creation");
        return NULL;
    }

    for (size_t i = 0; i<javaPropertiesSize; i++)
    {
        nativeProperties[i] = (cl_queue_properties)javaPropertyValues[i];
    }
    nativeProperties[javaPropertiesSize] = 0;
    if (!releasePointerData(env, propertiesPointerData, JNI_ABORT)) return NULL;
    return nativeProperties;
}

/**
 * Create the cl_pipe_properties array for the given properties
 * object. The given 'properties' object is a org.jocl.Pointer to
 * a long array (wrapped in a Buffer), which contains pairs of
 * identifiers and values, and is terminated with a 0. This array
 * is converted element-wise to a cl_pipe_properties array here.
 * The returned array must be freed by the caller! If an error
 * occurs, NULL is returned.
 */
cl_pipe_properties* createPipePropertiesArray(JNIEnv *env, jobject properties)
{
    if (properties == NULL)
    {
        return NULL;
    }
    PointerData *propertiesPointerData = initPointerData(env, properties);
    if (propertiesPointerData == NULL)
    {
        return NULL;
    }
    size_t javaPropertiesSize = 0;
    jlong *javaPropertyValues = (jlong*)propertiesPointerData->pointer;
    size_t MAX_PROPERTIES = 100;
    for (size_t i = 0; i<MAX_PROPERTIES; i++)
    {
        if (javaPropertyValues[i] == 0)
        {
            break;
        }
        javaPropertiesSize++;
    }
    cl_pipe_properties *nativeProperties = new cl_pipe_properties[javaPropertiesSize + 1];
    if (nativeProperties == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory during property array creation");
        return NULL;
    }

    for (size_t i = 0; i<javaPropertiesSize; i++)
    {
        nativeProperties[i] = (cl_pipe_properties)javaPropertyValues[i];
    }
    nativeProperties[javaPropertiesSize] = 0;
    if (!releasePointerData(env, propertiesPointerData, JNI_ABORT)) return NULL;
    return nativeProperties;
}

/**
 * Create the cl_sampler_properties array for the given properties
 * object. The given 'properties' object is a org.jocl.Pointer to
 * a long array (wrapped in a Buffer), which contains pairs of
 * identifiers and values, and is terminated with a 0. This array
 * is converted element-wise to a cl_sampler_properties array here.
 * The returned array must be freed by the caller! If an error
 * occurs, NULL is returned.
 */
cl_sampler_properties* createSamplerPropertiesArray(JNIEnv *env, jobject properties)
{
    if (properties == NULL)
    {
        return NULL;
    }
    PointerData *propertiesPointerData = initPointerData(env, properties);
    if (propertiesPointerData == NULL)
    {
        return NULL;
    }
    size_t javaPropertiesSize = 0;
    jlong *javaPropertyValues = (jlong*)propertiesPointerData->pointer;
    size_t MAX_PROPERTIES = 100;
    for (size_t i = 0; i<MAX_PROPERTIES; i++)
    {
        if (javaPropertyValues[i] == 0)
        {
            break;
        }
        javaPropertiesSize++;
    }
    cl_sampler_properties *nativeProperties = new cl_sampler_properties[javaPropertiesSize + 1];
    if (nativeProperties == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory during property array creation");
        return NULL;
    }

    for (size_t i = 0; i<javaPropertiesSize; i++)
    {
        nativeProperties[i] = (cl_sampler_properties)javaPropertyValues[i];
    }
    nativeProperties[javaPropertiesSize] = 0;
    if (!releasePointerData(env, propertiesPointerData, JNI_ABORT)) return NULL;
    return nativeProperties;
}


/**
 * Fills the native representation of the given Java object.
 */
void getCl_image_format(JNIEnv *env, jobject image_format, cl_image_format &nativeImage_format)
{
    nativeImage_format.image_channel_order = 0;
    nativeImage_format.image_channel_data_type = 0;
    if (image_format != NULL)
    {
        nativeImage_format.image_channel_order     = (cl_channel_order)env->GetIntField(image_format, cl_image_format_image_channel_order);
        nativeImage_format.image_channel_data_type = (cl_channel_type) env->GetIntField(image_format, cl_image_format_image_channel_data_type);
    }
}

/**
 * Assign the values of the given native object to the given
 * Java object.
 */
void setCl_image_format(JNIEnv *env, jobject image_format, cl_image_format &nativeImage_format)
{
    if (image_format != NULL)
    {
        env->SetIntField(image_format, cl_image_format_image_channel_order,     (jint)nativeImage_format.image_channel_order);
        env->SetIntField(image_format, cl_image_format_image_channel_data_type, (jint)nativeImage_format.image_channel_data_type);
    }
}


/**
 * Fills the native representation of the given Java object.
 */
void getCl_image_desc(JNIEnv *env, jobject image_desc, cl_image_desc &nativeImage_desc)
{
    nativeImage_desc.image_type = CL_MEM_OBJECT_BUFFER;
    nativeImage_desc.image_width = 0;
    nativeImage_desc.image_height = 0;
    nativeImage_desc.image_depth = 0;
    nativeImage_desc.image_array_size = 0;
    nativeImage_desc.image_row_pitch = 0;
    nativeImage_desc.image_slice_pitch = 0;
    nativeImage_desc.num_mip_levels = 0;
    nativeImage_desc.num_samples = 0;
    nativeImage_desc.buffer = NULL;
    if (image_desc != NULL)
    {
        nativeImage_desc.image_type        = (cl_mem_object_type)env->GetIntField( image_desc, cl_image_desc_image_type);
        nativeImage_desc.image_width       = (size_t) env->GetLongField(image_desc, cl_image_desc_image_width);
        nativeImage_desc.image_height      = (size_t) env->GetLongField(image_desc, cl_image_desc_image_height);
        nativeImage_desc.image_depth       = (size_t) env->GetLongField(image_desc, cl_image_desc_image_depth);
        nativeImage_desc.image_array_size  = (size_t) env->GetLongField(image_desc, cl_image_desc_image_array_size);
        nativeImage_desc.image_row_pitch   = (size_t) env->GetLongField(image_desc, cl_image_desc_image_row_pitch);
        nativeImage_desc.image_slice_pitch = (size_t) env->GetLongField(image_desc, cl_image_desc_image_slice_pitch);
        nativeImage_desc.num_mip_levels    = (cl_uint)env->GetIntField( image_desc, cl_image_desc_num_mip_levels);
        nativeImage_desc.num_samples       = (cl_uint)env->GetIntField( image_desc, cl_image_desc_num_samples);
        jobject buffer = env->GetObjectField(image_desc, cl_image_desc_buffer);
        if (buffer != NULL)
        {
            nativeImage_desc.buffer = (cl_mem)env->GetLongField(buffer, NativePointerObject_nativePointer);
        }
    }
}

/**
 * Fills the native representation of the given Java object.
 */
void getCl_buffer_region(JNIEnv *env, jobject buffer_region, cl_buffer_region &nativeBuffer_region)
{
    nativeBuffer_region.origin = 0;
    nativeBuffer_region.size = 0;
    if (buffer_region != NULL)
    {
        nativeBuffer_region.origin = (size_t)env->GetLongField(buffer_region, cl_buffer_region_origin);
        nativeBuffer_region.size   = (size_t)env->GetLongField(buffer_region, cl_buffer_region_size);
    }
}


/**
 * Create the cl_device_partition_property for the given properties
 * object. The given 'properties' object is a cl_device_partition_property
 * object, which is a pointer to a long array (wrapped in a Buffer), which
 * contains pairs of identifiers and values, and is terminated with a 0.
 * This array is converted element-wise to a cl_device_partition_property
 * here. The returned array must be freed by the caller! If an error
 * occurs, NULL is returned.
 */
cl_device_partition_property* getCl_device_partition_property (JNIEnv *env, jobject properties)
{
    if (properties == NULL)
    {
        return NULL;
    }
    PointerData *propertiesPointerData = initPointerData(env, properties);
    if (propertiesPointerData == NULL)
    {
        return NULL;
    }
    size_t javaPropertiesSize = 0;
    jlong *javaPropertyValues = (jlong*)propertiesPointerData->pointer;
    size_t MAX_PROPERTIES = 100;
    for (size_t i = 0; i<MAX_PROPERTIES; i++)
    {
        if (javaPropertyValues[i] == 0)
        {
            break;
        }
        javaPropertiesSize++;
    }
    cl_device_partition_property *nativeProperties = new cl_device_partition_property[javaPropertiesSize + 1];
    if (nativeProperties == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory during property array creation");
        return NULL;
    }

    for (size_t i = 0; i<javaPropertiesSize; i++)
    {
        nativeProperties[i] = (cl_device_partition_property)javaPropertyValues[i];
    }
    nativeProperties[javaPropertiesSize] = 0;
    if (!releasePointerData(env, propertiesPointerData, JNI_ABORT)) return NULL;
    return nativeProperties;
}


/**
 * Creates a list containing the native cl_event objects for the
 * java cl_event objects in the given java array. To delete the
 * returned array is left to the caller. The returned array will
 * have num_events entries. If one of the java objects is 'null',
 * then a NullPointerException will be thrown and the function
 * returns NULL. If the java array has less than num_events
 * elements, then and ArrayIndexOutOfBoundsException will be
 * thrown and the function returns NULL.
 * Returns NULL if an error occurs.
 */
cl_event* createEventList(JNIEnv *env, jobjectArray event_list, cl_uint num_events)
{
    cl_event* nativeEvent_list = new cl_event[num_events];
    if (nativeEvent_list == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory during event array creation");
        return NULL;
    }

    cl_uint event_listLength = (cl_uint)env->GetArrayLength(event_list);
    if (event_listLength < num_events)
    {
        ThrowByName(env, "java/lang/ArrayIndexOutOfBoundsException",
            "Event list size is smaller than specified number of events");
        return NULL;
    }
    for (size_t i=0; i<num_events; i++)
    {
        jobject ev = env->GetObjectArrayElement(event_list, (jsize)i);
        if (env->ExceptionCheck())
        {
            delete[] nativeEvent_list;
            return NULL;
        }
        if (ev == NULL)
        {
            delete[] nativeEvent_list;
            ThrowByName(env, "java/lang/NullPointerException",
                "Event list contains 'null' elements");
            return NULL;
        }
        nativeEvent_list[i] = (cl_event)env->GetLongField(ev, NativePointerObject_nativePointer);
    }
    return nativeEvent_list;
}

/**
 * Creates a list containing the native cl_device_id objects for the
 * java cl_device_id objects in the given java array. To delete the
 * returned array is left to the caller. The returned array will
 * have num_devices entries. If one of the java objects is 'null',
 * then a NullPointerException will be thrown and the function
 * returns NULL. If the java array has less than num_devices
 * elements, then and ArrayIndexOutOfBoundsException will be
 * thrown and the function returns NULL.
 * Returns NULL if an error occurs.
 */
cl_device_id* createDeviceList(JNIEnv *env, jobjectArray device_list, cl_uint num_devices)
{
    cl_device_id *nativeDevice_list = new cl_device_id[num_devices];
    if (nativeDevice_list == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory during device array creation");
        return NULL;
    }

    cl_uint device_listLength = (cl_uint)env->GetArrayLength(device_list);
    if (device_listLength < num_devices)
    {
        ThrowByName(env, "java/lang/ArrayIndexOutOfBoundsException",
            "Device list size is smaller than specified number of devices");
        return NULL;
    }
    for (size_t i = 0; i<num_devices; i++)
    {
        jobject device = env->GetObjectArrayElement(device_list, (jsize)i);
        if (env->ExceptionCheck())
        {
            delete[] nativeDevice_list;
            return NULL;
        }
        if (device == NULL)
        {
            delete[] nativeDevice_list;
            ThrowByName(env, "java/lang/NullPointerException",
                "Device list contains 'null' elements");
            return NULL;
        }
        nativeDevice_list[i] = (cl_device_id)env->GetLongField(device, NativePointerObject_nativePointer);
    }
    return nativeDevice_list;
}


/**
 * Creates a list containing the native cl_mem objects for the
 * java cl_mem objects in the given java array. To delete the
 * returned array is left to the caller. The returned array will
 * have num_mems entries. If one of the java objects is 'null',
 * then a NullPointerException will be thrown and the function
 * returns NULL. If the java array has less than num_mems
 * elements, then and ArrayIndexOutOfBoundsException will be
 * thrown and the function returns NULL.
 * Returns NULL if an error occurs.
 */
cl_mem* createMemList(JNIEnv *env, jobjectArray mem_list, cl_uint num_mems)
{
    cl_mem *nativeMem_list = new cl_mem[num_mems];
    if (nativeMem_list == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory during mem array creation");
        return NULL;
    }

    cl_uint mem_listLength = (cl_uint)env->GetArrayLength(mem_list);
    if (mem_listLength < num_mems)
    {
        ThrowByName(env, "java/lang/ArrayIndexOutOfBoundsException",
            "Memory object list size is smaller than specified number of memory objects");
        return NULL;
    }
    for (size_t i = 0; i<num_mems; i++)
    {
        jobject mem = env->GetObjectArrayElement(mem_list, (jsize)i);
        if (env->ExceptionCheck())
        {
            delete[] nativeMem_list;
            return NULL;
        }
        if (mem == NULL)
        {
            delete[] nativeMem_list;
            ThrowByName(env, "java/lang/NullPointerException",
                "Memory object list contains 'null' elements");
            return NULL;
        }
        nativeMem_list[i] = (cl_mem)env->GetLongField(mem, NativePointerObject_nativePointer);
    }
    return nativeMem_list;
}


/**
 * Creates a list containing the native void* objects for the
 * java Pointer objects in the given java array. To delete the
 * returned array is left to the caller. The returned array will
 * have num_svm_pointers entries. If one of the java objects is 'null',
 * then a NullPointerException will be thrown and the function
 * returns NULL. If the java array has less than num_svm_pointers
 * elements, then and ArrayIndexOutOfBoundsException will be
 * thrown and the function returns NULL.
 * Returns NULL if an error occurs.
 */
void** createSvmPointers(JNIEnv *env, jobjectArray svm_pointers, cl_uint num_svm_pointers)
{
    void** nativeSvm_pointers = new void*[num_svm_pointers];
    if (nativeSvm_pointers == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory during pointer array creation");
        return NULL;
    }

    cl_uint svm_pointersLength = (cl_uint)env->GetArrayLength(svm_pointers);
    if (svm_pointersLength < num_svm_pointers)
    {
        ThrowByName(env, "java/lang/ArrayIndexOutOfBoundsException",
            "Pointer array size is smaller than specified number of pointers");
        return NULL;
    }
    for (size_t i = 0; i<num_svm_pointers; i++)
    {
        jobject svm_pointerObject = env->GetObjectArrayElement(svm_pointers, (jsize)i);
        if (env->ExceptionCheck())
        {
            delete[] nativeSvm_pointers;
            return NULL;
        }
        if (svm_pointerObject == NULL)
        {
            delete[] nativeSvm_pointers;
            ThrowByName(env, "java/lang/NullPointerException",
                "Pointer array contains 'null' elements");
            return NULL;
        }
        nativeSvm_pointers[i] = (void*)env->GetLongField(svm_pointerObject, NativePointerObject_nativePointer);
    }
    return nativeSvm_pointers;
}


/**
 * Creates a list containing the native cl_program objects for the
 * java cl_program objects in the given java array. To delete the
 * returned array is left to the caller. The returned array will
 * have num_programs entries. If one of the java objects is 'null',
 * then a NullPointerException will be thrown and the function
 * returns NULL. If the java array has less than num_programs
 * elements, then and ArrayIndexOutOfBoundsException will be
 * thrown and the function returns NULL.
 * Returns NULL if an error occurs.
 */
cl_program* createProgramList(JNIEnv *env, jobjectArray program_list, cl_uint num_programs)
{
    cl_program* nativeProgram_list = new cl_program[num_programs];
    if (nativeProgram_list == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory during program array creation");
        return NULL;
    }

    cl_uint program_listLength = (cl_uint)env->GetArrayLength(program_list);
    if (program_listLength < num_programs)
    {
        ThrowByName(env, "java/lang/ArrayIndexOutOfBoundsException",
            "Program list size is smaller than specified number of programs");
        return NULL;
    }
    for (size_t i = 0; i<num_programs; i++)
    {
        jobject ev = env->GetObjectArrayElement(program_list, (jsize)i);
        if (env->ExceptionCheck())
        {
            delete[] nativeProgram_list;
            return NULL;
        }
        if (ev == NULL)
        {
            delete[] nativeProgram_list;
            ThrowByName(env, "java/lang/NullPointerException",
                "Program list contains 'null' elements");
            return NULL;
        }
        nativeProgram_list[i] = (cl_program)env->GetLongField(ev, NativePointerObject_nativePointer);
    }
    return nativeProgram_list;
}



/**
 * Initializes and returns a CallbackInfo structure with the given
 * arguments. pfn_notify is the Java callback interface object
 * (may be NULL), and user_data is a java.lang.Object (may be NULL).
 * The returned CallbackInfo structure will contain global references
 * to the non-null arguments. Returns NULL if an arror occurs.
 */
CallbackInfo* initCallbackInfo(JNIEnv *env, jobject pfn_notify, jobject user_data)
{
    Logger::log(LOG_DEBUGTRACE, "Executing initCallbackInfo\n");

    CallbackInfo *callbackInfo = new CallbackInfo();
    if (callbackInfo == NULL)
    {
        ThrowByName(env, "java/lang/OutOfMemoryError",
            "Out of memory when preparing callback function");
        return NULL;
    }
    callbackInfo->globalPfn_notify = NULL;
    callbackInfo->globalUser_data = NULL;
    if (pfn_notify != NULL)
    {
        callbackInfo->globalPfn_notify = env->NewGlobalRef(pfn_notify);
        if (env->ExceptionCheck())
        {
            delete callbackInfo;
            return NULL;
        }
    }
    if (user_data != NULL)
    {
        callbackInfo->globalUser_data = env->NewGlobalRef(user_data);
        if (env->ExceptionCheck())
        {
            delete callbackInfo;
            return NULL;
        }
    }
    return callbackInfo;
}


/**
 * Deletes the global references stored in the given CallbackInfo
 * object and deletes the object.
 */
void deleteCallbackInfo(JNIEnv *env, CallbackInfo* &callbackInfo)
{
    Logger::log(LOG_DEBUGTRACE, "Executing deleteCallbackInfo\n");

    if (callbackInfo == NULL)
    {
        return;
    }
    if (callbackInfo->globalUser_data != NULL)
    {
        env->DeleteGlobalRef(callbackInfo->globalUser_data);
    }
    if (callbackInfo->globalPfn_notify != NULL)
    {
        env->DeleteGlobalRef(callbackInfo->globalPfn_notify);
    }
    delete callbackInfo;
    callbackInfo = NULL;
}


/**
 * Deletes the CallbackInfo that is associated with the
 * given cl_context and removes this mapping from the
 * contextCallbackMap
 */
void destroyCallbackInfo(JNIEnv *env, cl_context context)
{
    std::map<cl_context, CallbackInfo*>::iterator iter =
        contextCallbackMap.find(context);
    if (iter != contextCallbackMap.end())
    {
        contextCallbackMap.erase(iter);
        deleteCallbackInfo(env, iter->second);
    }
}



/**
 * TODO: Check how exceptions from callback functions may be handled
 * properly.
 *
 * This functions checks if an exception occurred, rethrows it
 * as a RuntimeException if necessary
 */
void finishCallback(JNIEnv *env)
{
    if (env->ExceptionCheck())
    {
        env->ExceptionClear();
        jclass newExceptionClass = env->FindClass("java/lang/RuntimeException");
        if (newExceptionClass == NULL)
        {
            globalJvm->DetachCurrentThread();
            return;
        }
        globalJvm->DetachCurrentThread();
        env->ThrowNew(newExceptionClass, "From CL callback");
        return;
    }
}

