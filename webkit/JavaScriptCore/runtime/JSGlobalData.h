

#ifndef JSGlobalData_h
#define JSGlobalData_h

#include "Collector.h"
#include "ExecutableAllocator.h"
#include "JITStubs.h"
#include "JSValue.h"
#include "MarkStack.h"
#include "SmallStrings.h"
#include "TimeoutChecker.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>

struct OpaqueJSClass;
struct OpaqueJSClassContextData;

namespace JSC {

    class CommonIdentifiers;
    class FunctionBodyNode;
    class IdentifierTable;
    class Interpreter;
    class JSGlobalObject;
    class JSObject;
    class Lexer;
    class Parser;
    class ScopeNode;
    class Stringifier;
    class Structure;
    class UString;

    struct HashTable;
    struct Instruction;    
    struct VPtrSet;

    class JSGlobalData : public RefCounted<JSGlobalData> {
    public:
        struct ClientData {
            virtual ~ClientData() = 0;
        };

        static bool sharedInstanceExists();
        static JSGlobalData& sharedInstance();

        static PassRefPtr<JSGlobalData> create(bool isShared = false);
        static PassRefPtr<JSGlobalData> createLeaked();
        ~JSGlobalData();

#if ENABLE(JSC_MULTIPLE_THREADS)
        // Will start tracking threads that use the heap, which is resource-heavy.
        void makeUsableFromMultipleThreads() { heap.makeUsableFromMultipleThreads(); }
#endif

        bool isSharedInstance;
        ClientData* clientData;

        const HashTable* arrayTable;
        const HashTable* dateTable;
        const HashTable* jsonTable;
        const HashTable* mathTable;
        const HashTable* numberTable;
        const HashTable* regExpTable;
        const HashTable* regExpConstructorTable;
        const HashTable* stringTable;
        
        RefPtr<Structure> activationStructure;
        RefPtr<Structure> interruptedExecutionErrorStructure;
        RefPtr<Structure> staticScopeStructure;
        RefPtr<Structure> stringStructure;
        RefPtr<Structure> notAnObjectErrorStubStructure;
        RefPtr<Structure> notAnObjectStructure;
        RefPtr<Structure> propertyNameIteratorStructure;
        RefPtr<Structure> getterSetterStructure;
        RefPtr<Structure> apiWrapperStructure;

#if USE(JSVALUE32)
        RefPtr<Structure> numberStructure;
#endif

        void* jsArrayVPtr;
        void* jsByteArrayVPtr;
        void* jsStringVPtr;
        void* jsFunctionVPtr;

        IdentifierTable* identifierTable;
        CommonIdentifiers* propertyNames;
        const MarkedArgumentBuffer* emptyList; // Lists are supposed to be allocated on the stack to have their elements properly marked, which is not the case here - but this list has nothing to mark.
        SmallStrings smallStrings;

#if ENABLE(ASSEMBLER)
        ExecutableAllocator executableAllocator;
#endif

        Lexer* lexer;
        Parser* parser;
        Interpreter* interpreter;
#if ENABLE(JIT)
        JITThunks jitStubs;
#endif
        TimeoutChecker timeoutChecker;
        Heap heap;

        JSValue exception;
#if ENABLE(JIT)
        ReturnAddressPtr exceptionLocation;
#endif

        const Vector<Instruction>& numericCompareFunction(ExecState*);
        Vector<Instruction> lazyNumericCompareFunction;
        bool initializingLazyNumericCompareFunction;

        HashMap<OpaqueJSClass*, OpaqueJSClassContextData*> opaqueJSClassData;

        JSGlobalObject* head;
        JSGlobalObject* dynamicGlobalObject;

        HashSet<JSObject*> arrayVisitedElements;

        ScopeNode* scopeNodeBeingReparsed;
        Stringifier* firstStringifierToMark;

        MarkStack markStack;
    private:
        JSGlobalData(bool isShared, const VPtrSet&);
        static JSGlobalData*& sharedInstanceInternal();
        void createNativeThunk();
    };

} // namespace JSC

#endif // JSGlobalData_h
