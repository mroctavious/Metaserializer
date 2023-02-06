#include <type_traits>
#include <string>
#include <vector>
#include <iostream>
#include <set>
#include <cstdint>
#include <cstring>
#include <typeinfo>
#include <memory>

/**
 * @brief This method will serialize a 
 * a class object or a set of values with
 * different datatypes.
 */
namespace MetaSerializer
{

    template<typename T, typename P>
    constexpr inline T convertTo(P bytes)
    {
        return static_cast<T>(bytes);
    }

    struct TypeHasher
    {

        template<typename T>
        struct Metahasher{
            //static const size_t value = ;
        };

        template<typename T>
        static constexpr std::size_t exec_impl(const size_t value, const T obj){
            constexpr const std::type_info &id = typeid(T);
            return value^id.hash_code();
        }

        template<typename T, typename... ArgsT>
        static constexpr std::size_t exec_impl(const size_t value, const T obj, const ArgsT... args){
            constexpr const std::type_info &id = typeid(T);
            const auto hash_value = value ^ id.hash_code();
            return exec_impl( hash_value, args...);
        }

        template <typename T, typename... ArgsT>
        static constexpr std::size_t apply(const T obj, const ArgsT... args)
        {
            return exec_impl(0, obj, args...);
        }

        struct EqualTo
        {
            using TypeInfoRef = std::reference_wrapper<const std::type_info>;
            bool operator()(TypeInfoRef lhs, TypeInfoRef rhs) const
            {
                return lhs.get() == rhs.get();
            }
        };
    };

    template <typename T>
    struct ContiguousMem
    {
        template <typename _DestT, typename _SrcT>
        static inline int serialize(_DestT dest, _SrcT src)
        {
            std::memcpy(dest, &src, sizeof(T));
            return sizeof(T);
        }

        static inline std::pair<T,int> deserialize(unsigned char *data)
        {
            T tmp;
            std::memcpy(&tmp, data, sizeof(T));
            return std::make_pair(tmp,sizeof(T));
        };
    };

    template <typename T>
    struct ObjectMem
    {
    };

    /**
     * @brief This class will serialize and deserialize a std::string type.
     * 
     * @tparam  Specialization, std::string.
     */
    template <>
    struct ObjectMem<std::string>
    {
        using SizeT = unsigned short;

        static inline int serialize(unsigned char *buffer, std::string &data)
        {
            auto bytes2cpy = sizeof(typename std::string::value_type) * data.size();
            SizeT byte_size_value = convertTo<SizeT>(bytes2cpy);
            std::memcpy(buffer, &byte_size_value, sizeof(SizeT));
            std::memcpy(buffer + sizeof(SizeT), data.data(), bytes2cpy);
            return sizeof(SizeT) + bytes2cpy;
        }

        static inline std::pair<std::string, int> deserialize(unsigned char *buffer)
        {
            SizeT size = 0;
            std::memcpy(&size, buffer, sizeof(SizeT));
            return std::make_pair(std::string((char *)(buffer + sizeof(int)), size), sizeof(int) + size);
        }
    };

    template <typename T>
    struct TypeSerializer
    {
        static const bool is_trivial = std::is_trivially_copyable<T>::value;
        typedef typename std::conditional<is_trivial, std::true_type, std::false_type>::type is_trivial_type;

        static inline int _apply(const std::true_type, T &data, unsigned char *buffer)
        {
            using unref_value_type = typename std::remove_reference<T>::type;
            return ContiguousMem<unref_value_type>::serialize(buffer, data);
        }

        static inline int _apply(const std::false_type, T &data, unsigned char *buffer)
        {
            using unref_value_type = typename std::remove_reference<T>::type;
            return ObjectMem<unref_value_type>::serialize(buffer, data);
        }

        static inline unsigned char *apply(T &data, unsigned char *buffer)
        {
            return buffer + _apply(std::integral_constant<bool, is_trivial>(), data, buffer);
        }
    };

