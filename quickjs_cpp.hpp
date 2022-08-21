#ifndef QUICKJS_CPP_HPP_INCLUDED
#define QUICKJS_CPP_HPP_INCLUDED

#include <variant>
#include <vector>
#include <map>
#include <optional>
#include <tuple>
#include <assert.h>
#include <nlohmann/json.hpp>
#include <quickjs/quickjs.h>

namespace js_quickjs
{
    void throw_exception(JSContext* ctx, JSValue val, const std::string& data = "");

    struct value;

    struct value_context
    {
        std::vector<value> this_stack;

        JSRuntime* heap = nullptr;
        JSContext* ctx = nullptr;
        bool runtime_owner = false;
        bool context_owner = false;

        value_context(JSContext* ctx);
        value_context(value_context&);
        value_context(JSInterruptHandler handler, void* sandbox);
        ~value_context();

        value_context& operator=(const value_context& other);

        void push_this(const value& val);
        void pop_this();
        value get_current_this();

        void execute_jobs();
        void execute_timeout_check();
        void compact_heap_stash();
    };

    using funcptr_t = JSValue (*)(JSContext*, JSValueConst, int, JSValueConst*);

    struct undefined_t{};
    const static inline undefined_t undefined;

    struct null_t{};
    const static inline null_t null;

    namespace args
    {
        JSValue push(JSContext* ctx, const char* v);
        JSValue push(JSContext* ctx, const std::string& v);
        JSValue push(JSContext* ctx, int64_t v);
        JSValue push(JSContext* ctx, int v);
        JSValue push(JSContext* ctx, double v);
        JSValue push(JSContext* ctx, bool v);
        template<typename T>
        JSValue push(JSContext* ctx, const std::vector<T>& v);
        template<typename T, typename U>
        JSValue push(JSContext* ctx, const std::map<T, U>& v);
        JSValue push(JSContext* ctx, js_quickjs::funcptr_t fptr);
        JSValue push(JSContext* ctx, const nlohmann::json& in);
        template<typename T>
        JSValue push(JSContext* ctx, T* in);
        JSValue push(JSContext* ctx, std::nullptr_t in);
        JSValue push(JSContext* ctx, const js_quickjs::value& in);
        JSValue push(JSContext* ctx, js_quickjs::funcptr_t in);
        JSValue push(JSContext* ctx, const JSValue& in);
        template<typename T>
        JSValue push(JSContext* ctx, const std::optional<T>& v);

        inline
        JSValue push(JSContext* ctx, const char* v)
        {
            return JS_NewString(ctx, v);
        }

        inline
        JSValue push(JSContext* ctx, const std::string& v)
        {
            return JS_NewStringLen(ctx, v.c_str(), v.size());
        }

        inline
        JSValue push(JSContext* ctx, int64_t v)
        {
            return JS_NewInt64(ctx, v);
        }

        inline
        JSValue push(JSContext* ctx, int v)
        {
            return JS_NewInt32(ctx, v);
        }

        inline
        JSValue push(JSContext* ctx, double v)
        {
            return JS_NewFloat64(ctx, v);
        }

        inline
        JSValue push(JSContext* ctx, bool v)
        {
            return JS_NewBool(ctx,v);
        }

        template<typename T>
        inline
        JSValue push(JSContext* ctx, const std::vector<T>& v)
        {
            JSValue val = JS_NewArray(ctx);

            for(int i=0; i < (int)v.size(); i++)
            {
                JSValue found = push(ctx, v[i]);
                JS_SetPropertyUint32(ctx, val, i, found);
            }

            return val;
        }

        template<typename T, typename U>
        inline
        JSValue push(JSContext* ctx, const std::map<T, U>& v)
        {
            JSValue obj = JS_NewObject(ctx);

            for(auto& i : v)
            {
                JSValue key = push(ctx, i.first);
                JSValue val = push(ctx, i.second);

                JSAtom key_atom = JS_ValueToAtom(ctx, key);

                JS_SetProperty(ctx, obj, key_atom, val);

                JS_FreeAtom(ctx, key_atom);
                JS_FreeValue(ctx, key);
            }

            return obj;
        }

