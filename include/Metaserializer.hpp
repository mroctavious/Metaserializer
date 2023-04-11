#pragma once

#include <type_traits>
#include <string>
#include <vector>
#include <iostream>
#include <set>
#include <cstdint>
#include <cstring>
#include <typeinfo>
#include <memory>
#include <vector>
#include <algorithm>
#include <cstddef>

/**
 * @brief This method will serialize a
 * a class object or a set of values with
 * different datatypes.
 */
namespace Metaserializer
{
    /**
     * @brief Data type where the size of the serial will be indicated. This will be used all times inside the serialization bytes string.
     * 
     */
    typedef short serial_size_t;

    /**
     * @brief Class to create a hash of all the data types given.
     * 
     */
    struct TypeHasher
    {
        /**
         * @brief Base case of implementation of the execution.
         * 
         * @tparam T Datatype to hash.
         * @param value Current hash value to xor.
         * @param obj Object to be hashed.
         * @return constexpr std::size_t new hash.
         */
        template <typename T>
        static constexpr std::size_t exec_impl(const size_t value, const T obj)
        {
            constexpr const std::type_info &id = typeid(T);
            return value ^ id.hash_code();
        }

        /**
         * @brief Recursive case of implementation of the execution.
         * 
         * @tparam T Datatype to hash.
         * @tparam ArgsT Remaining datatypes to hash.
         * @param value Current hash value to xor.
         * @param obj Object to be hashed.
         * @param args Rest of the types to hash.
         * @return constexpr std::size_t new hash.
         */
        template <typename T, typename... ArgsT>
        static constexpr std::size_t exec_impl(const size_t value, const T obj, const ArgsT... args)
        {
            constexpr const std::type_info &id = typeid(T);
            const auto hash_value = value ^ id.hash_code();
            return exec_impl(hash_value, args...);
        }

        /**
         * @brief Start case of implementation of the execution.
         * 
         * @tparam T Datatype to hash.
         * @tparam ArgsT Remaining datatypes to hash.
         * @param obj Object to be hashed.
         * @param args Rest of the types to hash.
         * @return constexpr std::size_t new hash.
         */
        template <typename T, typename... ArgsT>
        static constexpr std::size_t apply(const T obj, const ArgsT... args)
        {
            return exec_impl(0, obj, args...);
        }

        /**
         * @brief Compare the hashes.
         * 
         */
        struct EqualTo
        {
            using TypeInfoRef = std::reference_wrapper<const std::type_info>;
            bool operator()(TypeInfoRef lhs, TypeInfoRef rhs) const
            {
                return lhs.get() == rhs.get();
            }
        };

        /**
         * @brief Get the current hash id of the data type.
         * 
         * @tparam T Datatype to get hash id.
         * @return std::size_t constexpr 
         */
        template <typename T>
        std::size_t constexpr getID()
        {
            constexpr const std::type_info &id = typeid(T);
            return id.hash_code();
        };
    };

    /**
     * @brief This method will check if a class has the serialize method.
     * 
     * @tparam T Class to check if method serialize exists.
     */
    template <typename T>
    struct HasSerializeMethod
    {
        template <typename U, std::string (U::*)()> struct SFINAE{};
        template <typename U>
        static char SubstitutionTry(SFINAE<U, &U::serialize> *);
        template <typename U>
        static int SubstitutionTry(...);
        static const bool value = sizeof(SubstitutionTry<T>(0)) == sizeof(char);
    };

    /**
     * @brief This method will check if a class has the unserialize method.
     * 
     * @tparam T Class to check if method unserialize exists.
     */
    template <typename T>
    struct HasUnserializeMethod
    {
        template <typename U, size_t (U::*)(std::string&)> struct SFINAE{};
        template <typename U>
        static char SubstitutionTry(SFINAE<U, &U::unserialize> *);
        template <typename U>
        static int SubstitutionTry(...);
        static const bool value = sizeof(SubstitutionTry<T>(0)) == sizeof(char);
    };

    /**
     * @brief This metafunction will serialize a complex class.
     * 
     * @tparam T Class to serialize.
     * @tparam has_serialize boolean flag which indicates if the method serialize exists.
     */
    template <typename T, bool has_serialize>
    struct ComplexObject
    {
        /**
         * @brief This method will serialize a complex class which already has a serialize method.
         * 
         * @param obj 
         * @param buffer 
         * @return size_t 
         */
        static size_t serialize(T &obj, unsigned char *buffer)
        {
            auto serialized_obj = obj.serialize();
            int index = 0;
            for (auto it = serialized_obj.begin(); it != serialized_obj.end(); ++it)
            {
                buffer[index] = static_cast<unsigned char>(*it);
                ++index;
            }
            return index;
        }

