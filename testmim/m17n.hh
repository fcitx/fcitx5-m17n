#ifndef __M17N_HH
#define __M17N_HH

// Simple RAII wrapper for libm17n to ease resource management
#include <m17n.h>
namespace m17n {
    template <class T>
    void unref(T obj) {
        m17n_object_unref(obj);
    }
    template <class T>
    void destruct_nop(T obj) {
    }

    template <class T, void (*D)(T)=destruct_nop>
    class Object {
    public:
        T id; // in Freudian sense
        typedef T Wrapped;
        typedef Object<T, D> Wrapper;
        Object(T id_):
            id(id_) {}
        ~Object() {
            if (id) {
                D(id);
            }
        }
        operator T() {
            return id;
        }
    };

    typedef Object<MSymbol> Symbol;
    typedef Object<MText*, unref> Text;
    typedef Object<MPlist*, unref> Plist;
    typedef Object<MInputMethod*, minput_close_im> InputMethod;
    typedef Object<MInputContext*, minput_destroy_ic> InputContext;
    typedef Object<MConverter*, mconv_free_converter> Converter;
}

#endif