        JSValue push(JSContext* ctx, js_quickjs::funcptr_t fptr);

        inline
        JSValue push(JSContext* ctx, const js_quickjs::undefined_t&)
        {
            return JS_UNDEFINED;
        }

        inline
        JSValue push(JSContext* ctx, const js_quickjs::null_t&)
        {
            return JS_NULL;
        }

        inline
        JSValue push(JSContext* ctx, const nlohmann::json& in)
        {
            std::string str = in.dump();

            return JS_ParseJSON(ctx, str.c_str(), str.size(), nullptr);
        }

        template<typename T>
        inline
        JSValue push(JSContext* ctx, T* in)
        {
            return JS_MKPTR(JS_TAG_UNINITIALIZED, in);
        }

        inline
        JSValue push(JSContext* ctx, std::nullptr_t in)
        {
            return JS_MKPTR(JS_TAG_UNINITIALIZED, in);
        }

        JSValue push(JSContext* ctx, const js_quickjs::value& in);

        inline
        JSValue push(JSContext* ctx, js_quickjs::funcptr_t in)
        {
            return JS_NewCFunction(ctx, in, "", 0);
        }

        inline
        JSValue push(JSContext* ctx, const JSValue& val)
        {
            return JS_DupValue(ctx, val);
        }

        template<typename T>
        inline
        JSValue push(JSContext* ctx, const std::optional<T>& in)
        {
            if(in.has_value())
            {
                return push(ctx, in.value());
            }
            else
            {
                return push(ctx, js_quickjs::undefined_t());
            }
        }

        #define UNDEF() if(JS_IsUndefined(val)){out = std::remove_reference_t<decltype(out)>(); return;}

        void get(js_quickjs::value_context& vctx, const JSValue& val, std::string& out);
        void get(js_quickjs::value_context& vctx, const JSValue& val, int64_t& out);
        void get(js_quickjs::value_context& vctx, const JSValue& val, int& out);
        void get(js_quickjs::value_context& vctx, const JSValue& val, double& out);
        void get(js_quickjs::value_context& vctx, const JSValue& val, bool& out);
        template<typename T, typename U>
        void get(js_quickjs::value_context& vctx, const JSValue& val, std::map<T, U>& out);
        template<typename T>
        void get(js_quickjs::value_context& vctx, const JSValue& val, std::vector<T>& out);
        template<typename T>
        void get(js_quickjs::value_context& vctx, const JSValue& val, T*& out);
        void get(js_quickjs::value_context& vctx, const JSValue& val, js_quickjs::value& out);
        void get(js_quickjs::value_context& vctx, const JSValue& val, std::vector<std::pair<js_quickjs::value, js_quickjs::value>>& out); ///equivalent to std::map
        void get(js_quickjs::value_context& vctx, const JSValue& val, std::vector<js_quickjs::value>& out); ///equivalent to std::map

        inline
        void get(js_quickjs::value_context& vctx, const JSValue& val, std::string& out)
        {
            UNDEF();

            size_t len = 0;
            const char* str = JS_ToCStringLen(vctx.ctx, &len, val);

            if(str == nullptr)
            {
                out = "";
                return;
            }

            out = std::string(str, str + len);

            JS_FreeCString(vctx.ctx, str);
        }

        inline
        void get(js_quickjs::value_context& vctx, const JSValue& val, int64_t& out)
        {
            UNDEF();

            JS_ToInt64(vctx.ctx, &out, val);
        }

        inline
        void get(js_quickjs::value_context& vctx, const JSValue& val, int& out)
        {
            UNDEF();

            int32_t ival = 0;

            JS_ToInt32(vctx.ctx, &ival, val);
            out = ival;
        }