        /**
         * @brief Unserialize complex object using the unserialize method in the complex class.
         * 
         * @param obj Object where the result will be stored.
         * @param buffer Buffer where the serialized data is.
         * @param size Size of the data inside the buffer.
         * @return size_t Bytes serialized.
         */
        static size_t unserialize(T &obj, unsigned char *buffer, size_t size){
            std::string serialized_string((char*) buffer, size);
            return obj.unserialize(serialized_string);
        }
    };

    /**
     * @brief This metafunction will not compile when no serialization method found.
     * 
     * @tparam T Class which has no specialization neither a serialize/unserialize method.
     */
    template <typename T>
    struct ComplexObject<T, false>
    {
        static int serialize(T &obj, unsigned char *buffer) = delete;
    };

    /**
     * @brief This metafunction will serialize a complex class.
     * 
     * @tparam std::string specialization. 
     */
    template <>
    struct ComplexObject<std::string, false>
    {
        /**
         * @brief This method will serialize a std::string class.
         * 
         * @param obj String to be serialized.
         * @param buffer Buffer where the data will be stored.
         * @return size_t bytes written in the buffer.
         */
        static inline size_t serialize(std::string &obj, unsigned char *buffer)
        {
            static const size_t serial_size = sizeof(serial_size_t);
            auto bytes2cpy = sizeof(typename std::string::value_type) * obj.size();
            serial_size_t byte_size_value = static_cast<serial_size_t>(bytes2cpy);
            std::memcpy(buffer, &byte_size_value, serial_size);
            std::memcpy(buffer + serial_size, obj.data(), bytes2cpy);
            return serial_size + bytes2cpy;
        }

        /**
         * @brief Method which will reconstruct the object from a serialization string.
         * 
         * @param result Reference to the object to store the result.
         * @param buffer Buffer where the serializations is stored.
         * @param buffer_size Remaining bytes inside the buffer.
         * @return size_t The size of the object serialized.
         */
        static inline size_t unserialize(std::string &result, unsigned char *buffer, size_t buffer_size){
            static const size_t serial_size = sizeof(serial_size_t);
            serial_size_t string_size;
            std::memcpy(&string_size, buffer, sizeof(serial_size_t));
            const size_t full_size = string_size+sizeof(serial_size_t);

            if(full_size>buffer_size){
                throw std::runtime_error("Error while trying to parse string, String size is bigger than the buffer, this will cause an overflow.");
            }
            std::string obj_reconstruction((char*)(buffer+sizeof(serial_size_t)), string_size);
            result=obj_reconstruction;
            return full_size;
        }
    };

    /**
     * @brief This class will serialize/unserialize simple object, a simple object must be trivially_copyable.
     * 
     * @tparam T Datatype to be serialized.
     */
    template <typename T>
    struct SimpleObject
    {
        /**
         * @brief Serialize object into a string of bytes.
         * 
         * @tparam _SrcT Source type.
         * @tparam _DestT Destination type.
         * @param src The object to be serialized.
         * @param dest Buffer where the bytes are going to be stored.
         * @return size_t Number of bytes written.
         */
        template <typename _SrcT, typename _DestT>
        static inline size_t serialize(_SrcT &src, _DestT dest)
        {
            std::memcpy(dest, &src, sizeof(T));
            return sizeof(T);
        }

        /**
         * @brief Unserialize string and reconstruct the object from raw bytes.
         * 
         * @param result Reference to the object where the data will be stored.
         * @param buffer Buffer where the serialized data is stored.
         * @return size_t Number of bytes read from byffer.
         */
        static inline size_t unserialize(T& result, unsigned char *buffer){
            std::memcpy(&result, buffer, sizeof(T));
            return sizeof(T);
        }
    };

    /**
     * @brief Class which will apply the serialization methods.
     * 
     * @tparam T Datatype to be serialized.
     * @tparam is_array Boolean which indicates if its an static array.
     * @tparam is_trivial Boolean which indicated if the class to be serialized.
     */
    template <typename T, bool is_array, bool is_trivial>
    struct TypeSerializerImpl
    {
        /**
         * @brief Default method to use when using the TypeSerializedImpl, if the datatype does not meet the requeriment, it will enter to this method and it wont compile for safety.
         * 
         * @return int Number of bytes written.
         */
        static int apply() = delete;
    };


