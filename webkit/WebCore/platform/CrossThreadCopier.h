

#ifndef CrossThreadCopier_h
#define CrossThreadCopier_h

#include <memory>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>
#include <wtf/TypeTraits.h>

namespace WebCore {

    class ResourceError;
    struct ResourceRequest;
    class ResourceResponse;
    class String;
    struct CrossThreadResourceResponseData;
    struct CrossThreadResourceRequestData;

    template<typename T> struct CrossThreadCopierPassThrough {
        typedef T Type;
        static Type copy(const T& parameter)
        {
            return parameter;
        }
    };

    template<bool isConvertibleToInteger, typename T> struct CrossThreadCopierBase;

    // Integers get passed through without any changes.
    template<typename T> struct CrossThreadCopierBase<true, T> : public CrossThreadCopierPassThrough<T> {
    };

    // Pointers get passed through without any significant changes.
    template<typename T> struct CrossThreadCopierBase<false, T*> : public CrossThreadCopierPassThrough<T*> {
    };

    // Custom copy methods.
    template<typename T> struct CrossThreadCopierBase<false, RefPtr<ThreadSafeShared<T> > > {
        typedef PassRefPtr<T> Type;
        static Type copy(const RefPtr<ThreadSafeShared<T> >& refPtr)
        {
            return PassRefPtr<T>(static_cast<T*>(refPtr.get()));
        }
    };

    template<typename T> struct CrossThreadCopierBase<false, PassOwnPtr<T> > {
        typedef PassOwnPtr<T> Type;
        static Type copy(const PassOwnPtr<T>& ownPtr)
        {
            return PassOwnPtr<T>(static_cast<T*>(ownPtr.release()));
        }
    };

    template<typename T> struct CrossThreadCopierBase<false, std::auto_ptr<T> > {
        typedef std::auto_ptr<T> Type;
        static Type copy(const std::auto_ptr<T>& autoPtr)
        {
            return std::auto_ptr<T>(*const_cast<std::auto_ptr<T>*>(&autoPtr));
        }
    };

    template<> struct CrossThreadCopierBase<false, String> {
        typedef String Type;
        static Type copy(const String&);
    };

    template<> struct CrossThreadCopierBase<false, ResourceError> {
        typedef ResourceError Type;
        static Type copy(const ResourceError&);
    };

    template<> struct CrossThreadCopierBase<false, ResourceRequest> {
        typedef std::auto_ptr<CrossThreadResourceRequestData> Type;
        static Type copy(const ResourceRequest&);
    };

    template<> struct CrossThreadCopierBase<false, ResourceResponse> {
        typedef std::auto_ptr<CrossThreadResourceResponseData> Type;
        static Type copy(const ResourceResponse&);
    };

    template<typename T> struct CrossThreadCopier : public CrossThreadCopierBase<WTF::IsConvertibleToInteger<T>::value, T> {
    };

} // namespace WebCore

#endif // CrossThreadCopier_h