        inline
        void get(js_quickjs::value_context& vctx, const JSValue& val, double& out)
        {
            UNDEF();

            JS_ToFloat64(vctx.ctx, &out, val);
        }

        inline
        void get(js_quickjs::value_context& vctx, const JSValue& val, bool& out)
        {
            UNDEF();

            out = JS_ToBool(vctx.ctx, val) > 0;
        }

        template<typename T, typename U>
        inline
        void get(js_quickjs::value_context& vctx, const JSValue& val, std::map<T, U>& out)
        {
            UNDEF();

            JSPropertyEnum* names = nullptr;
            uint32_t len = 0;

            JS_GetOwnPropertyNames(vctx.ctx, &names, &len, val, JS_GPN_STRING_MASK|JS_GPN_SYMBOL_MASK);

            if(names == nullptr)
            {
                out.clear();
                return;
            }

            for(int i=0; i < (int)len; i++)
            {
                JSAtom atom = names[i].atom;

                JSValue found = JS_GetProperty(vctx.ctx, val, atom);
                JSValue key = JS_AtomToValue(vctx.ctx, atom);

                T out_key;
                get(vctx, key, out_key);
                U out_value;
                get(vctx, found, out_value);

                out[out_key] = out_value;

                JS_FreeValue(vctx.ctx, found);
                JS_FreeValue(vctx.ctx, key);
            }

            for(int i=0; i < (int)len; i++)
            {
                JS_FreeAtom(vctx.ctx, names[i].atom);
            }

            js_free(vctx.ctx, names);
        }

        template<typename T>
        inline
        void get(js_quickjs::value_context& vctx, const JSValue& val, std::vector<T>& out)
        {
            UNDEF();

            out.clear();

            JSValue jslen = JS_GetPropertyStr(vctx.ctx, val, "length");

            int32_t len = 0;
            JS_ToInt32(vctx.ctx, &len, jslen);

            JS_FreeValue(vctx.ctx, jslen);

            out.reserve(len);

            for(int i=0; i < len; i++)
            {
                JSValue found = JS_GetPropertyUint32(vctx.ctx, val, i);

                T next;
                get(vctx, found, next);

                out.push_back(next);

                JS_FreeValue(vctx.ctx, found);
            }
        }

        template<typename T>
        inline
        void get(js_quickjs::value_context& vctx, const JSValue& val, T*& out)
        {
            UNDEF();

            out = (T*)JS_VALUE_GET_PTR(val);
        }

        inline
        void get(js_quickjs::value_context& vctx, const JSValue& val, std::vector<std::pair<js_quickjs::value, js_quickjs::value>>& out);
    }

    struct value;

    struct qstack_manager
    {
        value& val;

        qstack_manager(value& _val);
        ~qstack_manager();
    };

    struct value
    {
        value_context* vctx = nullptr;
        JSContext* ctx = nullptr;
        JSValue val = {};
        JSValue parent_value = {};
        bool has_value = false;
        bool has_parent = false;
        bool released = false;

        std::variant<std::monostate, int, std::string> indices;

        value(const value& other);
        //value(value&& other);
        ///pushes a fresh object
        value(value_context& ctx);
        value(value_context& ctx, const js_quickjs::undefined_t&);
        value(value_context& ctx, const value& other);
        value(value_context& ctx, const value& base, const std::string& key);
        value(value_context& ctx, const value& base, int key);
        value(value_context& ctx, const value& base, const char* key);
        ~value();

        bool has(const std::string& key) const;
        bool has(int key) const;
        bool has(const char* key) const;
        bool has_hidden(const std::string& key) const;

        value get(const std::string& key);
        value get(int key);
        value get(const char* key);
        value get_hidden(const std::string& key);

        bool del(const std::string& key);

        void add_hidden_value(const std::string& key, const value& val);