    /**
     * @brief This metafunction will serialize a static array of the datatype given. Is Array and its a simple object.
     * 
     * @tparam T Datatype to be serialized, this class is for static array of T type.
     * @tparam N Number of elements in the array.
     */
    template <typename T, size_t N>
    struct TypeSerializerImpl<T[N], true, true>
    {
        /**
         * @brief Apply serialization to the datatype.
         * 
         * @param data Reference to the object to be serialized.
         * @param buffer Pointer to the serialize data buffer.
         * @return size_t Number of bytes written.
         */
        static size_t apply(T data[N], unsigned char *buffer)
        {
            static const serial_size_t jump = sizeof(serial_size_t);
            static const size_t full_array_size = sizeof(T) * N;
            static const serial_size_t size = static_cast<serial_size_t>(N);
            const void *data_ptr = &(data[0]);
            std::memcpy(buffer, &size, jump);
            std::memcpy(buffer + jump, data_ptr, full_array_size);
            return jump + full_array_size;
        }
    };

    /**
     * @brief This metafunction will serialize a static array of the datatype given. Is Array and its a complex object.
     * 
     * @tparam T Datatype to be serialized, this class is for static array of T type.
     * @tparam N Number of elements in the array.
     */
    template <typename T, size_t N>
    struct TypeSerializerImpl<T[N], true, false>
    {
        static const bool has_serialize = HasSerializeMethod<T>::value; //< Check if the class the 'serialize' method.

        /**
         * @brief Apply serialization to the datatype.
         * 
         * @param data Reference to the object to be serialized.
         * @param buffer Pointer to the serialize data buffer.
         * @return size_t Number of bytes written.
         */
        static size_t apply(T data[N], unsigned char *buffer)
        {
            static const serial_size_t jump = sizeof(serial_size_t);
            static const serial_size_t size = static_cast<serial_size_t>(N);
            std::memcpy(buffer, &size, jump);
            size_t bytes_written = jump;
            for (int i = 0; i < N; ++i)
            {
                unsigned char *buffer_iterator = bytes_written + buffer;
                bytes_written += ComplexObject<T, has_serialize>::serialize(data[i], buffer_iterator);
            }
            return bytes_written;
        }
    };

    /**
     * @brief This metafunction will serialize an object of the datatype given. It is not an array and its a simple object.
     * 
     * @tparam T Datatype to be serialized.
     */
    template <typename T>
    struct TypeSerializerImpl<T, false, true>
    {
        /**
         * @brief Apply serialization to the datatype.
         * 
         * @param data Reference to the object to be serialized.
         * @param buffer Pointer to the serialize data buffer.
         * @return size_t Number of bytes written.
         */
        static size_t apply(T &data, unsigned char *buffer)
        {
            return SimpleObject<T>::serialize(data, buffer);
        }
    };

    /**
     * @brief This metafunction will serialize an object of the datatype given. It is not an array and its a complex object.
     * 
     * @tparam T Datatype to be serialized.
     */
    template <typename T>
    struct TypeSerializerImpl<T, false, false>
    {
        static const bool has_serialize = HasSerializeMethod<T>::value; //< Check if the class the 'serialize' method.

        /**
         * @brief Apply serialization to the datatype.
         * 
         * @param data Reference to the object to be serialized.
         * @param buffer Pointer to the serialize data buffer.
         * @return size_t Number of bytes written.
         */
        static size_t apply(T &data, unsigned char *buffer)
        {
            return ComplexObject<T, has_serialize>::serialize(data, buffer);
        }
    };

    /**
     * @brief This metafunction will unserialize a set of bytes, reconstructing the object from raw data.
     * 
     * @tparam T Datatype to be unserialized.
     * @tparam is_array Boolean which indicates if its an static array.
     * @tparam is_trivial Boolean which indicated if the class to be serialized.
     */
    template <typename T, bool is_array, bool is_trivial>
    struct TypeUnserializerImpl
    {
        /**
         * @brief Default method to use when using the TypeUnserializerImpl, if the datatype does not meet the requeriment, it will enter to this method and it wont compile for safety.
         * 
         * @return int Number of bytes read from buffer.
         */
        static int apply() = delete;
    };

