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
namespace MetaSerializer
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
        template <typename U, size_t (U::*)() const> struct SFINAE{};
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
        template <typename U, size_t (U::*)() const> struct SFINAE{};
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
                buffer[index] = reinterpret_cast<unsigned char>(*it);
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
            T new_obj = T::unserialize(serialized_string);
            obj = new_obj;
            return new_obj.size();
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
         * @brief 
         * 
         * @param result 
         * @param buffer 
         * @param buffer_size 
         * @return size_t 
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
     * @brief 
     * 
     * @tparam T 
     */
    template <typename T>
    struct SimpleObject
    {
        /**
         * @brief 
         * 
         * @tparam _SrcT 
         * @tparam _DestT 
         * @param src 
         * @param dest 
         * @return size_t 
         */
        template <typename _SrcT, typename _DestT>
        static inline size_t serialize(_SrcT &src, _DestT dest)
        {
            std::memcpy(dest, &src, sizeof(T));
            return sizeof(T);
        }

        /**
         * @brief 
         * 
         * @param result 
         * @param data 
         * @return size_t 
         */
        static inline size_t unserialize(T& result, unsigned char *buffer){
            std::memcpy(&result, buffer, sizeof(T));
            return sizeof(T);
        }
    };

    /**
     * @brief 
     * 
     * @tparam T 
     * @tparam is_array 
     * @tparam is_trivial 
     */
    template <typename T, bool is_array, bool is_trivial>
    struct TypeSerializerImpl
    {
        static int apply() = delete;
    };

    /**
     * @brief 
     * 
     * @tparam T 
     * @tparam N 
     */
    template <typename T, size_t N>
    struct TypeSerializerImpl<T[N], true, true>
    {
        /**
         * @brief 
         * 
         * @param data 
         * @param buffer 
         * @return size_t 
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
     * @brief 
     * 
     * @tparam T 
     * @tparam N 
     */
    template <typename T, size_t N>
    struct TypeSerializerImpl<T[N], true, false>
    {
        static const bool has_serialize = HasSerializeMethod<T>::value;

        /**
         * @brief 
         * 
         * @param data 
         * @param buffer 
         * @return size_t 
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
     * @brief 
     * 
     * @tparam T 
     */
    template <typename T>
    struct TypeSerializerImpl<T, false, true>
    {
        /**
         * @brief 
         * 
         * @param data 
         * @param buffer 
         * @return size_t 
         */
        static size_t apply(T &data, unsigned char *buffer)
        {
            return SimpleObject<T>::serialize(data, buffer);
        }
    };

    /**
     * @brief 
     * 
     * @tparam T 
     */
    template <typename T>
    struct TypeSerializerImpl<T, false, false>
    {
        static const bool has_serialize = HasSerializeMethod<T>::value;
        /**
         * @brief 
         * 
         * @param data 
         * @param buffer 
         * @return size_t 
         */
        static size_t apply(T &data, unsigned char *buffer)
        {
            return ComplexObject<T, has_serialize>::serialize(data, buffer);
        }
    };

    /**
     * @brief 
     * 
     * @tparam T 
     * @tparam is_array 
     * @tparam is_trivial 
     */
    template <typename T, bool is_array, bool is_trivial>
    struct TypeUnserializerImpl
    {
        static int apply() = delete;
    };

    /**
     * @brief 
     * 
     * @tparam T 
     * @tparam N 
     */
    template <typename T, size_t N>
    struct TypeUnserializerImpl<T[N], true, true>
    {
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
     * @brief 
     * 
     * @tparam T 
     * @tparam N 
     */
    template <typename T, size_t N>
    struct TypeUnserializerImpl<T[N], true, false>
    {
        static const bool has_serialize = HasUnserializeMethod<T>::value;

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

    template <typename T>
    struct TypeUnserializerImpl<T, false, false>
    {
        static const bool has_serialize = HasUnserializeMethod<T>::value;

        static size_t apply(T& result, unsigned char *buffer, size_t buffer_size){
            return ComplexObject<T, has_serialize>::unserialize(result, buffer, buffer_size);
        }
    };

    template <typename T>
    struct TypeUnserializerImpl<T, false, true>
    {
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
         * @brief 
         * 
         * @tparam T 
         * @param data 
         * @param buffer 
         * @return size_t 
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


    struct TypeUnserializer
    {
        /**
         * @brief 
         * 
         * @tparam T 
         * @param data 
         * @param buffer 
         * @return size_t 
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
     * @brief 
     * 
     * @tparam BufferSize 
     */
    template <int BufferSize = 16384>
    struct Serialize
    {
        /**
         * @brief 
         * 
         * @tparam T 
         * @param buff_ptr 
         * @param data 
         * @return unsigned char* 
         */
        template <typename T>
        static inline unsigned char *exec_impl(unsigned char **buff_ptr, T& data)
        {
            *buff_ptr = *buff_ptr + TypeSerializer::apply(data, *buff_ptr);
            return *buff_ptr;
        }

        /**
         * @brief 
         * 
         * @tparam T 
         * @tparam TArgs 
         * @param buff_ptr 
         * @param data 
         * @param Args 
         * @return unsigned char* 
         */
        template <typename T, typename... TArgs>
        static inline unsigned char *exec_impl(unsigned char **buff_ptr, T& data, TArgs&... Args)
        {
            *buff_ptr = *buff_ptr + TypeSerializer::apply(data, *buff_ptr);
            return exec_impl(buff_ptr, Args...);
        }

        /**
         * @brief Set the hash object
         * 
         * @tparam T 
         * @tparam TArgs 
         * @param buffer 
         * @param data 
         * @param Args 
         * @return size_t 
         */
        template <typename T, typename... TArgs>
        static inline size_t set_hash(unsigned char *buffer, T& data, TArgs&... Args){
            size_t hash = MetaSerializer::TypeHasher::apply(data, Args...);
            std::memcpy(buffer, &hash, sizeof(size_t));
            return sizeof(size_t);
        }

        /**
         * @brief 
         * 
         * @tparam T 
         * @tparam TArgs 
         * @param data 
         * @param args 
         * @return std::string 
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

/************************************************************/

    template <int BufferSize = 16384>
    struct Unserialize
    {
        static const int hash_size = sizeof(size_t);

        template<typename T>
        static inline size_t get_hash_from_bytes(T& data){
            size_t hash;
            if(data.size() < hash_size){
                throw std::runtime_error("Deserialize Error! Data size is too small to be parsed.");
            }
            std::memcpy( &hash, data.data(), hash_size );
            return hash;
        }

        template<typename T, typename... TArgs>
        static inline bool check_type(T& raw_data,  TArgs... args){
            auto msg_hash = get_hash_from_bytes(raw_data);
            auto struct_hash = MetaSerializer::TypeHasher::apply(args...);
            if ( msg_hash == struct_hash ){
                return true;
            }else{
                return false;
            }
        }

        template <typename T>
        static inline void exec_impl(unsigned char *buff_ptr, size_t bytes_in_buffer, T& result_ref)
        {
            auto bytes_read = MetaSerializer::TypeUnserializer::apply(result_ref, buff_ptr, bytes_in_buffer);
            return;
        }

        template <typename T, typename... TArgs>
        static inline void exec_impl(unsigned char *buff_ptr, size_t bytes_in_buffer, T& result_ref, TArgs&... Args)
        {
            auto bytes_read = MetaSerializer::TypeUnserializer::apply(result_ref, buff_ptr, bytes_in_buffer);
            return exec_impl(buff_ptr+bytes_read, bytes_in_buffer-bytes_read, Args...);
        }

        template<typename T>
        static size_t copy_to_buffer(T& data, unsigned char* buffer){
            std::memset(buffer, 0, BufferSize);
            std::memcpy(buffer, (unsigned char*)data.data(), data.size());
            return data.size();

        }

        template <typename T, typename... TArgs>
        static inline void apply(T& data, TArgs&... args)
        {
            static unsigned char buffer[BufferSize] = {0};

            if(data.size() > BufferSize){
                throw std::runtime_error("Error while unserialize, Bytes are more than buffer capacity.");
            }

            size_t bytes_in_buffer=copy_to_buffer(data, buffer);
            
            if( ! check_type(data, args...) ){
                throw std::runtime_error("Types hash is different from the serial data hash.");
            }
            
            return exec_impl(buffer+hash_size, bytes_in_buffer-hash_size, args...);
        }
    };

};