        template<typename T>
        value add(const std::string& key, const T& val)
        {
            auto jval = js_quickjs::value(*vctx, *this, key);
            jval = val;
            return jval;
        }

        template<typename T>
        value add_hidden(const std::string& key, const T& val)
        {
            value fval(*vctx);
            fval = val;

            add_hidden_value(key, fval);

            return fval;
        }

        bool is_string();
        bool is_number();
        bool is_array();
        bool is_map();
        bool is_empty();
        bool is_function() const;
        bool is_boolean();
        bool is_undefined() const;
        bool is_truthy();
        bool is_object_coercible();
        bool is_object() const;
        bool is_error() const;
        bool is_exception() const;

        ///stop managing element
        void release();

        value& operator=(const char* v);
        value& operator=(const std::string& v);
        value& operator=(int64_t v);
        value& operator=(int v);
        value& operator=(double v);
        value& operator=(bool v);
        value& operator=(std::nullopt_t v);
        value& operator=(const value& right);
        //value& operator=(value&& right);
        value& operator=(js_quickjs::undefined_t);
        value& operator=(js_quickjs::null_t);
        value& operator=(const nlohmann::json&);

        template<typename T>
        value& operator=(const std::vector<T>& in)
        {
            qstack_manager m(*this);

            val = args::push(ctx, in);

            return *this;
        }

        template<typename T, typename U>
        value& operator=(const std::map<T, U>& in)
        {
            qstack_manager m(*this);

            val = args::push(ctx, in);

            return *this;
        }

        value& operator=(js_quickjs::funcptr_t fptr);
        value& operator=(const JSValue& val);

        value& set_ptr(std::nullptr_t)
        {
            qstack_manager m(*this);

            val = args::push(ctx, nullptr);

            return *this;
        }

        ///seems dangerous to let this be arbitrary, because its extremely rarely what you want
        template<typename T>
        value& set_ptr(T* in)
        {
            qstack_manager m(*this);

            val = args::push(ctx, in);

            return *this;
        }


        template<typename T>
        value& allocate_in_heap(T& in)
        {
            T* val = new T(in);
            return set_ptr(val);
        }

        template<typename T>
        T* get_ptr()
        {
            if(!has_value)
                throw std::runtime_error("No value in trying to get a pointer. This is dangerous");

            T* ret;
            args::get(*vctx, val, ret);
            return ret;
        }

        template<typename T>
        void free_in_heap()
        {
            if(!has_value)
                return;

            T* ptr = get_ptr<T>();

            if(ptr == nullptr)
                return;

            delete ptr;

            T* null = nullptr;
            set_ptr(null);
        }

        operator std::string() const;
        operator int64_t() const;
        operator int() const;
        operator double() const;
        operator bool() const;

        template<typename T>
        operator std::vector<T>() const
        {
            if(!has_value)
                return std::vector<T>();

            std::vector<T> ret;
            args::get(*vctx, val, ret);
            return ret;
        }

        template<typename T, typename U>
        operator std::map<T, U>() const
        {
            if(!has_value)
                return std::map<T, U>();

            std::map<T, U> ret;
            args::get(*vctx, val, ret);
            return ret;
        }

        template<typename T>
        operator T*() const
        {
            if(!has_value)
                return nullptr;

            static_assert(!std::is_same_v<T, char const>, "Trying to get a const char* pointer out is almost certainly not what you want");

            T* ret;
            args::get(*vctx, val, ret);
            return ret;
        }

        std::vector<std::pair<js_quickjs::value, js_quickjs::value>> iterate();

        value operator[](int64_t val);
        value operator[](const std::string& str);
        value operator[](const char* str);

        void pack(){}
        void stringify_parse();

        void from_json(const std::string& in);
        std::string to_json();
        std::string to_error_message();
        nlohmann::json to_nlohmann(int stack_depth = 0);
    };

    inline
    JSValue val2value(const value& in)
    {
        return in.val;
    }

    //JSValue process_return_value(JSContext* ctx, JSValue in);