    /**
     * @brief This metafunction will unserialize a set of bytes, reconstructing the object from raw data. It is an static array and a simple object.
     * 
     * @tparam T Datatype to be unserialized.
     * @tparam N Number of elements in the array.
     */
    template <typename T, size_t N>
    struct TypeUnserializerImpl<T[N], true, true>
    {
        /**
         * @brief Apply unserialize process to the bytes inside the buffer and convert it to the datatype given.
         * 
         * @param result Reference to the object where the data will be stored.
         * @param buffer Pointer to the buffer which contains the raw bytes from the serialize process.
         * @param buffer_size Remaining bytes in the buffer.
         * @return size_t Number of bytes read from the buffer.
         */
        static size_t apply(T result[N], unsigned char *buffer, size_t buffer_size){
            if( sizeof(serial_size_t) > buffer_size ){
                std::runtime_error("Error while unserializing simple type array, buffer bytes remaining are too low to continue.");
            }
            
            serial_size_t size;
            std::memcpy( &size, buffer, sizeof(serial_size_t) );
            
            if( sizeof(T) * size > (buffer_size - sizeof(serial_size_t) ) ){
                std::runtime_error("Error while unserializing simple type array, can't read bytes indicated in byte size serialization.");
            }

            std::memcpy(&(result[0]), buffer+sizeof(serial_size_t), sizeof(T) * size );
            return sizeof(T) * size + sizeof(serial_size_t);
        }
    };

    /**
     * @brief This metafunction will unserialize a set of bytes, reconstructing the object from raw data. It is an static array and a complex object.
     * 
     * @tparam T Datatype to be unserialized.
     * @tparam N Number of elements in the array.
     */
    template <typename T, size_t N>
    struct TypeUnserializerImpl<T[N], true, false>
    {
        static const bool has_serialize = HasUnserializeMethod<T>::value; //< Check if the class to be unserialized has the unserialize method.

        /**
         * @brief Apply unserialize process to the bytes inside the buffer and convert it to the datatype given.
         * 
         * @param result Reference to the object where the data will be stored.
         * @param buffer Pointer to the buffer which contains the raw bytes from the serialize process.
         * @param buffer_size size_t Remaining bytes in the buffer.
         * @return size_t Number of bytes written.
         */
        static size_t apply(T result[N], unsigned char *buffer, size_t buffer_size){
            if( sizeof(serial_size_t) > buffer_size ){
                std::runtime_error("Error while unserializing complex type array, buffer bytes remaining are too low to continue.");
            }

            serial_size_t size;
            std::memcpy(&size, buffer, sizeof(serial_size_t));
            size_t bytes_read=sizeof(serial_size_t);
            size_t bytes_remaining=0;
            unsigned char *buffer_it=0;

            for(int i=0; i<size; i++){
                buffer_it = buffer + bytes_read;
                bytes_remaining = buffer_size - bytes_read;
                bytes_read += ComplexObject<T, has_serialize>::unserialize(result[i], buffer_it, bytes_remaining);
            }
            return bytes_read;
        }
    };

    /**
     * @brief This metafunction will unserialize a set of bytes, reconstructing the object from raw data, its a complex object.
     * 
     * @tparam T Datatype to be unserialized.
     */
    template <typename T>
    struct TypeUnserializerImpl<T, false, false>
    {
        static const bool has_serialize = HasUnserializeMethod<T>::value;

        static size_t apply(T& result, unsigned char *buffer, size_t buffer_size){
            return ComplexObject<T, has_serialize>::unserialize(result, buffer, buffer_size);
        }
    };

    /**
     * @brief This metafunction will unserialize a set of bytes, reconstructing the object from raw data, its a simple object.
     * 
     * @tparam T Datatype to be unserialized.
     */
    template <typename T>
    struct TypeUnserializerImpl<T, false, true>
    {
        /**
         * @brief 
         * 
         * @param result 
         * @param buffer 
         * @param buffer_size 
         * @return size_t 
         */
        static size_t apply(T& result, unsigned char *buffer, size_t buffer_size){
            if(sizeof(T) > buffer_size) throw std::runtime_error("Error while unserializing simple type, buffer bytes remaining are too low to continue.");
            return SimpleObject<T>::unserialize(result, buffer);
        }
    };

    /**
     * @brief 
     * 
     */
    struct TypeSerializer
    {
        /**
         * @brief This metafunction will define if given object is an array and if its trivially copyable, then it will call the corresponding metafunctions to apply serialization. 
         * 
         * @tparam T Datatype to serialize.
         * @param data Data reference to be serialized.
         * @param buffer Pointer to the buffer to store the result.
         * @return size_t Number of bytes written.
         */
        template <typename T>
        static inline size_t apply(T &data, unsigned char *buffer)
        {
            const bool is_trivial = std::is_trivially_copyable<T>::value;
            const bool is_array = std::is_array<T>::value;
            using unref_value_type = typename std::remove_reference<T>::type;
            return TypeSerializerImpl<unref_value_type, is_array, is_trivial>::apply(data, buffer);
        }
    };