    template <typename T>
    struct TypeDeserializer
    {
        static inline const bool is_trivial = std::is_trivially_copyable<T>::value;
        using unref_value_type = typename std::remove_reference<T>::type;

        static inline std::pair<T, unsigned char *> _apply(const std::true_type, unsigned char *buffer)
        {
            auto data = ContiguousMem<unref_value_type>::deserialize(buffer);
            return std::make_pair(data.first, data.second + buffer);
        }

        static inline std::pair<T, unsigned char *> _apply(const std::false_type, unsigned char *buffer)
        {
            auto data = ObjectMem<unref_value_type>::deserialize(buffer);
            return std::make_pair(data.first, data.second + buffer);
        }

        static inline std::pair<T, unsigned char *> apply(unsigned char *buffer)
        {
            return _apply(std::integral_constant<bool, is_trivial>(), buffer);
        }
    };

    template <int BufferSize = 8192>
    struct Serialize
    {
        template <typename T>
        static inline unsigned char *exec_impl(unsigned char **buff_ptr, T data)
        {
            *buff_ptr = TypeSerializer<T>::apply(data, *buff_ptr);
            return *buff_ptr;
        }

        template <typename T, typename... TArgs>
        static inline unsigned char *exec_impl(unsigned char **buff_ptr, T data, TArgs... Args)
        {
            *buff_ptr = TypeSerializer<T>::apply(data, *buff_ptr);
            return exec_impl(buff_ptr, Args...);
        }

        template <typename T, typename... TArgs>
        static inline int set_hash(unsigned char *buffer, T data, TArgs... Args){
            size_t hash = MetaSerializer::TypeHasher::apply(data, Args...);
            std::memcpy(buffer, &hash, sizeof(size_t));
            return sizeof(size_t);
        }

        template <typename T, typename... TArgs>
        static inline std::string apply(T data, TArgs... args)
        {
            unsigned char buffer[BufferSize] = {0};
            unsigned char *buffer_it = buffer + set_hash(buffer, data, args...);
            auto end_buffer = exec_impl(&buffer_it, data, args...);
            int size = end_buffer - buffer;
            return std::string((char *)buffer, size);
        }
    };

    struct Deserialize{

        static const int hash_size = sizeof(size_t);

        template<typename T>
        static inline void exec_impl(unsigned char **buff_ptr, T data){
            auto parsed_data = MetaSerializer::TypeDeserializer<T>::apply(*buff_ptr);
            data = parsed_data.first;
            *buff_ptr = parsed_data.second;
            return;
        }

        template<typename T, typename... TArgs>
        static inline void exec_impl(unsigned char **buff_ptr, T data, TArgs... args){
            auto parsed_data = MetaSerializer::TypeDeserializer<T>::apply(*buff_ptr);
            data = parsed_data.first;
            *buff_ptr = parsed_data.second;
            return exec_impl(buff_ptr,args...);
        }

        static inline size_t get_hash_from_bytes(std::string& data){
            size_t hash;
            if(data.size() < hash_size){
                throw std::runtime_error("Deserialize Error! Data size is too small to be parsed.");
            }
            std::memcpy( &hash, data.data(), hash_size );
            return hash;
        }

        template<typename... TArgs>
        static inline bool check_type(std::string raw_data,  TArgs... args){
            auto msg_hash = get_hash_from_bytes(raw_data);
            auto struct_hash = MetaSerializer::TypeHasher::apply(args...);
            if ( msg_hash == struct_hash ){
                return true;
            }else{
                return false;
            }
        }

        template<typename... TArgs>
        static inline void apply(std::string raw_data,  TArgs... args){
            if ( Deserialize::check_type(raw_data, args...) ){
                std::cout << "OK"  << std::endl;
                unsigned char *ptr_it = (unsigned char*)raw_data.data();
                return exec_impl(&ptr_it,args...);
            }else{
                std::cout << "ERROR"  << std::endl;
            }
            return;
        }
    };
};

template<typename T> 
std::size_t constexpr getID() { 
    constexpr const std::type_info &id = typeid(T);
    return id.hash_code();
};