    ///This eats C++ exceptions and converts them into JS exceptions
    template<typename... T>
    inline
    std::pair<bool, value> call(value& func, T&&... vals)
    {
        bool all_same = ((vals.ctx == func.ctx) && ...);

        if(!all_same)
            throw std::runtime_error("Not all same contexts");

        constexpr size_t nargs = sizeof...(T);

        JSValue arr[nargs] = {val2value(vals)...};

        JSValue glob = JS_GetGlobalObject(func.ctx);

        ///not sure this will work for compiled scripts
        JSValue ret = JS_Call(func.ctx, func.val, glob, nargs, arr);

        JS_FreeValue(func.ctx, glob);

        if(JS_IsException(ret))
            throw_exception(func.ctx, ret);

        bool err = JS_IsError(func.ctx, ret);

        value rval(*func.vctx);
        rval = ret;

        JS_FreeValue(func.ctx, ret);

        return {!err, rval};
    }

    std::pair<bool, value> call_compiled(value& bitcode);

    template<typename I, typename... T>
    inline
    std::pair<bool, value> call_prop(value& obj, I& key, T&&... vals)
    {
        value func = obj[key];

        return call(func, std::forward<T>(vals)...);
    }

    js_quickjs::value execute_promises(js_quickjs::value_context& vctx, js_quickjs::value& potential_promise);

    template<typename T, typename Enable = void>
    struct is_optional : std::false_type {};

    template<typename T>
    struct is_optional<std::optional<T>> : std::true_type {};

    template<typename T, typename... U>
    constexpr bool is_first_context()
    {
        return std::is_same_v<T, js_quickjs::value_context*>;
    }

    template<typename T, typename... U>
    constexpr int num_args(T(*fptr)(U...))
    {
        if constexpr(is_first_context<U...>())
            return sizeof...(U) - 1;
        else
            return sizeof...(U);
    }

    template<typename T, typename... U>
    constexpr int num_rets(T(*fptr)(U...))
    {
        return !std::is_same_v<void, T>;
    }

    template<typename T, int N>
    inline
    T get_element(js_quickjs::value_context& vctx, int argc, JSValueConst* argv)
    {
        constexpr bool is_first_value = std::is_same_v<T, js_quickjs::value_context*> && N == 0;

        if constexpr(is_first_value)
        {
            return &vctx;
        }

        if constexpr(!is_first_value)
        {
            if((N - 1) >= argc)
            {
                value val(vctx);
                val = js_quickjs::undefined;

                if constexpr(is_optional<T>::value)
                    return std::nullopt;

                return val;
            }
            else
            {
                value val(vctx);
                val = argv[N - 1];

                if constexpr(is_optional<T>::value)
                {
                    if(val.is_undefined())
                        return std::nullopt;
                    else
                        return (typename T::value_type)val;
                }

                return val;
            }
        }
    }

    template<typename... U, std::size_t... Is>
    inline
    std::tuple<U...> get_args(js_quickjs::value_context& vctx, std::index_sequence<Is...>, int argc, JSValueConst* argv)
    {
        return std::make_tuple(get_element<U, Is>(vctx, argc, argv)...);
    }

    struct this_cleaner
    {
        js_quickjs::value_context& vctx;

        this_cleaner(js_quickjs::value_context& _vctx) : vctx(_vctx) {}

        ~this_cleaner()
        {
            vctx.pop_this();
        }
    };