    /**
     * @brief Class which apply unserialize raw data and reconstruct the object.
     * 
     */
    struct TypeUnserializer
    {
        /**
         * @brief This metafunction will define if given object is an array and if its trivially copyable, then it will call the corresponding metafunctions to apply unserialization. 
         * 
         * @tparam T Datatype to serialize.
         * @param data Data reference where the result will be stored.
         * @param buffer Pointer to the buffer where the raw bytes are stored.
         * @param bytes_remaining Number of bytes remaining in the buffer.
         * @return size_t Number of bytes read from the buffer.
         */
        template <typename T>
        static inline size_t apply(T &data, unsigned char *buffer, size_t bytes_remaining)
        {
            const bool is_trivial = std::is_trivially_copyable<T>::value;
            const bool is_array = std::is_array<T>::value;
            using unref_value_type = typename std::remove_reference<T>::type;
            return TypeUnserializerImpl<unref_value_type, is_array, is_trivial>::apply(data, buffer, bytes_remaining);
        }
    };


    /**
     * @brief Class which apply the serialize algorithm to the datatypes given.
     * 
     * @tparam BufferSize Max buffer size.
     */
    template <int BufferSize = 16384>
    struct Serialize
    {
        /**
         * @brief Base case of the serialization.
         * 
         * @tparam T Datatype to serialize.
         * @param buff_ptr Pointer to the buffer.
         * @param data Data to be serialized.
         * @return unsigned char* Pointer where the writing of byted ended.
         */
        template <typename T>
        static inline unsigned char *exec_impl(unsigned char **buff_ptr, T& data)
        {
            *buff_ptr = *buff_ptr + TypeSerializer::apply(data, *buff_ptr);
            return *buff_ptr;
        }

        /**
         * @brief Recursive case of the serialization algorithm.
         * 
         * @tparam T Datatype to serialize.
         * @tparam TArgs Rest of the datatypes to be serialized.
         * @param buff_ptr Pointer to the buffer.
         * @param data Data to be serialized.
         * @param Args Rest of the objects to be serialized.
         * @return unsigned char* Pointer where the writing of byted ended.
         */
        template <typename T, typename... TArgs>
        static inline unsigned char *exec_impl(unsigned char **buff_ptr, T& data, TArgs&... Args)
        {
            *buff_ptr = *buff_ptr + TypeSerializer::apply(data, *buff_ptr);
            return exec_impl(buff_ptr, Args...);
        }

        /**
         * @brief Save in the buffer the hash created from all the datatypes given.
         * 
         * @tparam T First datatype to be serialized.
         * @tparam TArgs Rest of the datatypes to be serilized.
         * @param buffer Pointer to the buffer to store the data.
         * @param data Object to be serialized.
         * @param Args Rest of the object to be serialized.
         * @return size_t Number of bytes written in the buffer.
         */
        template <typename T, typename... TArgs>
        static inline size_t set_hash(unsigned char *buffer, T& data, TArgs&... Args){
            size_t hash = Metaserializer::TypeHasher::apply(data, Args...);
            std::memcpy(buffer, &hash, sizeof(size_t));
            return sizeof(size_t);
        }

        /**
         * @brief This method will start the serialization algorithm, it will create a hash from all the datatypes given and will insert it at the beggining of the serial result. 
         * 
         * @tparam T First datatype to be serialized.
         * @tparam TArgs Rest of the datatypes to be serilized.
         * @param data Object where the bytes are stored.
         * @param args Rest of the object to be serialized.
         * @return std::string Serialized result in a std::string which contains a set of ordered bytes.
         */
        template <typename T, typename... TArgs>
        static inline std::string apply(T& data, TArgs&... args)
        {
            unsigned char buffer[BufferSize] = {0};
            unsigned char *buffer_it = buffer + set_hash(buffer, data, args...);
            unsigned char *buffer_end = exec_impl(&buffer_it, data, args...);  
            size_t bytes_written = buffer_end - buffer;
            return std::string(reinterpret_cast<char*>(buffer), bytes_written);
        }
    };