    template<typename T, typename... U>
    inline
    JSValue js_safe_function_decomposed(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst *argv, T(*func)(U...))
    {
        ///semantics here are wrong
        ///need to pad arguments up to this size with undefined
        if(argc > js_quickjs::num_args(func))
        {
            return JS_ThrowInternalError(ctx, "Bad quickjs function, too many args");
        }

        static_assert(is_first_context<U...>());

        js_quickjs::value_context vctx(ctx);

        vctx.execute_timeout_check();

        js_quickjs::value func_this(vctx);
        func_this = this_val;

        vctx.push_this(func_this);
        this_cleaner clean(vctx);

        std::index_sequence_for<U...> iseq;

        auto tup = get_args<U...>(vctx, iseq, argc, argv);

        try
        {
            if constexpr(std::is_same_v<void, T>)
            {
                std::apply(func, tup);

                js_quickjs::value val(vctx);
                val = js_quickjs::undefined;

                val.release();

                return val.val;
            }
            else
            {
                auto rval = std::apply(func, tup);

                if constexpr(std::is_same_v<T, js_quickjs::value>)
                {
                    rval.release();

                    return rval.val;
                }
                else
                {
                    js_quickjs::value val(vctx);
                    val = rval;
                    val.release();

                    return val.val;
                }
            }
        }
        catch(std::runtime_error& err)
        {
            const char* str = err.what();

            return JS_ThrowInternalError(ctx, "%s", str);
        }
        catch(...)
        {
            return JS_ThrowInternalError(ctx, "Unknown C++ exception");
        }
    }

    template<auto func>
    inline
    JSValue function(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
    {
        return js_safe_function_decomposed(ctx, this_val, argc, argv, func);
    }

    //still need call

    js_quickjs::value get_global(value_context& vctx);
    void set_global(value_context& vctx, const js_quickjs::value& val);
    js_quickjs::value get_current_function(value_context& vctx);
    js_quickjs::value get_this(value_context& vctx);
    js_quickjs::value get_heap_stash(value_context& vctx);
    js_quickjs::value get_global_stash(value_context& vctx);
    void* get_sandbox_data_impl(value_context& vctx);

    template<typename T>
    inline
    T* get_sandbox_data(value_context& vctx)
    {
        return (T*)get_sandbox_data_impl(vctx);
    }

    #if 0
    value add_getter(value& base, const std::string& key, js_quickjs::funcptr_t func);
    value add_setter(value& base, const std::string& key, js_quickjs::funcptr_t func);
    #endif // 0

    std::pair<value, value> add_getter_setter(value& base, const std::string& key, js_quickjs::funcptr_t get, js_quickjs::funcptr_t set);

    std::pair<bool, value> compile(value_context& vctx, const std::string& data);
    std::pair<bool, value> compile(value_context& vctx, const std::string& data, const std::string& name);

    std::string dump_function(value& val);
    value eval(value_context& vctx, const std::string& data, const std::string& name = "test-eval");
    value eval_module(value_context& vctx, const std::string& data, const std::string& name = "test-eval");
    value compile_module(value_context& vctx, const std::string& data, const std::string& name = "test-eval");
    value xfer_between_contexts(value_context& destination, const value& val);

    value make_proxy(value& target, value& handle);
    //unimplemented
    value from_cbor(value_context& vctx, const std::vector<uint8_t>& cb);

    void dump_stack(value_context& vctx);

    template<typename T>
    inline
    value make_value(value_context& vctx, const T& t)
    {
        value v(vctx);
        v = t;
        return v;
    }

    ///this is a convention, not a formal type
    template<typename T>
    inline
    value make_error(value_context& vctx, const T& msg)
    {
        value v(vctx);
        v["ok"] = false;
        v["msg"] = msg;

        return v;
    }

    inline
    value make_success(value_context& vctx)
    {
        value v(vctx);
        v["ok"] = true;

        return v;
    }

    template<typename T>
    inline
    value make_success(value_context& vctx, const T& msg)
    {
        value v(vctx);
        v["ok"] = true;
        v["msg"] = msg;

        return v;
    }

    template<typename T, typename U>
    inline
    value add_key_value(value& base, const T& key, const U& val)
    {
        assert(base.vctx);

        value nval(*base.vctx, base, key);
        nval = val;
        return nval;
    }

    inline
    void empty_function(value_context*)
    {

    }
}

#endif