    /**
     * @brief Class which apply the serialize algorithm to the datatypes given.
     * 
     * @tparam BufferSize BufferSize Max buffer size.
     */
    template <int BufferSize = 16384>
    struct Unserialize
    {
        static const int hash_size = sizeof(size_t); //< Number of bytes which indicates the size of the hash.

        /**
         * @brief Get the hash from bytes object.
         * 
         * @tparam T Datatype of the object to get hash from raw bytes.
         * @param data Object which contains the raw bytes.
         * @return size_t Number of bytes read.
         */
        template<typename T>
        static inline size_t get_hash_from_bytes(T& data){
            size_t hash;
            if(data.size() < hash_size){
                throw std::runtime_error("Deserialize Error! Data size is too small to be parsed.");
            }
            std::memcpy( &hash, data.data(), hash_size );
            return hash;
        }

        /**
         * @brief Check if the hash included in the serialization data correspond to the hash created with all the datatypes given to unserialize.
         * 
         * @tparam T Datatype of the object which contains the raw bytes.
         * @tparam TArgs Rest of the datatypes to be unserilized.
         * @param raw_data Object which contains the raw bytes.
         * @param args All the objects to unserialize.
         * @return true Hash value correspond to the hash included in the serialized data.
         * @return false Hash values is different.
         */
        template<typename T, typename... TArgs>
        static inline bool check_type(T& raw_data,  TArgs... args){
            auto msg_hash = get_hash_from_bytes(raw_data);
            auto struct_hash = Metaserializer::TypeHasher::apply(args...);
            if ( msg_hash == struct_hash ){
                return true;
            }else{
                return false;
            }
        }

        /**
         * @brief Base case of the unserialization algorithm.
         * 
         * @tparam T First datatype to be serialized.
         * @param buff_ptr Pointer of the data buffer where the raw bytes are located.
         * @param bytes_in_buffer Number of bytes remaining in the buffer.
         * @param result_ref Referece to the object where the result will be stored.
         */
        template <typename T>
        static inline size_t exec_impl(unsigned char *buff_ptr, size_t bytes_in_buffer, T& result_ref)
        {
            auto bytes_read = Metaserializer::TypeUnserializer::apply(result_ref, buff_ptr, bytes_in_buffer);
            return bytes_read;
        }

        /**
         * @brief Recursive case of the unserialization algorithm.
         * 
         * @tparam T First datatype to be serialized.
         * @tparam TArgs Rest of the datatypes to be unserilized
         * @param buff_ptr Pointer of the data buffer where the raw bytes are located.
         * @param bytes_in_buffer Number of bytes remaining in the buffer.
         * @param result_ref Referece to the object where the result will be stored.
         * @param Args Rest of the object to be unserialized.
         */
        template <typename T, typename... TArgs>
        static inline size_t exec_impl(unsigned char *buff_ptr, size_t bytes_in_buffer, T& result_ref, TArgs&... Args)
        {
            auto bytes_read = Metaserializer::TypeUnserializer::apply(result_ref, buff_ptr, bytes_in_buffer);
            return bytes_read + exec_impl(buff_ptr+bytes_read, bytes_in_buffer-bytes_read, Args...);
        }

        /**
         * @brief This method will copy the bytes from the object to a raw unsigned char array.
         * 
         * @tparam T Datatype of the object which contains the raw bytes.
         * @param data Object which contains the raw bytes.
         * @param buffer Pointer to the buffer where the data will be copied.
         * @return size_t Number of bytes written.
         */
        template<typename T>
        static size_t copy_to_buffer(T& data, unsigned char* buffer){
            std::memset(buffer, 0, BufferSize);
            std::memcpy(buffer, (unsigned char*)data.data(), data.size());
            return data.size();

        }

        /**
         * @brief Start the unserialization algorithm.
         * 
         * @tparam T Datatype of the object which contains the raw bytes.
         * @tparam TArgs Rest of the datatypes to be unserilized
         * @param data Object which contains the raw bytes.
         * @param args All the objects to unserialize.
         */
        template <typename T, typename... TArgs>
        static inline size_t apply(T& data, TArgs&... args)
        {
            static unsigned char buffer[BufferSize] = {0};

            if(data.size() > BufferSize){
                throw std::runtime_error("Error while unserialize, Bytes are more than buffer capacity.");
            }

            size_t bytes_in_buffer=copy_to_buffer(data, buffer);
            
            if( ! check_type(data, args...) ){
                throw std::runtime_error("Types hash are different from the serial data hash.");
            }
            
            return hash_size + exec_impl(buffer+hash_size, bytes_in_buffer-hash_size, args...);
        }
    };

};
